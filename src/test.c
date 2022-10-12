// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include <side/trace.h>
#include "tracer.h"

/* User code example */

static side_define_event(my_provider_event, "myprovider", "myevent", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field(SIDE_TYPE_U32, "abc"),
		side_field(SIDE_TYPE_S64, "def"),
		side_field(SIDE_TYPE_DYNAMIC, "dynamic"),
	)
);

static
void test_fields(void)
{
	uint32_t uw = 42;
	int64_t sdw = -500;

	my_provider_event.enabled = 1;
	side_event(&my_provider_event, side_arg_list(side_arg_u32(uw), side_arg_s64(sdw), side_arg_string("zzz")));
}

static side_define_event(my_provider_event2, "myprovider", "myevent2", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_struct("structfield",
			side_field_list(
				side_field(SIDE_TYPE_U32, "x"),
				side_field(SIDE_TYPE_S64, "y"),
			)
		),
		side_field(SIDE_TYPE_U8, "z"),
	)
);

static
void test_struct(void)
{
	my_provider_event2.enabled = 1;
	side_event_cond(&my_provider_event2) {
		side_arg_define_vec(mystruct, side_arg_list(side_arg_u32(21), side_arg_s64(22)));
		side_event_call(&my_provider_event2, side_arg_list(side_arg_struct(&mystruct), side_arg_u8(55)));
	}
}

static side_define_event(my_provider_event_array, "myprovider", "myarray", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_array("arr", side_array_elem(SIDE_TYPE_U32), 3),
		side_field(SIDE_TYPE_S64, "v"),
	)
);

static
void test_array(void)
{
	my_provider_event_array.enabled = 1;
	side_event_cond(&my_provider_event_array) {
		side_arg_define_vec(myarray, side_arg_list(side_arg_u32(1), side_arg_u32(2), side_arg_u32(3)));
		side_event_call(&my_provider_event_array, side_arg_list(side_arg_array(&myarray), side_arg_s64(42)));
	}
}

static side_define_event(my_provider_event_vla, "myprovider", "myvla", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_vla("vla", side_vla_elem(SIDE_TYPE_U32)),
		side_field(SIDE_TYPE_S64, "v"),
	)
);

static
void test_vla(void)
{
	my_provider_event_vla.enabled = 1;
	side_event_cond(&my_provider_event_vla) {
		side_arg_define_vec(myvla, side_arg_list(side_arg_u32(1), side_arg_u32(2), side_arg_u32(3)));
		side_event_call(&my_provider_event_vla, side_arg_list(side_arg_vla(&myvla), side_arg_s64(42)));
	}
}

struct app_visitor_ctx {
	const uint32_t *ptr;
	int init_pos;
	int current_pos;
	int end_pos;
};

enum side_visitor_status test_visitor_begin(void *_ctx)
{
	struct app_visitor_ctx *ctx = (struct app_visitor_ctx *) _ctx;
	ctx->current_pos = ctx->init_pos;
	return SIDE_VISITOR_STATUS_OK;
}

enum side_visitor_status test_visitor_end(void *_ctx)
{
	return SIDE_VISITOR_STATUS_OK;
}

enum side_visitor_status test_visitor_get_next(void *_ctx, struct side_arg_vec *sav_elem)
{
	struct app_visitor_ctx *ctx = (struct app_visitor_ctx *) _ctx;

	if (ctx->current_pos >= ctx->end_pos)
		return SIDE_VISITOR_STATUS_END;
	sav_elem->type = SIDE_TYPE_U32;
	sav_elem->u.side_u32 = ctx->ptr[ctx->current_pos++];
	return SIDE_VISITOR_STATUS_OK;
}

static uint32_t testarray[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

static side_define_event(my_provider_event_vla_visitor, "myprovider", "myvlavisit", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_vla_visitor("vlavisit", side_vla_visitor_elem(SIDE_TYPE_U32),
			test_visitor_begin, test_visitor_end, test_visitor_get_next),
		side_field(SIDE_TYPE_S64, "v"),
	)
);

static
void test_vla_visitor(void)
{
	my_provider_event_vla_visitor.enabled = 1;
	side_event_cond(&my_provider_event_vla_visitor) {
		struct app_visitor_ctx ctx = {
			.ptr = testarray,
			.init_pos = 0,
			.current_pos = 0,
			.end_pos = SIDE_ARRAY_SIZE(testarray),
		};
		side_event_call(&my_provider_event_vla_visitor, side_arg_list(side_arg_vla_visitor(&ctx), side_arg_s64(42)));
	}
}

int main()
{
	test_fields();
	test_struct();
	test_array();
	test_vla();
	test_vla_visitor();
	return 0;
}
