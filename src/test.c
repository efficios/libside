// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <side/trace.h>
#include "tracer.h"

/* User code example */

static side_define_event(my_provider_event, "myprovider", "myevent", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("abc", SIDE_TYPE_U32, side_attr_list()),
		side_field("def", SIDE_TYPE_S64, side_attr_list()),
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

static
void test_fields(void)
{
	uint32_t uw = 42;
	int64_t sdw = -500;

	my_provider_event.enabled = 1;
	side_event(&my_provider_event, side_arg_list(side_arg_u32(uw), side_arg_s64(sdw),
		side_arg_dynamic(side_arg_dynamic_string("zzz", side_attr_list()))));
}

static side_define_event(my_provider_event2, "myprovider", "myevent2", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_struct("structfield",
			side_field_list(
				side_field("x", SIDE_TYPE_U32, side_attr_list()),
				side_field("y", SIDE_TYPE_S64, side_attr_list()),
			),
			side_attr_list()
		),
		side_field("z", SIDE_TYPE_U8, side_attr_list()),
	),
	side_attr_list()
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
		side_field_array("arr", side_elem_type(SIDE_TYPE_U32, side_attr_list()), 3, side_attr_list()),
		side_field("v", SIDE_TYPE_S64, side_attr_list()),
	),
	side_attr_list()
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
		side_field_vla("vla", side_elem_type(SIDE_TYPE_U32, side_attr_list()), side_attr_list()),
		side_field("v", SIDE_TYPE_S64, side_attr_list()),
	),
	side_attr_list()
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

/* 1D array visitor */

struct app_visitor_ctx {
	const uint32_t *ptr;
	uint32_t length;
};

static
enum side_visitor_status test_visitor(const struct side_tracer_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_visitor_ctx *ctx = (struct app_visitor_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		const struct side_arg_vec elem = {
			.type = SIDE_TYPE_U32,
			.u = {
				.side_u32 = ctx->ptr[i],
			},
		};
		if (tracer_ctx->write_elem(tracer_ctx, &elem) != SIDE_VISITOR_STATUS_OK)
			return SIDE_VISITOR_STATUS_ERROR;
	}
	return SIDE_VISITOR_STATUS_OK;
}

static uint32_t testarray[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

static side_define_event(my_provider_event_vla_visitor, "myprovider", "myvlavisit", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_vla_visitor("vlavisit", side_elem_type(SIDE_TYPE_U32, side_attr_list()), test_visitor, side_attr_list()),
		side_field("v", SIDE_TYPE_S64, side_attr_list()),
	),
	side_attr_list()
);

static
void test_vla_visitor(void)
{
	my_provider_event_vla_visitor.enabled = 1;
	side_event_cond(&my_provider_event_vla_visitor) {
		struct app_visitor_ctx ctx = {
			.ptr = testarray,
			.length = SIDE_ARRAY_SIZE(testarray),
		};
		side_event_call(&my_provider_event_vla_visitor, side_arg_list(side_arg_vla_visitor(&ctx), side_arg_s64(42)));
	}
}

/* 2D array visitor */

struct app_visitor_2d_inner_ctx {
	const uint32_t *ptr;
	uint32_t length;
};

static
enum side_visitor_status test_inner_visitor(const struct side_tracer_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_visitor_2d_inner_ctx *ctx = (struct app_visitor_2d_inner_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		const struct side_arg_vec elem = {
			.type = SIDE_TYPE_U32,
			.u = {
				.side_u32 = ctx->ptr[i],
			},
		};
		if (tracer_ctx->write_elem(tracer_ctx, &elem) != SIDE_VISITOR_STATUS_OK)
			return SIDE_VISITOR_STATUS_ERROR;
	}
	return SIDE_VISITOR_STATUS_OK;
}

struct app_visitor_2d_outer_ctx {
	const uint32_t (*ptr)[2];
	uint32_t length;
};

