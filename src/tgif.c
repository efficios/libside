// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <tgif/trace.h>
#include <string.h>

#include "rcu.h"
#include "list.h"

/* Top 8 bits reserved for kernel tracer use. */
#if TGIF_BITS_PER_LONG == 64
# define TGIF_EVENT_ENABLED_KERNEL_MASK			0xFF00000000000000ULL
# define TGIF_EVENT_ENABLED_KERNEL_USER_EVENT_MASK 	0x8000000000000000ULL

/* Allow 2^56 tracer references on an event. */
# define TGIF_EVENT_ENABLED_USER_MASK			0x00FFFFFFFFFFFFFFULL
#else
# define TGIF_EVENT_ENABLED_KERNEL_MASK			0xFF000000UL
# define TGIF_EVENT_ENABLED_KERNEL_USER_EVENT_MASK	0x80000000UL

/* Allow 2^24 tracer references on an event. */
# define TGIF_EVENT_ENABLED_USER_MASK			0x00FFFFFFUL
#endif

struct tgif_events_register_handle {
	struct tgif_list_node node;
	struct tgif_event_description **events;
	uint32_t nr_events;
};

struct tgif_tracer_handle {
	struct tgif_list_node node;
	void (*cb)(enum tgif_tracer_notification notif,
		struct tgif_event_description **events, uint32_t nr_events, void *priv);
	void *priv;
};

static struct tgif_rcu_gp_state rcu_gp;

/*
 * Lazy initialization for early use within library constructors.
 */
static bool initialized;
/*
 * Do not register/unregister any more events after destructor.
 */
static bool finalized;

/*
 * Recursive mutex to allow tracer callbacks to use the tgif API.
 */
static pthread_mutex_t tgif_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static DEFINE_TGIF_LIST_HEAD(tgif_events_list);
static DEFINE_TGIF_LIST_HEAD(tgif_tracer_list);

/*
 * The empty callback has a NULL function callback pointer, which stops
 * iteration on the array of callbacks immediately.
 */
const struct tgif_callback tgif_empty_callback = { };

void tgif_call(const struct tgif_event_description *desc, const struct tgif_arg_vec *tgif_arg_vec)
{
	struct tgif_rcu_read_state rcu_read_state;
	const struct tgif_callback *tgif_cb;
	uintptr_t enabled;

	if (tgif_unlikely(finalized))
		return;
	if (tgif_unlikely(!initialized))
		tgif_init();
	if (tgif_unlikely(desc->flags & TGIF_EVENT_FLAG_VARIADIC)) {
		printf("ERROR: unexpected variadic event description\n");
		abort();
	}
	enabled = __atomic_load_n(desc->enabled, __ATOMIC_RELAXED);
	if (tgif_unlikely(enabled & TGIF_EVENT_ENABLED_KERNEL_USER_EVENT_MASK)) {
		// TODO: call kernel write.
	}
	tgif_rcu_read_begin(&rcu_gp, &rcu_read_state);
	for (tgif_cb = tgif_rcu_dereference(desc->callbacks); tgif_cb->u.call != NULL; tgif_cb++)
		tgif_cb->u.call(desc, tgif_arg_vec, tgif_cb->priv);
	tgif_rcu_read_end(&rcu_gp, &rcu_read_state);
}

void tgif_call_variadic(const struct tgif_event_description *desc,
	const struct tgif_arg_vec *tgif_arg_vec,
	const struct tgif_arg_dynamic_struct *var_struct)
{
	struct tgif_rcu_read_state rcu_read_state;
	const struct tgif_callback *tgif_cb;
	uintptr_t enabled;

	if (tgif_unlikely(finalized))
		return;
	if (tgif_unlikely(!initialized))
		tgif_init();
	if (tgif_unlikely(!(desc->flags & TGIF_EVENT_FLAG_VARIADIC))) {
		printf("ERROR: unexpected non-variadic event description\n");
		abort();
	}
	enabled = __atomic_load_n(desc->enabled, __ATOMIC_RELAXED);
	if (tgif_unlikely(enabled & TGIF_EVENT_ENABLED_KERNEL_USER_EVENT_MASK)) {
		// TODO: call kernel write.
	}
	tgif_rcu_read_begin(&rcu_gp, &rcu_read_state);
	for (tgif_cb = tgif_rcu_dereference(desc->callbacks); tgif_cb->u.call_variadic != NULL; tgif_cb++)
		tgif_cb->u.call_variadic(desc, tgif_arg_vec, var_struct, tgif_cb->priv);
	tgif_rcu_read_end(&rcu_gp, &rcu_read_state);
}

