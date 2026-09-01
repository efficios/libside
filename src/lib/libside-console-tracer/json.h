// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

#ifndef SIDE_CONSOLE_TRACER_JSON_H
#define SIDE_CONSOLE_TRACER_JSON_H

#include <side/trace.h>

#include "libside-tools/visit-arg-vec.h"

/*
 * Emission of the events as JSON, one object per line. The values are
 * emitted as the JSON values they are; an object is only used where
 * there is more to describe than the value itself, such as the labels
 * of an enumeration or the selector of a variant.
 */
extern const struct side_type_visitor json_type_visitor;

/*
 * Describe the events of a notification as a JSON object, one per
 * line.
 */
void json_print_event_notification(enum side_tracer_notification notif,
		struct side_event_state **states, uint32_t nr_events);

#endif /* SIDE_CONSOLE_TRACER_JSON_H */