static
enum side_visitor_status test_outer_visitor(const struct side_tracer_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_visitor_2d_outer_ctx *ctx = (struct app_visitor_2d_outer_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		struct app_visitor_2d_inner_ctx inner_ctx = {
			.ptr = ctx->ptr[i],
			.length = 2,
		};
		const struct side_arg_vec elem = side_arg_vla_visitor(&inner_ctx);
		if (tracer_ctx->write_elem(tracer_ctx, &elem) != SIDE_VISITOR_STATUS_OK)
			return SIDE_VISITOR_STATUS_ERROR;
	}
	return SIDE_VISITOR_STATUS_OK;
}

static uint32_t testarray2d[][2] = {
	{ 1, 2 },
	{ 33, 44 },
	{ 55, 66 },
};

static side_define_event(my_provider_event_vla_visitor2d, "myprovider", "myvlavisit2d", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_vla_visitor("vlavisit2d",
			side_elem(
				side_type_vla_visitor_decl(
					side_elem_type(SIDE_TYPE_U32, side_attr_list()),
					test_inner_visitor,
					side_attr_list())
			), test_outer_visitor, side_attr_list()),
		side_field("v", SIDE_TYPE_S64, side_attr_list()),
	),
	side_attr_list()
);

static
void test_vla_visitor_2d(void)
{
	my_provider_event_vla_visitor2d.enabled = 1;
	side_event_cond(&my_provider_event_vla_visitor2d) {
		struct app_visitor_2d_outer_ctx ctx = {
			.ptr = testarray2d,
			.length = SIDE_ARRAY_SIZE(testarray2d),
		};
		side_event_call(&my_provider_event_vla_visitor2d, side_arg_list(side_arg_vla_visitor(&ctx), side_arg_s64(42)));
	}
}

static int64_t array_fixint[] = { -444, 555, 123, 2897432587 };

static side_define_event(my_provider_event_array_fixint, "myprovider", "myarrayfixint", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_array("arrfixint", side_elem_type(SIDE_TYPE_S64, side_attr_list()), SIDE_ARRAY_SIZE(array_fixint), side_attr_list()),
		side_field("v", SIDE_TYPE_S64, side_attr_list()),
	),
	side_attr_list()
);

static
void test_array_fixint(void)
{
	my_provider_event_array_fixint.enabled = 1;
	side_event(&my_provider_event_array_fixint,
		side_arg_list(side_arg_array_s64(array_fixint), side_arg_s64(42)));
}

static int64_t vla_fixint[] = { -444, 555, 123, 2897432587 };

static side_define_event(my_provider_event_vla_fixint, "myprovider", "myvlafixint", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_vla("vlafixint", side_elem_type(SIDE_TYPE_S64, side_attr_list()), side_attr_list()),
		side_field("v", SIDE_TYPE_S64, side_attr_list()),
	),
	side_attr_list()
);

static
void test_vla_fixint(void)
{
	my_provider_event_vla_fixint.enabled = 1;
	side_event(&my_provider_event_vla_fixint,
		side_arg_list(side_arg_vla_s64(vla_fixint, SIDE_ARRAY_SIZE(vla_fixint)), side_arg_s64(42)));
}

