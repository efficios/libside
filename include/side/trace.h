// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _SIDE_TRACE_H
#define _SIDE_TRACE_H

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <side/macros.h>
#include <side/endian.h>

/*
 * SIDE stands for "Specification for Instrumentation Data Encoding"
 *
 * This is an instrumentation ABI for Linux user-space, which exposes an
 * instrumentation type system and facilities allowing a kernel or
 * user-space tracer to consume user-space instrumentation.
 */

#include <side/abi/event-description.h>
#include <side/abi/event-state.h>
#include <side/abi/type-argument.h>
#include <side/api.h>

enum side_error {
	SIDE_ERROR_OK = 0,
	SIDE_ERROR_INVAL = 1,
	SIDE_ERROR_EXIST = 2,
	SIDE_ERROR_NOMEM = 3,
	SIDE_ERROR_NOENT = 4,
	SIDE_ERROR_EXITING = 5,
};

#ifdef __cplusplus
extern "C" {
#endif

struct side_tracer_handle;
struct side_statedump_request_handle;

extern const char side_empty_callback[];

void side_call(const struct side_event_state *state,
	const struct side_arg_vec *side_arg_vec);
void side_call_variadic(const struct side_event_state *state,
	const struct side_arg_vec *side_arg_vec,
	const struct side_arg_dynamic_struct *var_struct);

struct side_events_register_handle *side_events_register(struct side_event_state **events,
		uint32_t nr_events);
void side_events_unregister(struct side_events_register_handle *handle);

/*
 * Userspace tracer registration API. This allows userspace tracers to
 * register event notification callbacks to be notified of the currently
 * registered instrumentation, and to register their callbacks to
 * specific events.
 *
 * Application statedump callbacks are allowed to invoke
 * side event register/unregister(), but tracer callbacks are _not_
 * allowed to invoke statedump request notification register/unregister.
 * The latter could result in hangs across RCU grace period domains.
 */
typedef void (*side_tracer_callback_func)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			void *priv, void *caller_addr);
typedef void (*side_tracer_callback_variadic_func)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_struct *var_struct,
			void *priv, void *caller_addr);

int side_tracer_request_key(uint64_t *key);

int side_tracer_callback_register(struct side_event_state *state,
		side_tracer_callback_func call,
		void *priv, uint64_t key);
int side_tracer_callback_variadic_register(struct side_event_state *state,
		side_tracer_callback_variadic_func call_variadic,
		void *priv, uint64_t key);
int side_tracer_callback_unregister(struct side_event_state *state,
		side_tracer_callback_func call,
		void *priv, uint64_t key);
int side_tracer_callback_variadic_unregister(struct side_event_state *state,
		side_tracer_callback_variadic_func call_variadic,
		void *priv, uint64_t key);

/*
 * Deferred registration and unregistration of tracer callbacks.
 *
 * The register/unregister APIs above wait for a grace period before
 * returning, which is expensive when a tracer registers or unregisters
 * callbacks for many events in a batch (e.g. when a session is created
 * or destroyed). The _defer variants take the effect of their
 * non-deferred counterpart, but hand over the memory they replace to
 * the side library rather than waiting.
 *
 * The protocol is:
 *
 *   for each event:
 *           side_tracer_callback_{,variadic_}{register,unregister}_defer()
 *   side_tracer_callback_synchronize()
 *   side_tracer_callback_reclaim()
 *
 * The batch takes effect as each deferred call returns, exactly like
 * the non-deferred APIs: only the memory reclaim is postponed. A batch
 * can therefore mix deferred registrations and unregistrations, and can
 * span many events, with a single grace period for the whole batch.
 */
int side_tracer_callback_register_defer(struct side_event_state *state,
		side_tracer_callback_func call,
		void *priv, uint64_t key);
int side_tracer_callback_variadic_register_defer(struct side_event_state *state,
		side_tracer_callback_variadic_func call_variadic,
		void *priv, uint64_t key);
int side_tracer_callback_unregister_defer(struct side_event_state *state,
		side_tracer_callback_func call,
		void *priv, uint64_t key);
int side_tracer_callback_variadic_unregister_defer(struct side_event_state *state,
		side_tracer_callback_variadic_func call_variadic,
		void *priv, uint64_t key);

/*
 * Wait for a grace period of the domain within which the tracer
 * callbacks are invoked. Upon return, all tracer callbacks which had
 * begun before the call have completed.
 *
 * Tracers use this to reclaim memory which their callbacks can
 * observe, in cases where the memory is not unpublished by a callback
 * unregistration (which waits for a grace period on its own).
 * Tracers which have their own grace period domain, used for instance
 * by other instrumentation mechanisms, need to wait for both.
 *
 * This also readies for reclaim the memory left over by the deferred
 * register/unregister APIs which have returned before the call.
 *
 * Must not be invoked from a tracer callback: it would wait for the
 * completion of the callback performing the call.
 */
void side_tracer_callback_synchronize(void);

/*
 * Free the memory left over by the deferred register/unregister APIs
 * which is ready for reclaim.
 *
 * The caller is responsible for issuing a
 * side_tracer_callback_synchronize() after the deferred calls of a
 * batch and before reclaiming it. Memory deferred after the last
 * synchronize stays pending, and is reclaimed by a later
 * synchronize/reclaim pair.
 */
