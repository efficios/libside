// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _TRACER_H
#define _TRACER_H

#include <side/trace.h>

void tracer_call(const struct side_event_description *desc, const struct side_arg_vec_description *sav_desc);

#endif
