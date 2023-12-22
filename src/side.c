// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <side/trace.h>
#include <string.h>
#include <assert.h>

#include "rcu.h"
#include "list.h"
#include "rculist.h"

/* Top 8 bits reserved for shared tracer use. */
#if SIDE_BITS_PER_LONG == 64
# define SIDE_EVENT_ENABLED_SHARED_MASK			0xFF00000000000000ULL
# define SIDE_EVENT_ENABLED_SHARED_USER_EVENT_MASK 	0x8000000000000000ULL
# define SIDE_EVENT_ENABLED_SHARED_PTRACE_MASK 		0x4000000000000000ULL

/* Allow 2^56 private tracer references on an event. */
# define SIDE_EVENT_ENABLED_PRIVATE_MASK		0x00FFFFFFFFFFFFFFULL
#else
# define SIDE_EVENT_ENABLED_SHARED_MASK			0xFF000000UL
# define SIDE_EVENT_ENABLED_SHARED_USER_EVENT_MASK	0x80000000UL
# define SIDE_EVENT_ENABLED_SHARED_PTRACE_MASK		0x40000000UL

/* Allow 2^24 private tracer references on an event. */
# define SIDE_EVENT_ENABLED_PRIVATE_MASK		0x00FFFFFFUL
#endif

#define SIDE_KEY_RESERVED_RANGE_END			0x8

/* Key 0x0 is reserved to match all. */
#define SIDE_KEY_MATCH_ALL				0x0
/* Key 0x1 is reserved for user event. */
#define SIDE_KEY_USER_EVENT				0x1
/* Key 0x2 is reserved for ptrace. */
#define SIDE_KEY_PTRACE					0x2

struct side_events_register_handle {
	struct side_list_node node;
	struct side_event_description **events;
	uint32_t nr_events;
};

struct side_tracer_handle {
	struct side_list_node node;
	void (*cb)(enum side_tracer_notification notif,
		struct side_event_description **events, uint32_t nr_events, void *priv);
	void *priv;
};

struct side_statedump_notification {
	struct side_list_node node;
	uint64_t key;
};

struct side_statedump_request_handle {
	struct side_list_node node;			/* Statedump request RCU list node. */
	struct side_list_head notification_queue;	/* Queue of struct side_statedump_notification */
	void (*cb)(void);
	char *name;
	enum side_statedump_mode mode;
};

struct side_callback {
	union {
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			void *priv);
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_struct *var_struct,
			void *priv);
	} u;
	void *priv;
	uint64_t key;
};

static struct side_rcu_gp_state event_rcu_gp, statedump_rcu_gp;

/*
 * Lazy initialization for early use within library constructors.
 */
static bool initialized;
/*
 * Do not register/unregister any more events after destructor.
 */
static bool finalized;

/*
 * Recursive mutex to allow tracer callbacks to use the side API.
 */
static pthread_mutex_t side_event_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t side_statedump_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t side_key_lock = PTHREAD_MUTEX_INITIALIZER;

/* Dynamic tracer key allocation. */
static uint64_t side_key_next = SIDE_KEY_RESERVED_RANGE_END;

static DEFINE_SIDE_LIST_HEAD(side_events_list);
static DEFINE_SIDE_LIST_HEAD(side_tracer_list);

/*
 * The statedump request list is a RCU list to allow the agent thread to
 * iterate over this list with a RCU read-side lock.
 */
static DEFINE_SIDE_LIST_HEAD(side_statedump_list);

/*
 * Callback filter key for state dump.
 */
static __thread uint64_t filter_key = SIDE_KEY_MATCH_ALL;

/*
 * The empty callback has a NULL function callback pointer, which stops
 * iteration on the array of callbacks immediately.
 */
const char side_empty_callback[sizeof(struct side_callback)];

side_static_event(side_statedump_begin, "side", "statedump_begin",
	SIDE_LOGLEVEL_INFO, side_field_list(side_field_string("name")));