static side_define_event(my_provider_event_dynamic_basic,
	"myprovider", "mydynamicbasic", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

static
void test_dynamic_basic_type(void)
{
	my_provider_event_dynamic_basic.enabled = 1;
	side_event(&my_provider_event_dynamic_basic,
		side_arg_list(side_arg_dynamic(side_arg_dynamic_s16(-33, side_attr_list()))));
}

static side_define_event(my_provider_event_dynamic_vla,
	"myprovider", "mydynamicvla", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

static
void test_dynamic_vla(void)
{
	side_arg_dynamic_define_vec(myvla,
		side_arg_list(
			side_arg_dynamic_u32(1, side_attr_list()),
			side_arg_dynamic_u32(2, side_attr_list()),
			side_arg_dynamic_u32(3, side_attr_list()),
		)
	);
	my_provider_event_dynamic_vla.enabled = 1;
	side_event(&my_provider_event_dynamic_vla,
		side_arg_list(side_arg_dynamic(side_arg_dynamic_vla(&myvla, side_attr_list()))));
}

static side_define_event(my_provider_event_dynamic_null,
	"myprovider", "mydynamicnull", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

static
void test_dynamic_null(void)
{
	my_provider_event_dynamic_null.enabled = 1;
	side_event(&my_provider_event_dynamic_null,
		side_arg_list(side_arg_dynamic(side_arg_dynamic_null(side_attr_list()))));
}

static side_define_event(my_provider_event_dynamic_struct,
	"myprovider", "mydynamicstruct", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

static
void test_dynamic_struct(void)
{
	side_arg_dynamic_define_struct(mystruct,
		side_arg_list(
			side_arg_dynamic_field("a", side_arg_dynamic_u32(43, side_attr_list())),
			side_arg_dynamic_field("b", side_arg_dynamic_string("zzz", side_attr_list())),
			side_arg_dynamic_field("c", side_arg_dynamic_null(side_attr_list())),
		)
	);

	my_provider_event_dynamic_struct.enabled = 1;
	side_event(&my_provider_event_dynamic_struct,
		side_arg_list(side_arg_dynamic(side_arg_dynamic_struct(&mystruct, side_attr_list()))));
}

static side_define_event(my_provider_event_dynamic_nested_struct,
	"myprovider", "mydynamicnestedstruct", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

static
void test_dynamic_nested_struct(void)
{
	side_arg_dynamic_define_struct(nested,
		side_arg_list(
			side_arg_dynamic_field("a", side_arg_dynamic_u32(43, side_attr_list())),
			side_arg_dynamic_field("b", side_arg_dynamic_u8(55, side_attr_list())),
		)
	);
	side_arg_dynamic_define_struct(nested2,
		side_arg_list(
			side_arg_dynamic_field("aa", side_arg_dynamic_u64(128, side_attr_list())),
			side_arg_dynamic_field("bb", side_arg_dynamic_u16(1, side_attr_list())),
		)
	);
	side_arg_dynamic_define_struct(mystruct,
		side_arg_list(
			side_arg_dynamic_field("nested", side_arg_dynamic_struct(&nested, side_attr_list())),
			side_arg_dynamic_field("nested2", side_arg_dynamic_struct(&nested2, side_attr_list())),
		)
	);
	my_provider_event_dynamic_nested_struct.enabled = 1;
	side_event(&my_provider_event_dynamic_nested_struct,
		side_arg_list(side_arg_dynamic(side_arg_dynamic_struct(&mystruct, side_attr_list()))));
}

static side_define_event(my_provider_event_dynamic_vla_struct,
	"myprovider", "mydynamicvlastruct", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

static
void test_dynamic_vla_struct(void)
{
	side_arg_dynamic_define_struct(nested,
		side_arg_list(
			side_arg_dynamic_field("a", side_arg_dynamic_u32(43, side_attr_list())),
			side_arg_dynamic_field("b", side_arg_dynamic_u8(55, side_attr_list())),
		)
	);
	side_arg_dynamic_define_vec(myvla,
		side_arg_list(
			side_arg_dynamic_struct(&nested, side_attr_list()),
			side_arg_dynamic_struct(&nested, side_attr_list()),
			side_arg_dynamic_struct(&nested, side_attr_list()),
			side_arg_dynamic_struct(&nested, side_attr_list()),
		)
	);
	my_provider_event_dynamic_vla_struct.enabled = 1;
	side_event(&my_provider_event_dynamic_vla_struct,
		side_arg_list(side_arg_dynamic(side_arg_dynamic_vla(&myvla, side_attr_list()))));
}

static side_define_event(my_provider_event_dynamic_struct_vla,
	"myprovider", "mydynamicstructvla", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

static
void test_dynamic_struct_vla(void)
{
	side_arg_dynamic_define_vec(myvla,
		side_arg_list(
			side_arg_dynamic_u32(1, side_attr_list()),
			side_arg_dynamic_u32(2, side_attr_list()),
			side_arg_dynamic_u32(3, side_attr_list()),
		)
	);
	side_arg_dynamic_define_vec(myvla2,
		side_arg_list(
			side_arg_dynamic_u32(4, side_attr_list()),
			side_arg_dynamic_u64(5, side_attr_list()),
			side_arg_dynamic_u32(6, side_attr_list()),
		)
	);
	side_arg_dynamic_define_struct(mystruct,
		side_arg_list(
			side_arg_dynamic_field("a", side_arg_dynamic_vla(&myvla, side_attr_list())),
			side_arg_dynamic_field("b", side_arg_dynamic_vla(&myvla2, side_attr_list())),
		)
	);
	my_provider_event_dynamic_struct_vla.enabled = 1;
	side_event(&my_provider_event_dynamic_struct_vla,
		side_arg_list(side_arg_dynamic(side_arg_dynamic_struct(&mystruct, side_attr_list()))));
}

static side_define_event(my_provider_event_dynamic_nested_vla,
	"myprovider", "mydynamicnestedvla", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

static
void test_dynamic_nested_vla(void)
{
	side_arg_dynamic_define_vec(nestedvla,
		side_arg_list(
			side_arg_dynamic_u32(1, side_attr_list()),
			side_arg_dynamic_u16(2, side_attr_list()),
			side_arg_dynamic_u32(3, side_attr_list()),
		)
	);
	side_arg_dynamic_define_vec(nestedvla2,
		side_arg_list(
			side_arg_dynamic_u8(4, side_attr_list()),
			side_arg_dynamic_u32(5, side_attr_list()),
			side_arg_dynamic_u32(6, side_attr_list()),
		)
	);
	side_arg_dynamic_define_vec(myvla,
		side_arg_list(
			side_arg_dynamic_vla(&nestedvla, side_attr_list()),
			side_arg_dynamic_vla(&nestedvla2, side_attr_list()),
		)
	);
	my_provider_event_dynamic_nested_vla.enabled = 1;
	side_event(&my_provider_event_dynamic_nested_vla,
		side_arg_list(side_arg_dynamic(side_arg_dynamic_vla(&myvla, side_attr_list()))));
}

static side_define_event_variadic(my_provider_event_variadic,
	"myprovider", "myvariadicevent", SIDE_LOGLEVEL_DEBUG,
	side_field_list(),
	side_attr_list()
);

static
void test_variadic(void)
{
	my_provider_event_variadic.enabled = 1;
	side_event_variadic(&my_provider_event_variadic,
		side_arg_list(),
		side_arg_list(
			side_arg_dynamic_field("a", side_arg_dynamic_u32(55, side_attr_list())),
			side_arg_dynamic_field("b", side_arg_dynamic_s8(-4, side_attr_list())),
		)
	);
}

static side_define_event_variadic(my_provider_event_static_variadic,
	"myprovider", "mystaticvariadicevent", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("abc", SIDE_TYPE_U32, side_attr_list()),
		side_field("def", SIDE_TYPE_U16, side_attr_list()),
	),
	side_attr_list()
);

static
void test_static_variadic(void)
{
	my_provider_event_static_variadic.enabled = 1;
	side_event_variadic(&my_provider_event_static_variadic,
		side_arg_list(
			side_arg_u32(1),
			side_arg_u16(2),
		),
		side_arg_list(
			side_arg_dynamic_field("a", side_arg_dynamic_u32(55, side_attr_list())),
			side_arg_dynamic_field("b", side_arg_dynamic_s8(-4, side_attr_list())),
		)
	);
}

static side_define_event(my_provider_event_bool, "myprovider", "myeventbool", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("a_false", SIDE_TYPE_BOOL, side_attr_list()),
		side_field("b_true", SIDE_TYPE_BOOL, side_attr_list()),
		side_field("c_true", SIDE_TYPE_BOOL, side_attr_list()),
		side_field("d_true", SIDE_TYPE_BOOL, side_attr_list()),
		side_field("e_true", SIDE_TYPE_BOOL, side_attr_list()),
		side_field("f_false", SIDE_TYPE_BOOL, side_attr_list()),
		side_field("g_true", SIDE_TYPE_BOOL, side_attr_list()),
	),
	side_attr_list()
);

static
void test_bool(void)
{
	uint32_t a = 0;
	uint32_t b = 1;
	uint64_t c = 0x12345678;
	int16_t d = -32768;
	bool e = true;
	bool f = false;
	uint32_t g = 256;

	my_provider_event_bool.enabled = 1;
	side_event(&my_provider_event_bool,
		side_arg_list(
			side_arg_bool(a),
			side_arg_bool(b),
			side_arg_bool(c),
			side_arg_bool(d),
			side_arg_bool(e),
			side_arg_bool(f),
			side_arg_bool(g),
		)
	);
}

static side_define_event_variadic(my_provider_event_dynamic_bool,
	"myprovider", "mydynamicbool", SIDE_LOGLEVEL_DEBUG,
	side_field_list(),
	side_attr_list()
);

static
void test_dynamic_bool(void)
{
	my_provider_event_dynamic_bool.enabled = 1;
	side_event_variadic(&my_provider_event_dynamic_bool,
		side_arg_list(),
		side_arg_list(
			side_arg_dynamic_field("a_true", side_arg_dynamic_bool(55, side_attr_list())),
			side_arg_dynamic_field("b_true", side_arg_dynamic_bool(-4, side_attr_list())),
			side_arg_dynamic_field("c_false", side_arg_dynamic_bool(0, side_attr_list())),
			side_arg_dynamic_field("d_true", side_arg_dynamic_bool(256, side_attr_list())),
		)
	);
}

static side_define_event(my_provider_event_dynamic_vla_visitor,
	"myprovider", "mydynamicvlavisitor", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

struct app_dynamic_vla_visitor_ctx {
	const uint32_t *ptr;
	uint32_t length;
};

static
enum side_visitor_status test_dynamic_vla_visitor(const struct side_tracer_dynamic_vla_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_dynamic_vla_visitor_ctx *ctx = (struct app_dynamic_vla_visitor_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		const struct side_arg_dynamic_vec elem = {
			.dynamic_type = SIDE_DYNAMIC_TYPE_U32,
			.nr_attr = 0,
			.attr = NULL,
			.u = {
				.side_u32 = ctx->ptr[i],
			},
		};
		if (tracer_ctx->write_elem(tracer_ctx, &elem) != SIDE_VISITOR_STATUS_OK)
			return SIDE_VISITOR_STATUS_ERROR;
	}
	return SIDE_VISITOR_STATUS_OK;
}

static uint32_t testarray_dynamic_vla[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

static
void test_dynamic_vla_with_visitor(void)
{
	my_provider_event_dynamic_vla_visitor.enabled = 1;
	side_event_cond(&my_provider_event_dynamic_vla_visitor) {
		struct app_dynamic_vla_visitor_ctx ctx = {
			.ptr = testarray_dynamic_vla,
			.length = SIDE_ARRAY_SIZE(testarray_dynamic_vla),
		};
		side_event_call(&my_provider_event_dynamic_vla_visitor,
			side_arg_list(
				side_arg_dynamic(
					side_arg_dynamic_vla_visitor(test_dynamic_vla_visitor, &ctx, side_attr_list())
				)
			)
		);
	}
}

static side_define_event(my_provider_event_dynamic_struct_visitor,
	"myprovider", "mydynamicstructvisitor", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("dynamic", SIDE_TYPE_DYNAMIC, side_attr_list()),
	),
	side_attr_list()
);

struct struct_visitor_pair {
	const char *name;
	uint32_t value;
};

struct app_dynamic_struct_visitor_ctx {
	const struct struct_visitor_pair *ptr;
	uint32_t length;
};

static
enum side_visitor_status test_dynamic_struct_visitor(const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_dynamic_struct_visitor_ctx *ctx = (struct app_dynamic_struct_visitor_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		struct side_arg_dynamic_event_field dynamic_field = {
			.field_name = ctx->ptr[i].name,
			.elem = {
				.dynamic_type = SIDE_DYNAMIC_TYPE_U32,
				.nr_attr = 0,
				.attr = NULL,
				.u = {
					.side_u32 = ctx->ptr[i].value,
				},
			},
		};
		if (tracer_ctx->write_field(tracer_ctx, &dynamic_field) != SIDE_VISITOR_STATUS_OK)
			return SIDE_VISITOR_STATUS_ERROR;
	}
	return SIDE_VISITOR_STATUS_OK;
}

static struct struct_visitor_pair testarray_dynamic_struct[] = {
	{ "a", 1, },
	{ "b", 2, },
	{ "c", 3, },
	{ "d", 4, },
};

static
void test_dynamic_struct_with_visitor(void)
{
	my_provider_event_dynamic_struct_visitor.enabled = 1;
	side_event_cond(&my_provider_event_dynamic_struct_visitor) {
		struct app_dynamic_struct_visitor_ctx ctx = {
			.ptr = testarray_dynamic_struct,
			.length = SIDE_ARRAY_SIZE(testarray_dynamic_struct),
		};
		side_event_call(&my_provider_event_dynamic_struct_visitor,
			side_arg_list(
				side_arg_dynamic(
					side_arg_dynamic_struct_visitor(test_dynamic_struct_visitor, &ctx, side_attr_list())
				)
			)
		);
	}
}

static side_define_event(my_provider_event_user_attribute, "myprovider", "myevent_user_attribute", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("abc", SIDE_TYPE_U32, side_attr_list()),
		side_field("def", SIDE_TYPE_S64, side_attr_list()),
	),
	side_attr_list(
		side_attr("user_attribute_a", side_attr_string("val1")),
		side_attr("user_attribute_b", side_attr_string("val2")),
	)
);

static
void test_event_user_attribute(void)
{
	my_provider_event_user_attribute.enabled = 1;
	side_event(&my_provider_event_user_attribute, side_arg_list(side_arg_u32(1), side_arg_s64(2)));
}

static side_define_event(my_provider_field_user_attribute, "myprovider", "myevent_field_attribute", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field("abc", SIDE_TYPE_U32,
			side_attr_list(
				side_attr("user_attribute_a", side_attr_string("val1")),
				side_attr("user_attribute_b", side_attr_u32(2)),
			)
		),
		side_field("def", SIDE_TYPE_S64,
			side_attr_list(
				side_attr("user_attribute_c", side_attr_string("val3")),
				side_attr("user_attribute_d", side_attr_s64(-5)),
			)
		),
	),
	side_attr_list()
);

static
void test_field_user_attribute(void)
{
	my_provider_field_user_attribute.enabled = 1;
	side_event(&my_provider_field_user_attribute, side_arg_list(side_arg_u32(1), side_arg_s64(2)));
}

static side_define_event_variadic(my_provider_event_variadic_attr,
	"myprovider", "myvariadiceventattr", SIDE_LOGLEVEL_DEBUG,
	side_field_list(),
	side_attr_list()
);

static
void test_variadic_attr(void)
{
	my_provider_event_variadic_attr.enabled = 1;
	side_event_variadic(&my_provider_event_variadic_attr,
		side_arg_list(),
		side_arg_list(
			side_arg_dynamic_field("a",
				side_arg_dynamic_u32(55,
					side_attr_list(
						side_attr("user_attribute_c", side_attr_string("valX")),
						side_attr("user_attribute_d", side_attr_u8(55)),
					)
				)
			),
			side_arg_dynamic_field("b",
				side_arg_dynamic_s8(-4,
					side_attr_list(
						side_attr("X", side_attr_u8(1)),
						side_attr("Y", side_attr_s8(2)),
					)
				)
			),
		)
	);
}

static side_define_event_variadic(my_provider_event_variadic_vla_attr,
	"myprovider", "myvariadiceventvlaattr", SIDE_LOGLEVEL_DEBUG,
	side_field_list(),
	side_attr_list()
);

static
void test_variadic_vla_attr(void)
{
	side_arg_dynamic_define_vec(myvla,
		side_arg_list(
			side_arg_dynamic_u32(1,
				side_attr_list(
					side_attr("Z", side_attr_u8(0)),
					side_attr("A", side_attr_u8(123)),
				)
			),
			side_arg_dynamic_u32(2, side_attr_list()),
			side_arg_dynamic_u32(3, side_attr_list()),
		)
	);
	my_provider_event_variadic_vla_attr.enabled = 1;
	side_event_variadic(&my_provider_event_variadic_vla_attr,
		side_arg_list(),
		side_arg_list(
			side_arg_dynamic_field("a",
				side_arg_dynamic_vla(&myvla,
					side_attr_list(
						side_attr("X", side_attr_u8(1)),
						side_attr("Y", side_attr_u8(2)),
					)
				)
			),
		)
	);
}

static side_define_event_variadic(my_provider_event_variadic_struct_attr,
	"myprovider", "myvariadiceventstructattr", SIDE_LOGLEVEL_DEBUG,
	side_field_list(),
	side_attr_list()
);

static
void test_variadic_struct_attr(void)
{
	my_provider_event_variadic_struct_attr.enabled = 1;
	side_event_cond(&my_provider_event_variadic_struct_attr) {
		side_arg_dynamic_define_struct(mystruct,
			side_arg_list(
				side_arg_dynamic_field("a",
					side_arg_dynamic_u32(43,
						side_attr_list(
							side_attr("A", side_attr_bool(true)),
						)
					)
				),
				side_arg_dynamic_field("b", side_arg_dynamic_u8(55, side_attr_list())),
			)
		);
		side_event_call_variadic(&my_provider_event_variadic_struct_attr,
			side_arg_list(),
			side_arg_list(
				side_arg_dynamic_field("a",
					side_arg_dynamic_struct(&mystruct,
						side_attr_list(
							side_attr("X", side_attr_u8(1)),
							side_attr("Y", side_attr_u8(2)),
						)
					)
				),
			)
		);
	}
}

static side_define_event(my_provider_event_float, "myprovider", "myeventfloat", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
#if __HAVE_FLOAT16
		side_field("binary16", SIDE_TYPE_FLOAT_BINARY16, side_attr_list()),
#endif
#if __HAVE_FLOAT32
		side_field("binary32", SIDE_TYPE_FLOAT_BINARY32, side_attr_list()),
#endif
#if __HAVE_FLOAT64
		side_field("binary64", SIDE_TYPE_FLOAT_BINARY64, side_attr_list()),
#endif
#if __HAVE_FLOAT128
		side_field("binary128", SIDE_TYPE_FLOAT_BINARY128, side_attr_list()),
#endif
	),
	side_attr_list()
);

static
void test_float(void)
{
	my_provider_event_float.enabled = 1;
	side_event(&my_provider_event_float,
		side_arg_list(
#if __HAVE_FLOAT16
			side_arg_float_binary16(1.1),
#endif
#if __HAVE_FLOAT32
			side_arg_float_binary32(2.2),
#endif
#if __HAVE_FLOAT64
			side_arg_float_binary64(3.3),
#endif
#if __HAVE_FLOAT128
			side_arg_float_binary128(4.4),
#endif
		)
	);
}

static side_define_event_variadic(my_provider_event_variadic_float,
	"myprovider", "myvariadicfloat", SIDE_LOGLEVEL_DEBUG,
	side_field_list(),
	side_attr_list()
);

static
void test_variadic_float(void)
{
	my_provider_event_variadic_float.enabled = 1;
	side_event_variadic(&my_provider_event_variadic_float,
		side_arg_list(),
		side_arg_list(
#if __HAVE_FLOAT16
			side_arg_dynamic_field("binary16",
				side_arg_dynamic_float_binary16(1.1, side_attr_list())
			),
#endif
#if __HAVE_FLOAT32
			side_arg_dynamic_field("binary32",
				side_arg_dynamic_float_binary32(2.2, side_attr_list())
			),
#endif
#if __HAVE_FLOAT64
			side_arg_dynamic_field("binary64",
				side_arg_dynamic_float_binary64(3.3, side_attr_list())
			),
#endif
#if __HAVE_FLOAT128
			side_arg_dynamic_field("binary128",
				side_arg_dynamic_float_binary128(4.4, side_attr_list())
			),
#endif
		)
	);
}

static side_define_enum(myenum,
	side_enum_mapping_list(
		side_enum_mapping_range("one-ten", 1, 10),
		side_enum_mapping_range("100-200", 100, 200),
		side_enum_mapping_value("200", 200),
		side_enum_mapping_value("300", 300),
	)
);

static side_define_event(my_provider_event_enum, "myprovider", "myeventenum", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_enum("5", SIDE_TYPE_ENUM_U32, &myenum, side_attr_list()),
		side_field_enum("400", SIDE_TYPE_ENUM_U64, &myenum, side_attr_list()),
		side_field_enum("200", SIDE_TYPE_ENUM_U8, &myenum, side_attr_list()),
		side_field_enum("-100", SIDE_TYPE_ENUM_S8, &myenum, side_attr_list()),
	),
	side_attr_list()
);

static
void test_enum(void)
{
	my_provider_event_enum.enabled = 1;
	side_event(&my_provider_event_enum,
		side_arg_list(
			side_arg_enum_u32(5),
			side_arg_enum_u64(400),
			side_arg_enum_u8(200),
			side_arg_enum_s8(-100),
		)
	);
}

/* A bitmap enum maps bits to labels. */
static side_define_enum_bitmap(myenum_bitmap,
	side_enum_bitmap_mapping_list(
		side_enum_bitmap_mapping_value("0", 0),
		side_enum_bitmap_mapping_range("1-2", 1, 2),
		side_enum_bitmap_mapping_range("2-4", 2, 4),
		side_enum_bitmap_mapping_value("3", 3),
		side_enum_bitmap_mapping_value("30", 30),
		side_enum_bitmap_mapping_value("63", 63),
	)
);

static side_define_event(my_provider_event_enum_bitmap, "myprovider", "myeventenumbitmap", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_enum_bitmap("bit_0", SIDE_TYPE_ENUM_BITMAP32, &myenum_bitmap, side_attr_list()),
		side_field_enum_bitmap("bit_1", SIDE_TYPE_ENUM_BITMAP32, &myenum_bitmap, side_attr_list()),
		side_field_enum_bitmap("bit_2", SIDE_TYPE_ENUM_BITMAP8, &myenum_bitmap, side_attr_list()),
		side_field_enum_bitmap("bit_3", SIDE_TYPE_ENUM_BITMAP8, &myenum_bitmap, side_attr_list()),
		side_field_enum_bitmap("bit_30", SIDE_TYPE_ENUM_BITMAP32, &myenum_bitmap, side_attr_list()),
		side_field_enum_bitmap("bit_31", SIDE_TYPE_ENUM_BITMAP32, &myenum_bitmap, side_attr_list()),
		side_field_enum_bitmap("bit_63", SIDE_TYPE_ENUM_BITMAP64, &myenum_bitmap, side_attr_list()),
		side_field_enum_bitmap("bits_1+63", SIDE_TYPE_ENUM_BITMAP64, &myenum_bitmap, side_attr_list()),
	),
	side_attr_list()
);

static
void test_enum_bitmap(void)
{
	my_provider_event_enum_bitmap.enabled = 1;
	side_event(&my_provider_event_enum_bitmap,
		side_arg_list(
			side_arg_enum_bitmap32(1 << 0),
			side_arg_enum_bitmap32(1 << 1),
			side_arg_enum_bitmap8(1 << 2),
			side_arg_enum_bitmap8(1 << 3),
			side_arg_enum_bitmap32(1 << 30),
			side_arg_enum_bitmap32(1 << 31),
			side_arg_enum_bitmap64(1ULL << 63),
			side_arg_enum_bitmap64((1ULL << 1) | (1ULL << 63)),
		)
	);
}


int main()
{
	test_fields();
	test_struct();
	test_array();
	test_vla();
	test_vla_visitor();
	test_vla_visitor_2d();
	test_array_fixint();
	test_vla_fixint();
	test_dynamic_basic_type();
	test_dynamic_vla();
	test_dynamic_null();
	test_dynamic_struct();
	test_dynamic_nested_struct();
	test_dynamic_vla_struct();
	test_dynamic_struct_vla();
	test_dynamic_nested_vla();
	test_variadic();
	test_static_variadic();
	test_bool();
	test_dynamic_bool();
	test_dynamic_vla_with_visitor();
	test_dynamic_struct_with_visitor();
	test_event_user_attribute();
	test_field_user_attribute();
	test_variadic_attr();
	test_variadic_vla_attr();
	test_variadic_struct_attr();
	test_float();
	test_variadic_float();
	test_enum();
	test_enum_bitmap();
	return 0;
}
