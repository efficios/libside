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
 * SIDE stands for "Software Instrumentation Dynamically Enabled"
 *
 * This is an instrumentation ABI for Linux user-space, which exposes an
 * instrumentation type system and facilities allowing a kernel or
 * user-space tracer to consume user-space instrumentation.
 *
 * The extensibility scheme for the SIDE ABI for event state is as
 * follows:
 *
 * * If the semantic of the "struct side_event_state_N" fields change,
 *   the SIDE_EVENT_STATE_ABI_VERSION should be increased. The
 *   "struct side_event_state_N" is not extensible and must have its
 *   ABI version increased whenever it is changed. Note that increasing
 *   the version of SIDE_EVENT_DESCRIPTION_ABI_VERSION is not necessary
 *   when changing the layout of "struct side_event_state_N".
 */

#define SIDE_EVENT_STATE_ABI_VERSION		0

#include <side/abi/event-description.h>
#include <side/abi/type-argument.h>
#include <side/instrumentation-c-api.h>

enum side_error {
	SIDE_ERROR_OK = 0,
	SIDE_ERROR_INVAL = 1,
	SIDE_ERROR_EXIST = 2,
	SIDE_ERROR_NOMEM = 3,
	SIDE_ERROR_NOENT = 4,
	SIDE_ERROR_EXITING = 5,
};

/*
 * This structure is _not_ packed to allow atomic operations on its
 * fields. Changes to this structure must bump the "Event state ABI
 * version" and tracers _must_ learn how to deal with this ABI,
 * otherwise they should reject the event.
 */

struct side_event_state {
	uint32_t version;	/* Event state ABI version. */
};

struct side_event_state_0 {
	struct side_event_state p;	/* Required first field. */
	uint32_t enabled;
	const struct side_callback *callbacks;
	struct side_event_description *desc;
};

#ifdef __cplusplus
extern "C" {
#endif

struct side_callback {
	union {
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			void *priv);
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_struct *var_struct,
			void *priv);
	} SIDE_PACKED u;
	void *priv;
} SIDE_PACKED;

extern const struct side_callback side_empty_callback;

void side_call(const struct side_event_state *state,
	const struct side_arg_vec *side_arg_vec);
void side_call_variadic(const struct side_event_state *state,
	const struct side_arg_vec *side_arg_vec,
	const struct side_arg_dynamic_struct *var_struct);

struct side_events_register_handle *side_events_register(struct side_event_description **events,
		uint32_t nr_events);
void side_events_unregister(struct side_events_register_handle *handle);

/*
 * Userspace tracer registration API. This allows userspace tracers to
 * register event notification callbacks to be notified of the currently
 * registered instrumentation, and to register their callbacks to
 * specific events.
 */
typedef void (*side_tracer_callback_func)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			void *priv);
typedef void (*side_tracer_callback_variadic_func)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_struct *var_struct,
			void *priv);

int side_tracer_callback_register(struct side_event_description *desc,
		side_tracer_callback_func call,
		void *priv);
int side_tracer_callback_variadic_register(struct side_event_description *desc,
		side_tracer_callback_variadic_func call_variadic,
		void *priv);
int side_tracer_callback_unregister(struct side_event_description *desc,
		side_tracer_callback_func call,
		void *priv);
int side_tracer_callback_variadic_unregister(struct side_event_description *desc,
		side_tracer_callback_variadic_func call_variadic,
		void *priv);

enum side_tracer_notification {
	SIDE_TRACER_NOTIFICATION_INSERT_EVENTS,
	SIDE_TRACER_NOTIFICATION_REMOVE_EVENTS,
};

/* Callback is invoked with side library internal lock held. */
struct side_tracer_handle *side_tracer_event_notification_register(
		void (*cb)(enum side_tracer_notification notif,
			struct side_event_description **events, uint32_t nr_events, void *priv),
		void *priv);
void side_tracer_event_notification_unregister(struct side_tracer_handle *handle);

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
 * shared-ojbect (or for the whole main program).
 */
extern struct side_event_description * __start_side_event_description_ptr[]
	__attribute__((weak, visibility("hidden")));
extern struct side_event_description * __stop_side_event_description_ptr[]
	__attribute__((weak, visibility("hidden")));
int side_event_description_ptr_registered
        __attribute__((weak, visibility("hidden")));
struct side_events_register_handle *side_events_handle
	__attribute__((weak, visibility("hidden")));

static void
side_event_description_ptr_init(void)
	__attribute__((no_instrument_function))
	__attribute__((constructor));
static void
side_event_description_ptr_init(void)
{
	if (side_event_description_ptr_registered++)
		return;
	side_events_handle = side_events_register(__start_side_event_description_ptr,
		__stop_side_event_description_ptr - __start_side_event_description_ptr);
}

static void
side_event_description_ptr_exit(void)
	__attribute__((no_instrument_function))
	__attribute__((destructor));
static void
side_event_description_ptr_exit(void)
{
	if (--side_event_description_ptr_registered)
		return;
	side_events_unregister(side_events_handle);
	side_events_handle = NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* _SIDE_TRACE_H */