side_static_event(side_statedump_end, "side", "statedump_end",
	SIDE_LOGLEVEL_INFO, side_field_list(side_field_string("name")));

/*
 * side_ptrace_hook is a place holder for a debugger breakpoint.
 * var_struct is NULL if not variadic.
 */
void side_ptrace_hook(const struct side_event_state *event_state __attribute__((unused)),
	const struct side_arg_vec *side_arg_vec __attribute__((unused)),
	const struct side_arg_dynamic_struct *var_struct __attribute__((unused)))
	__attribute__((noinline));
void side_ptrace_hook(const struct side_event_state *event_state __attribute__((unused)),
	const struct side_arg_vec *side_arg_vec __attribute__((unused)),
	const struct side_arg_dynamic_struct *var_struct __attribute__((unused)))
{
}

static
void _side_call(const struct side_event_state *event_state, const struct side_arg_vec *side_arg_vec, uint64_t key)
{
	struct side_rcu_read_state rcu_read_state;
	const struct side_event_state_0 *es0;
	const struct side_callback *side_cb;
	uintptr_t enabled;

	if (side_unlikely(finalized))
		return;
	if (side_unlikely(!initialized))
		side_init();
	if (side_unlikely(event_state->version != 0))
		abort();
	es0 = side_container_of(event_state, const struct side_event_state_0, parent);
	assert(!(es0->desc->flags & SIDE_EVENT_FLAG_VARIADIC));
	enabled = __atomic_load_n(&es0->enabled, __ATOMIC_RELAXED);
	if (side_unlikely(enabled & SIDE_EVENT_ENABLED_SHARED_MASK)) {
		if ((enabled & SIDE_EVENT_ENABLED_SHARED_USER_EVENT_MASK) &&
		    (key == SIDE_KEY_MATCH_ALL || key == SIDE_KEY_USER_EVENT)) {
			// TODO: call kernel write.
		}
		if ((enabled & SIDE_EVENT_ENABLED_SHARED_PTRACE_MASK) &&
		    (key == SIDE_KEY_MATCH_ALL || key == SIDE_KEY_PTRACE))
			side_ptrace_hook(event_state, side_arg_vec, NULL);
	}
	side_rcu_read_begin(&event_rcu_gp, &rcu_read_state);
	for (side_cb = side_rcu_dereference(es0->callbacks); side_cb->u.call != NULL; side_cb++) {
		if (key != SIDE_KEY_MATCH_ALL && side_cb->key != SIDE_KEY_MATCH_ALL && side_cb->key != key)
			continue;
		side_cb->u.call(es0->desc, side_arg_vec, side_cb->priv);
	}
	side_rcu_read_end(&event_rcu_gp, &rcu_read_state);
}

void side_call(const struct side_event_state *event_state, const struct side_arg_vec *side_arg_vec)
{
	_side_call(event_state, side_arg_vec, SIDE_KEY_MATCH_ALL);
}

void side_statedump_call(const struct side_event_state *event_state, const struct side_arg_vec *side_arg_vec)
{
	_side_call(event_state, side_arg_vec, filter_key);
}