static
const struct tgif_callback *tgif_tracer_callback_lookup(
		const struct tgif_event_description *desc,
		void *call, void *priv)
{
	const struct tgif_callback *cb;

	for (cb = desc->callbacks; cb->u.call != NULL; cb++) {
		if ((void *) cb->u.call == call && cb->priv == priv)
			return cb;
	}
	return NULL;
}

static
int _tgif_tracer_callback_register(struct tgif_event_description *desc,
		void *call, void *priv)
{
	struct tgif_callback *old_cb, *new_cb;
	int ret = TGIF_ERROR_OK;
	uint32_t old_nr_cb;

	if (!call)
		return TGIF_ERROR_INVAL;
	if (finalized)
		return TGIF_ERROR_EXITING;
	if (!initialized)
		tgif_init();
	pthread_mutex_lock(&tgif_lock);
	old_nr_cb = desc->nr_callbacks;
	if (old_nr_cb == UINT32_MAX) {
		ret = TGIF_ERROR_INVAL;
		goto unlock;
	}
	/* Reject duplicate (call, priv) tuples. */
	if (tgif_tracer_callback_lookup(desc, call, priv)) {
		ret = TGIF_ERROR_EXIST;
		goto unlock;
	}
	old_cb = (struct tgif_callback *) desc->callbacks;
	/* old_nr_cb + 1 (new cb) + 1 (NULL) */
	new_cb = (struct tgif_callback *) calloc(old_nr_cb + 2, sizeof(struct tgif_callback));
	if (!new_cb) {
		ret = TGIF_ERROR_NOMEM;
		goto unlock;
	}
	memcpy(new_cb, old_cb, old_nr_cb);
	if (desc->flags & TGIF_EVENT_FLAG_VARIADIC)
		new_cb[old_nr_cb].u.call_variadic =
			(tgif_tracer_callback_variadic_func) call;
	else
		new_cb[old_nr_cb].u.call =
			(tgif_tracer_callback_func) call;
	new_cb[old_nr_cb].priv = priv;
	tgif_rcu_assign_pointer(desc->callbacks, new_cb);
	tgif_rcu_wait_grace_period(&rcu_gp);
	if (old_nr_cb)
		free(old_cb);
	desc->nr_callbacks++;
	/* Increment concurrently with kernel setting the top bits. */
	if (!old_nr_cb)
		(void) __atomic_add_fetch(desc->enabled, 1, __ATOMIC_RELAXED);
unlock:
	pthread_mutex_unlock(&tgif_lock);
	return ret;
}

int tgif_tracer_callback_register(struct tgif_event_description *desc,
		tgif_tracer_callback_func call,
		void *priv)
{
	if (desc->flags & TGIF_EVENT_FLAG_VARIADIC)
		return TGIF_ERROR_INVAL;
	return _tgif_tracer_callback_register(desc, (void *) call, priv);
}

int tgif_tracer_callback_variadic_register(struct tgif_event_description *desc,
		tgif_tracer_callback_variadic_func call_variadic,
		void *priv)
{
	if (!(desc->flags & TGIF_EVENT_FLAG_VARIADIC))
		return TGIF_ERROR_INVAL;
	return _tgif_tracer_callback_register(desc, (void *) call_variadic, priv);
}

