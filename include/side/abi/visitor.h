// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_ABI_VISITOR_H
#define SIDE_ABI_VISITOR_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

/*
 * SIDE ABI for visitor pattern.
 *
 * The visitor pattern is a double-dispatch visitor. Changing this ABI
 * is a breaking ABI change.
 *
 * This ABI is a contract between the instrumented application and
 * user-space tracers. Kernel tracers are not expected to interact with
 * visitors directly: a proxy in libside should execute visitors to
 * convert their output to other types which can be read by the kernel
 * tracers.
 */

struct side_arg;
struct side_arg_dynamic_field;
struct side_tracer_visitor_ctx;
struct side_tracer_dynamic_struct_visitor_ctx;

typedef enum side_visitor_status (*side_write_elem_func)(
		const struct side_tracer_visitor_ctx *tracer_ctx,
		const struct side_arg *elem);
typedef enum side_visitor_status (*side_visitor_func)(
		const struct side_tracer_visitor_ctx *tracer_ctx,
		void *app_ctx);

struct side_tracer_visitor_ctx {
	side_write_elem_func write_elem;
	void *priv;		/* Private tracer context. */
};

typedef enum side_visitor_status (*side_write_field_func)(
		const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
		const struct side_arg_dynamic_field *dynamic_field);
typedef enum side_visitor_status (*side_dynamic_struct_visitor_func)(
		const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
		void *app_ctx);

struct side_tracer_dynamic_struct_visitor_ctx {
	side_write_field_func write_field;
	void *priv;		/* Private tracer context. */
};

#endif /* SIDE_ABI_VISITOR_H */