static
void _side_call_variadic(const struct side_event_state *event_state,
	const struct side_arg_vec *side_arg_vec,
	const struct side_arg_dynamic_struct *var_struct,
	uint64_t key)
{
	struct side_rcu_read_state rcu_read_state;
	const struct side_event_state_0 *es0;
	const struct side_callback *side_cb;
	uintptr_t enabled;

	if (side_unlikely(finalized))
		return;
	if (side_unlikely(!initialized))
		side_init();
	if (side_unlikely(event_state->version != 0))
		abort();
	es0 = side_container_of(event_state, const struct side_event_state_0, parent);
	assert(es0->desc->flags & SIDE_EVENT_FLAG_VARIADIC);
	enabled = __atomic_load_n(&es0->enabled, __ATOMIC_RELAXED);
	if (side_unlikely(enabled & SIDE_EVENT_ENABLED_SHARED_MASK)) {
		if ((enabled & SIDE_EVENT_ENABLED_SHARED_USER_EVENT_MASK) &&
		    (key == SIDE_KEY_MATCH_ALL || key == SIDE_KEY_USER_EVENT)) {
			// TODO: call kernel write.
		}
		if ((enabled & SIDE_EVENT_ENABLED_SHARED_PTRACE_MASK) &&
		    (key == SIDE_KEY_MATCH_ALL || key == SIDE_KEY_PTRACE))
			side_ptrace_hook(event_state, side_arg_vec, var_struct);
	}
	side_rcu_read_begin(&event_rcu_gp, &rcu_read_state);
	for (side_cb = side_rcu_dereference(es0->callbacks); side_cb->u.call_variadic != NULL; side_cb++) {
		if (key != SIDE_KEY_MATCH_ALL && side_cb->key != SIDE_KEY_MATCH_ALL && side_cb->key != key)
			continue;
		side_cb->u.call_variadic(es0->desc, side_arg_vec, var_struct, side_cb->priv);
	}
	side_rcu_read_end(&event_rcu_gp, &rcu_read_state);
}

void side_call_variadic(const struct side_event_state *event_state,
	const struct side_arg_vec *side_arg_vec,
	const struct side_arg_dynamic_struct *var_struct)
{
	_side_call_variadic(event_state, side_arg_vec, var_struct, SIDE_KEY_MATCH_ALL);
}

void side_statedump_call_variadic(const struct side_event_state *event_state,
	const struct side_arg_vec *side_arg_vec,
	const struct side_arg_dynamic_struct *var_struct)
{
	_side_call_variadic(event_state, side_arg_vec, var_struct, filter_key);
}

static
const struct side_callback *side_tracer_callback_lookup(
		const struct side_event_description *desc,
		void *call, void *priv, uint64_t key)
{
	struct side_event_state *event_state = side_ptr_get(desc->state);
	const struct side_event_state_0 *es0;
	const struct side_callback *cb;

	if (side_unlikely(event_state->version != 0))
		abort();
	es0 = side_container_of(event_state, const struct side_event_state_0, parent);
	for (cb = es0->callbacks; cb->u.call != NULL; cb++) {
		if ((void *) cb->u.call == call && cb->priv == priv && cb->key == key)
			return cb;
	}
	return NULL;
}

static
int _side_tracer_callback_register(struct side_event_description *desc,
		void *call, void *priv, uint64_t key)
{
	struct side_event_state *event_state;
	struct side_callback *old_cb, *new_cb;
	struct side_event_state_0 *es0;
	int ret = SIDE_ERROR_OK;
	uint32_t old_nr_cb;

	if (!call)
		return SIDE_ERROR_INVAL;
	if (finalized)
		return SIDE_ERROR_EXITING;
	if (!initialized)
		side_init();
	pthread_mutex_lock(&side_event_lock);
	event_state = side_ptr_get(desc->state);
	if (side_unlikely(event_state->version != 0))
		abort();
	es0 = side_container_of(event_state, struct side_event_state_0, parent);
	old_nr_cb = es0->nr_callbacks;
	if (old_nr_cb == UINT32_MAX) {
		ret = SIDE_ERROR_INVAL;
		goto unlock;
	}
	/* Reject duplicate (call, priv) tuples. */
	if (side_tracer_callback_lookup(desc, call, priv, key)) {
		ret = SIDE_ERROR_EXIST;
		goto unlock;
	}
	old_cb = (struct side_callback *) es0->callbacks;
	/* old_nr_cb + 1 (new cb) + 1 (NULL) */
	new_cb = (struct side_callback *) calloc(old_nr_cb + 2, sizeof(struct side_callback));
	if (!new_cb) {
		ret = SIDE_ERROR_NOMEM;
		goto unlock;
	}
	memcpy(new_cb, old_cb, old_nr_cb);
	if (desc->flags & SIDE_EVENT_FLAG_VARIADIC)
		new_cb[old_nr_cb].u.call_variadic =
			(side_tracer_callback_variadic_func) call;
	else
		new_cb[old_nr_cb].u.call =
			(side_tracer_callback_func) call;
	new_cb[old_nr_cb].priv = priv;
	new_cb[old_nr_cb].key = key;
	/* High order bits are already zeroed. */
	side_rcu_assign_pointer(es0->callbacks, new_cb);
	side_rcu_wait_grace_period(&event_rcu_gp);
	if (old_nr_cb)
		free(old_cb);
	es0->nr_callbacks++;
	/* Increment concurrently with kernel setting the top bits. */
	if (!old_nr_cb)
		(void) __atomic_add_fetch(&es0->enabled, 1, __ATOMIC_RELAXED);
unlock:
	pthread_mutex_unlock(&side_event_lock);
	return ret;
}