void side_tracer_callback_reclaim(void);

enum side_tracer_notification {
	SIDE_TRACER_NOTIFICATION_INSERT_EVENTS,
	SIDE_TRACER_NOTIFICATION_REMOVE_EVENTS,
};

/* Callback is invoked with side library internal lock held. */
struct side_tracer_handle *side_tracer_event_notification_register(
		void (*cb)(enum side_tracer_notification notif,
			struct side_event_state **events, uint32_t nr_events, void *priv),
		void *priv);
void side_tracer_event_notification_unregister(struct side_tracer_handle *handle);

/*
 * The side_statedump_call APIs should be used for application/library
 * state dump.
 * The statedump callback dumps application state to tracers by invoking
 * side_statedump_call APIs.
 * The statedump callback should not invoke libside statedump request
 * notification register/unregister APIs.
 */
void side_statedump_call(const struct side_event_state *state,
		const struct side_arg_vec *side_arg_vec,
		void *statedump_request_key);
void side_statedump_call_variadic(const struct side_event_state *state,
		const struct side_arg_vec *side_arg_vec,
		const struct side_arg_dynamic_struct *var_struct,
		void *statedump_request_key);

/*
 * If side_statedump_request_notification_register is invoked from
 * library constructors and side_statedump_request_notification_unregister
 * from library destructors, make sure to:
 * - invoke side_event_state_ptr_init before registration of the
 *   callback,
 * - invoke side_event_state_ptr_exit after unregistration of the
 *   callback.
 *
 * In "polling" state dump mode, the application or library is responsible
 * for periodically invoking side_statedump_run_pending_requests(). This
 * mechanism is well-suited for single-threaded event-loop driven
 * applications which do not wish to introduce multithreading nor
 * locking-based synchronization of their state.
 *
 * In "agent thread" state dump mode, libside spawns a helper agent
 * thread which is responsible for invoking the state dump callbacks
 * when requested by the tracers. This mechanism is well-suited for
 * instrumentation of multi-threaded applications which rely on
 * locking to synchronize their data structures across threads, and
 * for libraries which have no control on application event loops.
 *
 * Applications using fork/clone with locks held should not take those
 * locks (or block on any resource that depend on these locks) within
 * their statedump callbacks registered with the agent thread. This
 * could result in deadlocks when pthread_atfork handler waits for
 * agent thread quiescence.
 *
 * The statedump_request_key received by the statedump_cb is only
 * valid until the statedump_cb returns.
 */
enum side_statedump_mode {
	SIDE_STATEDUMP_MODE_POLLING,
	SIDE_STATEDUMP_MODE_AGENT_THREAD,
};

struct side_statedump_request_handle *
	side_statedump_request_notification_register(
		const char *state_name,
		void (*statedump_cb)(void *statedump_request_key),
		enum side_statedump_mode mode);
void side_statedump_request_notification_unregister(
		struct side_statedump_request_handle *handle);

/* Returns true if the handle has pending statedump requests. */
bool side_statedump_poll_pending_requests(struct side_statedump_request_handle *handle);
int side_statedump_run_pending_requests(struct side_statedump_request_handle *handle);

/*
 * Request a state dump for tracer callbacks identified with "key".
 */
int side_tracer_statedump_request(uint64_t key);
/*
 * Cancel a statedump request.
 */
int side_tracer_statedump_request_cancel(uint64_t key);

/*
 * Explicit hooks to initialize/finalize the side instrumentation
 * library. Those are also library constructor/destructor.
 */
void side_init(void) __attribute__((constructor));
void side_exit(void) __attribute__((destructor));

/*
 * The following constructors/destructors perform automatic registration
 * of the declared side events. Those may have to be called explicitly
 * in a statically linked library.
 */

/*
 * These weak symbols, the constructor, and destructor take care of
 * registering only _one_ instance of the side instrumentation per
 * shared-object (or for the whole main program).
 */
extern struct side_event_state * __start_side_event_state_ptr[]
	__attribute__((weak, visibility("hidden")));
extern struct side_event_state * __stop_side_event_state_ptr[]
	__attribute__((weak, visibility("hidden")));
int side_event_state_ptr_registered
        __attribute__((weak, visibility("hidden")));
struct side_events_register_handle *side_events_handle
	__attribute__((weak, visibility("hidden")));

static void
side_event_state_ptr_init(void)
	__attribute__((no_instrument_function))
	__attribute__((constructor));
static void
side_event_state_ptr_init(void)
{
	if (side_event_state_ptr_registered++)
		return;
	side_events_handle = side_events_register(__start_side_event_state_ptr,
		__stop_side_event_state_ptr - __start_side_event_state_ptr);
}

static void
side_event_state_ptr_exit(void)
	__attribute__((no_instrument_function))
	__attribute__((destructor));
static void
side_event_state_ptr_exit(void)
{
	if (--side_event_state_ptr_registered)
		return;
	side_events_unregister(side_events_handle);
	side_events_handle = NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* _SIDE_TRACE_H */
