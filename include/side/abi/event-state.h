// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_ABI_EVENT_STATE_H
#define SIDE_ABI_EVENT_STATE_H

#include <stdint.h>
#include <side/macros.h>

#include <side/abi/event-description.h>

/*
 * SIDE ABI for event state.
 *
 * The state of an event is what a tracer writes to when it enables the
 * event, and it is also how an event is reached: the
 * side_event_state_ptr section holds one state per event, and the
 * description of an event hangs off its state.
 *
 * The edge runs that way, rather than from the description to the
 * state, because a description must hold no address of its own for the
 * pages it lives on to stay clean and shared between processes. See
 * side_ptr_rel_t. It suits the runtime as well: side_call() is given a
 * state, and takes the description from it only when a tracer is
 * listening.
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

struct side_callback;

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
	struct side_event_state parent;		/* Required first field. */
	uint32_t nr_callbacks;
	uintptr_t enabled;
	const struct side_callback *callbacks;
	struct side_event_description *desc;
};

/*
 * The description of the event a state belongs to, or NULL where the
 * state is of an ABI version this does not know. This is the only edge
 * between a state and its description; a tracer handed a state, by a
 * notification or by a call, reaches the description through here.
 */
static inline
struct side_event_description *side_event_state_description(const struct side_event_state *state)
{
	const struct side_event_state_0 *es0;

	if (state->version != SIDE_EVENT_STATE_ABI_VERSION)
		return NULL;
	es0 = side_container_of(state, const struct side_event_state_0, parent);
	return es0->desc;
}

#endif /* SIDE_ABI_EVENT_STATE_H */
