// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <side/trace.h>
#include <string.h>

#include "tracer.h"
#include "rcu.h"
#include "list.h"

/* Top 8 bits reserved for kernel tracer use. */
#define SIDE_EVENT_ENABLED_KERNEL_MASK			0xFF000000
#define SIDE_EVENT_ENABLED_KERNEL_USER_EVENT_MASK	0x80000000

/* Allow 2^24 tracer callbacks to be registered on an event. */
#define SIDE_EVENT_ENABLED_USER_MASK			0x00FFFFFF

static struct side_rcu_gp_state rcu_gp;

/*
 * Lazy initialization for early use within library constructors.
 */
static bool initialized;

static pthread_mutex_t side_lock = PTHREAD_MUTEX_INITIALIZER;

static DEFINE_SIDE_LIST_HEAD(side_list);

static
void side_init(void)
	__attribute__((constructor));

/*
 * The empty callback has a NULL function callback pointer, which stops
 * iteration on the array of callbacks immediately.
 */
const struct side_callback side_empty_callback;

void side_call(const struct side_event_description *desc, const struct side_arg_vec_description *sav_desc)
{
	const struct side_callback *side_cb;
	unsigned int rcu_period;
	uint32_t enabled;

	if (side_unlikely(!initialized))
		side_init();
	if (side_unlikely(desc->flags & SIDE_EVENT_FLAG_VARIADIC)) {
		printf("ERROR: unexpected variadic event description\n");
		abort();
	}
	enabled = __atomic_load_n(desc->enabled, __ATOMIC_RELAXED);
	if (side_unlikely(enabled & SIDE_EVENT_ENABLED_KERNEL_USER_EVENT_MASK)) {
		// TODO: call kernel write.
	}
	if (side_unlikely(!(enabled & SIDE_EVENT_ENABLED_USER_MASK)))
		return;

	//TODO: replace tracer_call by rcu iteration on list of registered callbacks
	tracer_call(desc, sav_desc, NULL);

	rcu_period = side_rcu_read_begin(&rcu_gp);
	for (side_cb = side_rcu_dereference(desc->callbacks); side_cb->u.call != NULL; side_cb++)
		side_cb->u.call(desc, sav_desc, side_cb->priv);
	side_rcu_read_end(&rcu_gp, rcu_period);
}

void side_call_variadic(const struct side_event_description *desc,
	const struct side_arg_vec_description *sav_desc,
	const struct side_arg_dynamic_event_struct *var_struct)
{
	const struct side_callback *side_cb;
	unsigned int rcu_period;
	uint32_t enabled;

	if (side_unlikely(!initialized))
		side_init();
	if (side_unlikely(!(desc->flags & SIDE_EVENT_FLAG_VARIADIC))) {
		printf("ERROR: unexpected non-variadic event description\n");
		abort();
	}
	enabled = __atomic_load_n(desc->enabled, __ATOMIC_RELAXED);
	if (side_unlikely(enabled & SIDE_EVENT_ENABLED_KERNEL_USER_EVENT_MASK)) {
		// TODO: call kernel write.
	}
	if (side_unlikely(!(enabled & SIDE_EVENT_ENABLED_USER_MASK)))
		return;

	//TODO: replace tracer_call by rcu iteration on list of registered callbacks
	tracer_call_variadic(desc, sav_desc, var_struct, NULL);

	rcu_period = side_rcu_read_begin(&rcu_gp);
	for (side_cb = side_rcu_dereference(desc->callbacks); side_cb->u.call_variadic != NULL; side_cb++)
		side_cb->u.call_variadic(desc, sav_desc, var_struct, side_cb->priv);
	side_rcu_read_end(&rcu_gp, rcu_period);
}

static
const struct side_callback *side_tracer_callback_lookup(
		const struct side_event_description *desc,
		void (*call)(), void *priv)
{
	const struct side_callback *cb;

	for (cb = desc->callbacks; cb->u.call != NULL; cb++) {
		if (cb->u.call == call && cb->priv == priv)
			return cb;
	}
	return NULL;
}