static int _tgif_tracer_callback_unregister(struct tgif_event_description *desc,
		void *call, void *priv)
{
	struct tgif_callback *old_cb, *new_cb;
	const struct tgif_callback *cb_pos;
	uint32_t pos_idx;
	int ret = TGIF_ERROR_OK;
	uint32_t old_nr_cb;

	if (!call)
		return TGIF_ERROR_INVAL;
	if (finalized)
		return TGIF_ERROR_EXITING;
	if (!initialized)
		tgif_init();
	pthread_mutex_lock(&tgif_lock);
	cb_pos = tgif_tracer_callback_lookup(desc, call, priv);
	if (!cb_pos) {
		ret = TGIF_ERROR_NOENT;
		goto unlock;
	}
	old_nr_cb = desc->nr_callbacks;
	old_cb = (struct tgif_callback *) desc->callbacks;
	if (old_nr_cb == 1) {
		new_cb = (struct tgif_callback *) &tgif_empty_callback;
	} else {
		pos_idx = cb_pos - desc->callbacks;
		/* Remove entry at pos_idx. */
		/* old_nr_cb - 1 (removed cb) + 1 (NULL) */
		new_cb = (struct tgif_callback *) calloc(old_nr_cb, sizeof(struct tgif_callback));
		if (!new_cb) {
			ret = TGIF_ERROR_NOMEM;
			goto unlock;
		}
		memcpy(new_cb, old_cb, pos_idx);
		memcpy(&new_cb[pos_idx], &old_cb[pos_idx + 1], old_nr_cb - pos_idx - 1);
	}
	tgif_rcu_assign_pointer(desc->callbacks, new_cb);
	tgif_rcu_wait_grace_period(&rcu_gp);
	free(old_cb);
	desc->nr_callbacks--;
	/* Decrement concurrently with kernel setting the top bits. */
	if (old_nr_cb == 1)
		(void) __atomic_add_fetch(desc->enabled, -1, __ATOMIC_RELAXED);
unlock:
	pthread_mutex_unlock(&tgif_lock);
	return ret;
}

int tgif_tracer_callback_unregister(struct tgif_event_description *desc,
		tgif_tracer_callback_func call,
		void *priv)
{
	if (desc->flags & TGIF_EVENT_FLAG_VARIADIC)
		return TGIF_ERROR_INVAL;
	return _tgif_tracer_callback_unregister(desc, (void *) call, priv);
}

int tgif_tracer_callback_variadic_unregister(struct tgif_event_description *desc,
		tgif_tracer_callback_variadic_func call_variadic,
		void *priv)
{
	if (!(desc->flags & TGIF_EVENT_FLAG_VARIADIC))
		return TGIF_ERROR_INVAL;
	return _tgif_tracer_callback_unregister(desc, (void *) call_variadic, priv);
}

struct tgif_events_register_handle *tgif_events_register(struct tgif_event_description **events, uint32_t nr_events)
{
	struct tgif_events_register_handle *events_handle = NULL;
	struct tgif_tracer_handle *tracer_handle;

	if (finalized)
		return NULL;
	if (!initialized)
		tgif_init();
	events_handle = (struct tgif_events_register_handle *)
			calloc(1, sizeof(struct tgif_events_register_handle));
	if (!events_handle)
		return NULL;
	events_handle->events = events;
	events_handle->nr_events = nr_events;

	pthread_mutex_lock(&tgif_lock);
	tgif_list_insert_node_tail(&tgif_events_list, &events_handle->node);
	tgif_list_for_each_entry(tracer_handle, &tgif_tracer_list, node) {
		tracer_handle->cb(TGIF_TRACER_NOTIFICATION_INSERT_EVENTS,
			events, nr_events, tracer_handle->priv);
	}
	pthread_mutex_unlock(&tgif_lock);
	//TODO: call event batch register ioctl
	return events_handle;
}

