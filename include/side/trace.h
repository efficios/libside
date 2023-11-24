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
 * This instrumentation ABI exposes 3 type systems:
 *
 * * Stack-copy type system: This is the core type system which can
 *   represent all supported types and into which all other type systems
 *   can be nested. This type system requires that every type is
 *   statically or dynamically declared and then registered, thus giving
 *   tracers a complete description of the events and their associated
 *   fields before the associated instrumentation is invoked. The
 *   application needs to copy each argument (side_arg_...()) onto the
 *   stack when calling the instrumentation.
 *
 *   This is the most expressive of the 3 type systems, althrough not the
 *   fastest due to the extra copy of the arguments.
 *
 * * Data-gathering type system: This type system requires every type to
 *   be statically or dynamically declared and registered, but does not
 *   require the application to copy its arguments onto the stack.
 *   Instead, the type description contains all the required information
 *   to fetch the data from the application memory. The only argument
 *   required from the instrumentation is the base pointer from which
 *   the data should be fetched.
 *
 *   This type system can be used as an event field, or nested within
 *   the stack-copy type system. Nesting of gather-vla within
 *   gather-array and gather-vla types is not allowed.
 *
 *   This type system is has the least overhead of the 3 type systems.
 *
 * * Dynamic type system: This type system receives both type
 *   description and actual data onto the stack at runtime. It has more
 *   overhead that the 2 other type systems, but does not require a
 *   prior registration of event field description. This makes it useful
 *   for seldom used types which are not performance critical, but for
 *   which registering each individual events would needlessly grow the
 *   number of events to declare and register.
 *
 *   Another use-case for this type system is for use by dynamically
 *   typed language runtimes, where the field type is only known when
 *   the instrumentation is called.
 *
 *   Those dynamic types can be either used as arguments to a variadic
 *   field list, or as on-stack instrumentation argument for a static
 *   type SIDE_TYPE_DYNAMIC place holder in the stack-copy type system.
 *
 * The extensibility scheme for the SIDE ABI is as follows:
 *
 * * Existing field types are never changed nor extended. Field types
 *   can be added to the ABI by reserving a label within
 *   enum side_type_label.
 * * Existing attribute types are never changed nor extended. Attribute
 *   types can be added to the ABI by reserving a label within
 *   enum side_attr_type.
 * * Each union part of the ABI has an explicit size defined by a
 *   side_padding() member. Each structure and union have a static
 *   assert validating its size.
 * * If the semantic of the existing event description or type fields
 *   change, the SIDE_EVENT_DESCRIPTION_ABI_VERSION should be increased.
 * * If the semantic of the "struct side_event_state_N" fields change,
 *   the SIDE_EVENT_STATE_ABI_VERSION should be increased. The
 *   "struct side_event_state_N" is not extensible and must have its
 *   ABI version increased whenever it is changed. Note that increasing
 *   the version of SIDE_EVENT_DESCRIPTION_ABI_VERSION is not necessary
 *   when changing the layout of "struct side_event_state_N".
 *
 * Handling of unknown types by the tracers:
 *
 * * A tracer may choose to support only a subset of the types supported
 *   by libside. When encountering an unknown or unsupported type, the
 *   tracer has the option to either disallow the entire event or skip
 *   over the unknown type, both at event registration and when
 *   receiving the side_call arguments.
 *
 * * Event descriptions can be extended by adding fields at the end of
 *   the structure. The "struct side_event_description" is a structure
 *   with flexible size which must not be used within arrays.
 */

#define SIDE_EVENT_STATE_ABI_VERSION		0

#include <side/event-description-abi.h>
#include <side/type-argument-abi.h>
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
	side_ptr_t(const struct side_callback) callbacks;
	side_ptr_t(struct side_event_description) desc;
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