static
int _side_tracer_callback_register(struct side_event_description *desc,
		void (*call)(), void *priv)
{
	struct side_callback *old_cb, *new_cb;
	int ret = SIDE_ERROR_OK;
	uint32_t old_nr_cb;

	if (!call)
		return SIDE_ERROR_INVAL;
	pthread_mutex_lock(&side_lock);
	old_nr_cb = *desc->enabled & SIDE_EVENT_ENABLED_USER_MASK;
	if (old_nr_cb == SIDE_EVENT_ENABLED_USER_MASK) {
		ret = SIDE_ERROR_INVAL;
		goto unlock;
	}
	/* Reject duplicate (call, priv) tuples. */
	if (side_tracer_callback_lookup(desc, call, priv)) {
		ret = SIDE_ERROR_EXIST;
		goto unlock;
	}
	old_cb = (struct side_callback *) desc->callbacks;
	/* old_nr_cb + 1 (new cb) + 1 (NULL) */
	new_cb = (struct side_callback *) calloc(old_nr_cb + 2, sizeof(struct side_callback));
	if (!new_cb) {
		ret = SIDE_ERROR_NOMEM;
		goto unlock;
	}
	memcpy(new_cb, old_cb, old_nr_cb);
	if (desc->flags & SIDE_EVENT_FLAG_VARIADIC)
		new_cb[old_nr_cb].u.call_variadic = call;
	else
		new_cb[old_nr_cb].u.call = call;
	new_cb[old_nr_cb].priv = priv;
	side_rcu_assign_pointer(desc->callbacks, new_cb);
	side_rcu_wait_grace_period(&rcu_gp);
	if (old_nr_cb)
		free(old_cb);
	/* Increment concurrently with kernel setting the top bits. */
	(void) __atomic_add_fetch(desc->enabled, 1, __ATOMIC_RELAXED);
unlock:
	pthread_mutex_unlock(&side_lock);
	return ret;
}

int side_tracer_callback_register(struct side_event_description *desc,
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
			void *priv),
		void *priv)
{
	if (desc->flags & SIDE_EVENT_FLAG_VARIADIC)
		return SIDE_ERROR_INVAL;
	return _side_tracer_callback_register(desc, call, priv);
}

int side_tracer_callback_variadic_register(struct side_event_description *desc,
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
			const struct side_arg_dynamic_event_struct *var_struct,
			void *priv),
		void *priv)
{
	if (!(desc->flags & SIDE_EVENT_FLAG_VARIADIC))
		return SIDE_ERROR_INVAL;
	return _side_tracer_callback_register(desc, call_variadic, priv);
}

int _side_tracer_callback_unregister(struct side_event_description *desc,
		void (*call)(), void *priv)
{
	struct side_callback *old_cb, *new_cb;
	const struct side_callback *cb_pos;
	uint32_t pos_idx;
	int ret = SIDE_ERROR_OK;
	uint32_t old_nr_cb;

	if (!call)
		return SIDE_ERROR_INVAL;
	pthread_mutex_lock(&side_lock);
	cb_pos = side_tracer_callback_lookup(desc, call, priv);
	if (!cb_pos) {
		ret = SIDE_ERROR_NOENT;
		goto unlock;
	}
	old_nr_cb = *desc->enabled & SIDE_EVENT_ENABLED_USER_MASK;
	old_cb = (struct side_callback *) desc->callbacks;
	if (old_nr_cb == 1) {
		new_cb = (struct side_callback *) &side_empty_callback;
	} else {
		pos_idx = cb_pos - desc->callbacks;
		/* Remove entry at pos_idx. */
		/* old_nr_cb - 1 (removed cb) + 1 (NULL) */
		new_cb = (struct side_callback *) calloc(old_nr_cb, sizeof(struct side_callback));
		if (!new_cb) {
			ret = SIDE_ERROR_NOMEM;
			goto unlock;
		}
		memcpy(new_cb, old_cb, pos_idx);
		memcpy(&new_cb[pos_idx], &old_cb[pos_idx + 1], old_nr_cb - pos_idx - 1);
	}
	side_rcu_assign_pointer(desc->callbacks, new_cb);
	side_rcu_wait_grace_period(&rcu_gp);
	free(old_cb);
	/* Decrement concurrently with kernel setting the top bits. */
	(void) __atomic_add_fetch(desc->enabled, -1, __ATOMIC_RELAXED);
unlock:
	pthread_mutex_unlock(&side_lock);
	return ret;
}

int side_tracer_callback_unregister(struct side_event_description *desc,
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
			void *priv),
		void *priv)
{
	if (desc->flags & SIDE_EVENT_FLAG_VARIADIC)
		return SIDE_ERROR_INVAL;
	return _side_tracer_callback_unregister(desc, call, priv);
}

int side_tracer_callback_variadic_unregister(struct side_event_description *desc,
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
			const struct side_arg_dynamic_event_struct *var_struct,
			void *priv),
		void *priv)
{
	if (!(desc->flags & SIDE_EVENT_FLAG_VARIADIC))
		return SIDE_ERROR_INVAL;
	return _side_tracer_callback_unregister(desc, call_variadic, priv);
}

static
void side_init(void)
{
	if (initialized)
		return;
	side_rcu_gp_init(&rcu_gp);
	initialized = true;
}