static
void tgif_event_remove_callbacks(struct tgif_event_description *desc)
{
	uint32_t nr_cb = desc->nr_callbacks;
	struct tgif_callback *old_cb;

	if (!nr_cb)
		return;
	old_cb = (struct tgif_callback *) desc->callbacks;
	(void) __atomic_add_fetch(desc->enabled, -1, __ATOMIC_RELAXED);
	/*
	 * Setting the state back to 0 cb and empty callbacks out of
	 * caution. This should not matter because instrumentation is
	 * unreachable.
	 */
	desc->nr_callbacks = 0;
	tgif_rcu_assign_pointer(desc->callbacks, &tgif_empty_callback);
	/*
	 * No need to wait for grace period because instrumentation is
	 * unreachable.
	 */
	free(old_cb);
}

/*
 * Unregister event handle. At this point, all tgif events in that
 * handle should be unreachable.
 */
void tgif_events_unregister(struct tgif_events_register_handle *events_handle)
{
	struct tgif_tracer_handle *tracer_handle;
	uint32_t i;

	if (!events_handle)
		return;
	if (finalized)
		return;
	if (!initialized)
		tgif_init();
	pthread_mutex_lock(&tgif_lock);
	tgif_list_remove_node(&events_handle->node);
	tgif_list_for_each_entry(tracer_handle, &tgif_tracer_list, node) {
		tracer_handle->cb(TGIF_TRACER_NOTIFICATION_REMOVE_EVENTS,
			events_handle->events, events_handle->nr_events,
			tracer_handle->priv);
	}
	for (i = 0; i < events_handle->nr_events; i++) {
		struct tgif_event_description *event = events_handle->events[i];

		/* Skip NULL pointers */
		if (!event)
			continue;
		tgif_event_remove_callbacks(event);
	}
	pthread_mutex_unlock(&tgif_lock);
	//TODO: call event batch unregister ioctl
	free(events_handle);
}

struct tgif_tracer_handle *tgif_tracer_event_notification_register(
		void (*cb)(enum tgif_tracer_notification notif,
			struct tgif_event_description **events, uint32_t nr_events, void *priv),
		void *priv)
{
	struct tgif_tracer_handle *tracer_handle;
	struct tgif_events_register_handle *events_handle;

	if (finalized)
		return NULL;
	if (!initialized)
		tgif_init();
	tracer_handle = (struct tgif_tracer_handle *)
				calloc(1, sizeof(struct tgif_tracer_handle));
	if (!tracer_handle)
		return NULL;
	pthread_mutex_lock(&tgif_lock);
	tracer_handle->cb = cb;
	tracer_handle->priv = priv;
	tgif_list_insert_node_tail(&tgif_tracer_list, &tracer_handle->node);
	tgif_list_for_each_entry(events_handle, &tgif_events_list, node) {
		cb(TGIF_TRACER_NOTIFICATION_INSERT_EVENTS,
			events_handle->events, events_handle->nr_events, priv);
	}
	pthread_mutex_unlock(&tgif_lock);
	return tracer_handle;
}

void tgif_tracer_event_notification_unregister(struct tgif_tracer_handle *tracer_handle)
{
	struct tgif_events_register_handle *events_handle;

	if (finalized)
		return;
	if (!initialized)
		tgif_init();
	pthread_mutex_lock(&tgif_lock);
	tgif_list_for_each_entry(events_handle, &tgif_events_list, node) {
		tracer_handle->cb(TGIF_TRACER_NOTIFICATION_REMOVE_EVENTS,
			events_handle->events, events_handle->nr_events,
			tracer_handle->priv);
	}
	tgif_list_remove_node(&tracer_handle->node);
	pthread_mutex_unlock(&tgif_lock);
}

void tgif_init(void)
{
	if (initialized)
		return;
	tgif_rcu_gp_init(&rcu_gp);
	initialized = true;
}

/*
 * tgif_exit() is executed from a library destructor. It can be called
 * explicitly at application exit as well. Concurrent tgif API use is
 * not expected at that point.
 */
void tgif_exit(void)
{
	struct tgif_events_register_handle *handle, *tmp;

	if (finalized)
		return;
	tgif_list_for_each_entry_safe(handle, tmp, &tgif_events_list, node)
		tgif_events_unregister(handle);
	tgif_rcu_gp_exit(&rcu_gp);
	finalized = true;
}
