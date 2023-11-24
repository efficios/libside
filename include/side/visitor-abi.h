// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_VISITOR_ABI_H
#define SIDE_VISITOR_ABI_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

struct side_arg;
struct side_arg_dynamic_field;
struct side_tracer_visitor_ctx;
struct side_tracer_dynamic_struct_visitor_ctx;

/* The visitor pattern is a double-dispatch visitor. */

typedef enum side_visitor_status (*side_write_elem_func)(
		const struct side_tracer_visitor_ctx *tracer_ctx,
		const struct side_arg *elem);
typedef enum side_visitor_status (*side_visitor_func)(
		const struct side_tracer_visitor_ctx *tracer_ctx,
		void *app_ctx);

struct side_tracer_visitor_ctx {
	side_write_elem_func write_elem;
	void *priv;		/* Private tracer context. */
} SIDE_PACKED;

typedef enum side_visitor_status (*side_write_field_func)(
		const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
		const struct side_arg_dynamic_field *dynamic_field);
typedef enum side_visitor_status (*side_dynamic_struct_visitor_func)(
		const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
		void *app_ctx);

struct side_tracer_dynamic_struct_visitor_ctx {
	side_write_field_func write_field;
	void *priv;		/* Private tracer context. */
} SIDE_PACKED;

#endif /* SIDE_VISITOR_ABI_H */