int side_tracer_callback_register(struct side_event_description *desc,
		side_tracer_callback_func call,
		void *priv, uint64_t key)
{
	if (desc->flags & SIDE_EVENT_FLAG_VARIADIC)
		return SIDE_ERROR_INVAL;
	return _side_tracer_callback_register(desc, (void *) call, priv, key);
}

int side_tracer_callback_variadic_register(struct side_event_description *desc,
		side_tracer_callback_variadic_func call_variadic,
		void *priv, uint64_t key)
{
	if (!(desc->flags & SIDE_EVENT_FLAG_VARIADIC))
		return SIDE_ERROR_INVAL;
	return _side_tracer_callback_register(desc, (void *) call_variadic, priv, key);
}

static int _side_tracer_callback_unregister(struct side_event_description *desc,
		void *call, void *priv, uint64_t key)
{
	struct side_event_state *event_state;
	struct side_callback *old_cb, *new_cb;
	const struct side_callback *cb_pos;
	struct side_event_state_0 *es0;
	uint32_t pos_idx;
	int ret = SIDE_ERROR_OK;
	uint32_t old_nr_cb;

	if (!call)
		return SIDE_ERROR_INVAL;
	if (finalized)
		return SIDE_ERROR_EXITING;
	if (!initialized)
		side_init();
	pthread_mutex_lock(&side_event_lock);
	event_state = side_ptr_get(desc->state);
	if (side_unlikely(event_state->version != 0))
		abort();
	es0 = side_container_of(event_state, struct side_event_state_0, parent);
	cb_pos = side_tracer_callback_lookup(desc, call, priv, key);
	if (!cb_pos) {
		ret = SIDE_ERROR_NOENT;
		goto unlock;
	}
	old_nr_cb = es0->nr_callbacks;
	old_cb = (struct side_callback *) es0->callbacks;
	if (old_nr_cb == 1) {
		new_cb = (struct side_callback *) &side_empty_callback;
	} else {
		pos_idx = cb_pos - es0->callbacks;
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
	/* High order bits are already zeroed. */
	side_rcu_assign_pointer(es0->callbacks, new_cb);
	side_rcu_wait_grace_period(&event_rcu_gp);
	free(old_cb);
	es0->nr_callbacks--;
	/* Decrement concurrently with kernel setting the top bits. */
	if (old_nr_cb == 1)
		(void) __atomic_add_fetch(&es0->enabled, -1, __ATOMIC_RELAXED);
unlock:
	pthread_mutex_unlock(&side_event_lock);
	return ret;
}

int side_tracer_callback_unregister(struct side_event_description *desc,
		side_tracer_callback_func call,
		void *priv, uint64_t key)
{
	if (desc->flags & SIDE_EVENT_FLAG_VARIADIC)
		return SIDE_ERROR_INVAL;
	return _side_tracer_callback_unregister(desc, (void *) call, priv, key);
}

int side_tracer_callback_variadic_unregister(struct side_event_description *desc,
		side_tracer_callback_variadic_func call_variadic,
		void *priv, uint64_t key)
{
	if (!(desc->flags & SIDE_EVENT_FLAG_VARIADIC))
		return SIDE_ERROR_INVAL;
	return _side_tracer_callback_unregister(desc, (void *) call_variadic, priv, key);
}

struct side_events_register_handle *side_events_register(struct side_event_description **events, uint32_t nr_events)
{
	struct side_events_register_handle *events_handle = NULL;
	struct side_tracer_handle *tracer_handle;

	if (finalized)
		return NULL;
	if (!initialized)
		side_init();
	events_handle = (struct side_events_register_handle *)
			calloc(1, sizeof(struct side_events_register_handle));
	if (!events_handle)
		return NULL;
	events_handle->events = events;
	events_handle->nr_events = nr_events;

	pthread_mutex_lock(&side_event_lock);
	side_list_insert_node_tail(&side_events_list, &events_handle->node);
	side_list_for_each_entry(tracer_handle, &side_tracer_list, node) {
		tracer_handle->cb(SIDE_TRACER_NOTIFICATION_INSERT_EVENTS,
			events, nr_events, tracer_handle->priv);
	}
	pthread_mutex_unlock(&side_event_lock);
	//TODO: call event batch register ioctl
	return events_handle;
}

static
void side_event_remove_callbacks(struct side_event_description *desc)
{
	struct side_event_state *event_state = side_ptr_get(desc->state);
	struct side_event_state_0 *es0;
	struct side_callback *old_cb;
	uint32_t nr_cb;

	if (side_unlikely(event_state->version != 0))
		abort();
	es0 = side_container_of(event_state, struct side_event_state_0, parent);
	nr_cb = es0->nr_callbacks;
	if (!nr_cb)
		return;
	old_cb = (struct side_callback *) es0->callbacks;
	(void) __atomic_add_fetch(&es0->enabled, -1, __ATOMIC_RELAXED);
	/*
	 * Setting the state back to 0 cb and empty callbacks out of
	 * caution. This should not matter because instrumentation is
	 * unreachable.
	 */
	es0->nr_callbacks = 0;
	side_rcu_assign_pointer(es0->callbacks, &side_empty_callback);
	/*
	 * No need to wait for grace period because instrumentation is
	 * unreachable.
	 */
	free(old_cb);
}

/*
 * Unregister event handle. At this point, all side events in that
 * handle should be unreachable.
 */
void side_events_unregister(struct side_events_register_handle *events_handle)
{
	struct side_tracer_handle *tracer_handle;
	uint32_t i;

	if (!events_handle)
		return;
	if (finalized)
		return;
	if (!initialized)
		side_init();
	pthread_mutex_lock(&side_event_lock);
	side_list_remove_node(&events_handle->node);
	side_list_for_each_entry(tracer_handle, &side_tracer_list, node) {
		tracer_handle->cb(SIDE_TRACER_NOTIFICATION_REMOVE_EVENTS,
			events_handle->events, events_handle->nr_events,
			tracer_handle->priv);
	}
	for (i = 0; i < events_handle->nr_events; i++) {
		struct side_event_description *event = events_handle->events[i];

		/* Skip NULL pointers */
		if (!event)
			continue;
		side_event_remove_callbacks(event);
	}
	pthread_mutex_unlock(&side_event_lock);
	//TODO: call event batch unregister ioctl
	free(events_handle);
}

struct side_tracer_handle *side_tracer_event_notification_register(
		void (*cb)(enum side_tracer_notification notif,
			struct side_event_description **events, uint32_t nr_events, void *priv),
		void *priv)
{
	struct side_tracer_handle *tracer_handle;
	struct side_events_register_handle *events_handle;

	if (finalized)
		return NULL;
	if (!initialized)
		side_init();
	tracer_handle = (struct side_tracer_handle *)
				calloc(1, sizeof(struct side_tracer_handle));
	if (!tracer_handle)
		return NULL;
	pthread_mutex_lock(&side_event_lock);
	tracer_handle->cb = cb;
	tracer_handle->priv = priv;
	side_list_insert_node_tail(&side_tracer_list, &tracer_handle->node);
	side_list_for_each_entry(events_handle, &side_events_list, node) {
		cb(SIDE_TRACER_NOTIFICATION_INSERT_EVENTS,
			events_handle->events, events_handle->nr_events, priv);
	}
	pthread_mutex_unlock(&side_event_lock);
	return tracer_handle;
}

void side_tracer_event_notification_unregister(struct side_tracer_handle *tracer_handle)
{
	struct side_events_register_handle *events_handle;

	if (finalized)
		return;
	if (!initialized)
		side_init();
	pthread_mutex_lock(&side_event_lock);
	side_list_for_each_entry(events_handle, &side_events_list, node) {
		tracer_handle->cb(SIDE_TRACER_NOTIFICATION_REMOVE_EVENTS,
			events_handle->events, events_handle->nr_events,
			tracer_handle->priv);
	}
	side_list_remove_node(&tracer_handle->node);
	pthread_mutex_unlock(&side_event_lock);
	free(tracer_handle);
}

/* Called with side_statedump_lock held. */
static
void queue_statedump_pending(struct side_statedump_request_handle *handle, uint64_t key)
{
	struct side_statedump_notification *notif;

	notif = (struct side_statedump_notification *) calloc(1, sizeof(struct side_statedump_notification));
	if (!notif)
		abort();
	notif->key = key;
	side_list_insert_node_tail(&handle->notification_queue, &notif->node);
}

/* Called with side_statedump_lock held. */
static
void unqueue_statedump_pending(struct side_statedump_request_handle *handle, uint64_t key)
{
	struct side_statedump_notification *notif, *tmp;

	side_list_for_each_entry_safe(notif, tmp, &handle->notification_queue, node) {
		if (key == SIDE_KEY_MATCH_ALL || key == notif->key) {
			side_list_remove_node(&notif->node);
			free(notif);
		}
	}
}

struct side_statedump_request_handle *
	side_statedump_request_notification_register(const char *state_name,
		void (*statedump_cb)(void),
		enum side_statedump_mode mode)
{
	struct side_statedump_request_handle *handle;
	char *name;

	if (finalized)
		return NULL;
	if (!initialized)
		side_init();
	/*
	 * The statedump request notification should not be registered
	 * from a notification callback.
	 */
	assert(!filter_key);
	handle = (struct side_statedump_request_handle *)
				calloc(1, sizeof(struct side_statedump_request_handle));
	if (!handle)
		return NULL;
	name = strdup(state_name);
	if (!name)
		goto name_nomem;
	handle->cb = statedump_cb;
	handle->name = name;
	handle->mode = mode;
	side_list_head_init(&handle->notification_queue);

	pthread_mutex_lock(&side_statedump_lock);
	side_list_insert_node_tail_rcu(&side_statedump_list, &handle->node);
	/* Queue statedump pending for all tracers. */
	queue_statedump_pending(handle, SIDE_KEY_MATCH_ALL);
	pthread_mutex_unlock(&side_statedump_lock);

	return handle;

name_nomem:
	free(handle);
	return NULL;
}

void side_statedump_request_notification_unregister(struct side_statedump_request_handle *handle)
{
	if (finalized)
		return;
	if (!initialized)
		side_init();
	assert(!filter_key);

	pthread_mutex_lock(&side_statedump_lock);
	unqueue_statedump_pending(handle, SIDE_KEY_MATCH_ALL);
	side_list_remove_node_rcu(&handle->node);
	pthread_mutex_unlock(&side_statedump_lock);

	side_rcu_wait_grace_period(&statedump_rcu_gp);
	free(handle->name);
	free(handle);
}

/* Returns true if the handle has pending statedump requests. */
bool side_statedump_poll_pending_requests(struct side_statedump_request_handle *handle)
{
	bool ret;

	if (handle->mode != SIDE_STATEDUMP_MODE_POLLING)
		return false;
	pthread_mutex_lock(&side_statedump_lock);
	ret = !side_list_empty(&handle->notification_queue);
	pthread_mutex_unlock(&side_statedump_lock);
	return ret;
}

static
void side_statedump_run(struct side_statedump_request_handle *handle,
		struct side_statedump_notification *notif)
{
	/* Invoke the state dump callback specifically for the tracer key. */
	filter_key = notif->key;
	side_statedump_event_call(side_statedump_begin,
		side_arg_list(side_arg_string(handle->name)));
	handle->cb();
	side_statedump_event_call(side_statedump_end,
		side_arg_list(side_arg_string(handle->name)));
	filter_key = SIDE_KEY_MATCH_ALL;
}

static
void _side_statedump_run_pending_requests(struct side_statedump_request_handle *handle)
{
	struct side_statedump_notification *notif, *tmp;
	DEFINE_SIDE_LIST_HEAD(tmp_head);

	pthread_mutex_lock(&side_statedump_lock);
	side_list_splice(&handle->notification_queue, &tmp_head);
	pthread_mutex_lock(&side_statedump_lock);

	/* We are now sole owner of the tmp_head list. */
	side_list_for_each_entry(notif, &tmp_head, node)
		side_statedump_run(handle, notif);
	side_list_for_each_entry_safe(notif, tmp, &tmp_head, node)
		free(notif);
}

/*
 * Only polling mode state dump handles allow application to explicitly handle the
 * pending requests.
 */
int side_statedump_run_pending_requests(struct side_statedump_request_handle *handle)
{
	if (handle->mode != SIDE_STATEDUMP_MODE_POLLING)
		return SIDE_ERROR_INVAL;
	_side_statedump_run_pending_requests(handle);
	return SIDE_ERROR_OK;
}

/*
 * Request a state dump for tracer callbacks identified with "key".
 */
int side_tracer_statedump_request(uint64_t key)
{
	struct side_statedump_request_handle *handle;

	if (key == SIDE_KEY_MATCH_ALL)
		return SIDE_ERROR_INVAL;
	pthread_mutex_lock(&side_statedump_lock);
	side_list_for_each_entry(handle, &side_statedump_list, node)
		queue_statedump_pending(handle, key);
	pthread_mutex_lock(&side_statedump_lock);
	return SIDE_ERROR_OK;
}

/*
 * Cancel a statedump request.
 */
int side_tracer_statedump_request_cancel(uint64_t key)
{
	struct side_statedump_request_handle *handle;

	if (key == SIDE_KEY_MATCH_ALL)
		return SIDE_ERROR_INVAL;
	pthread_mutex_lock(&side_statedump_lock);
	side_list_for_each_entry(handle, &side_statedump_list, node)
		unqueue_statedump_pending(handle, key);
	pthread_mutex_lock(&side_statedump_lock);
	return SIDE_ERROR_OK;
}

/*
 * Tracer keys are represented on 64-bit. Return SIDE_ERROR_NOMEM on
 * overflow (which should never happen in practice).
 */
int side_tracer_request_key(uint64_t *key)
{
	int ret = SIDE_ERROR_OK;

	pthread_mutex_lock(&side_key_lock);
	if (side_key_next == 0) {
		ret = SIDE_ERROR_NOMEM;
		goto end;
	}
	*key = side_key_next++;
end:
	pthread_mutex_unlock(&side_key_lock);
	return ret;
}

void side_init(void)
{
	if (initialized)
		return;
	side_rcu_gp_init(&event_rcu_gp);
	side_rcu_gp_init(&statedump_rcu_gp);
	initialized = true;
}

/*
 * side_exit() is executed from a library destructor. It can be called
 * explicitly at application exit as well. Concurrent side API use is
 * not expected at that point.
 */
void side_exit(void)
{
	struct side_events_register_handle *handle, *tmp;

	if (finalized)
		return;
	side_list_for_each_entry_safe(handle, tmp, &side_events_list, node)
		side_events_unregister(handle);
	side_rcu_gp_exit(&event_rcu_gp);
	side_rcu_gp_exit(&statedump_rcu_gp);
	finalized = true;
}
