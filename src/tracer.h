// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _TRACER_H
#define _TRACER_H

#include <side/trace.h>

void tracer_call(const struct side_event_description *desc,
	const struct side_arg_vec *side_arg_vec,
	void *priv);
void tracer_call_variadic(const struct side_event_description *desc,
	const struct side_arg_vec *side_arg_vec,
	const struct side_arg_dynamic_struct *var_struct,
	void *priv);

#endif
