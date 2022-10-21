// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <side/trace.h>
#include "tracer.h"

#define SIDE_EVENT_ENABLED_KERNEL_USER_EVENT_MASK	0x80000000

void side_call(const struct side_event_description *desc, const struct side_arg_vec_description *sav_desc)
{
	if (side_unlikely(desc->flags & SIDE_EVENT_FLAG_VARIADIC)) {
		printf("ERROR: unexpected variadic event description\n");
		abort();
	}
	if (side_unlikely(desc->enabled & SIDE_EVENT_ENABLED_KERNEL_USER_EVENT_MASK)) {
		// TODO: call kernel ioctl.
	}
	//TODO: replace tracer_call by rcu iteration on list of registered callbacks
	tracer_call(desc, sav_desc);
}

void side_call_variadic(const struct side_event_description *desc,
	const struct side_arg_vec_description *sav_desc,
	const struct side_arg_dynamic_event_struct *var_struct)
{
	if (side_unlikely(desc->enabled & SIDE_EVENT_ENABLED_KERNEL_USER_EVENT_MASK)) {
		// TODO: call kernel ioctl.
	}
	//TODO: replace tracer_call by rcu iteration on list of registered callbacks
	tracer_call_variadic(desc, sav_desc, var_struct);
}
