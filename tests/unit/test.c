// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include <tgif/trace.h>

/* User code example */

tgif_static_event(my_provider_event, "myprovider", "myevent", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_u32("abc", tgif_attr_list()),
		tgif_field_s64("def", tgif_attr_list()),
		tgif_field_pointer("ptr", tgif_attr_list()),
		tgif_field_dynamic("dynamic"),
		tgif_field_dynamic("dynamic_pointer"),
		tgif_field_null("null", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_fields(void)
{
	uint32_t uw = 42;
	int64_t sdw = -500;

	tgif_event(my_provider_event,
		tgif_arg_list(
			tgif_arg_u32(uw),
			tgif_arg_s64(sdw),
			tgif_arg_pointer((void *) 0x1),
			tgif_arg_dynamic_string("zzz", tgif_attr_list()),
			tgif_arg_dynamic_pointer((void *) 0x1, tgif_attr_list()),
			tgif_arg_null(),
		)
	);
}

tgif_hidden_event(my_provider_event_hidden, "myprovider", "myeventhidden", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_u32("abc", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_event_hidden(void)
{
	tgif_event(my_provider_event_hidden, tgif_arg_list(tgif_arg_u32(2)));
}

tgif_declare_event(my_provider_event_export);

tgif_export_event(my_provider_event_export, "myprovider", "myeventexport", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_u32("abc", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_event_export(void)
{
	tgif_event(my_provider_event_export, tgif_arg_list(tgif_arg_u32(2)));
}

tgif_static_event(my_provider_event_struct_literal, "myprovider", "myeventstructliteral", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_struct("structliteral",
			tgif_struct_literal(
				tgif_field_list(
					tgif_field_u32("x", tgif_attr_list()),
					tgif_field_s64("y", tgif_attr_list()),
				),
				tgif_attr_list()
			)
		),
		tgif_field_u8("z", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_struct_literal(void)
{
	tgif_event_cond(my_provider_event_struct_literal) {
		tgif_arg_define_vec(mystruct, tgif_arg_list(tgif_arg_u32(21), tgif_arg_s64(22)));
		tgif_event_call(my_provider_event_struct_literal, tgif_arg_list(tgif_arg_struct(&mystruct), tgif_arg_u8(55)));
	}
}

static tgif_define_struct(mystructdef,
	tgif_field_list(
		tgif_field_u32("x", tgif_attr_list()),
		tgif_field_s64("y", tgif_attr_list()),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_struct, "myprovider", "myeventstruct", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_struct("struct", &mystructdef),
		tgif_field_u8("z", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_struct(void)
{
	tgif_event_cond(my_provider_event_struct) {
		tgif_arg_define_vec(mystruct, tgif_arg_list(tgif_arg_u32(21), tgif_arg_s64(22)));
		tgif_event_call(my_provider_event_struct, tgif_arg_list(tgif_arg_struct(&mystruct), tgif_arg_u8(55)));
	}
}

tgif_static_event(my_provider_event_array, "myprovider", "myarray", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_array("arr", tgif_elem(tgif_type_u32(tgif_attr_list())), 3, tgif_attr_list()),
		tgif_field_s64("v", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_array(void)
{
	tgif_event_cond(my_provider_event_array) {
		tgif_arg_define_vec(myarray, tgif_arg_list(tgif_arg_u32(1), tgif_arg_u32(2), tgif_arg_u32(3)));
		tgif_event_call(my_provider_event_array, tgif_arg_list(tgif_arg_array(&myarray), tgif_arg_s64(42)));
	}
}

tgif_static_event(my_provider_event_vla, "myprovider", "myvla", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_vla("vla", tgif_elem(tgif_type_u32(tgif_attr_list())), tgif_attr_list()),
		tgif_field_s64("v", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_vla(void)
{
	tgif_event_cond(my_provider_event_vla) {
		tgif_arg_define_vec(myvla, tgif_arg_list(tgif_arg_u32(1), tgif_arg_u32(2), tgif_arg_u32(3)));
		tgif_event_call(my_provider_event_vla, tgif_arg_list(tgif_arg_vla(&myvla), tgif_arg_s64(42)));
	}
}

/* 1D array visitor */

struct app_visitor_ctx {
	const uint32_t *ptr;
	uint32_t length;
};

static
enum tgif_visitor_status test_visitor(const struct tgif_tracer_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_visitor_ctx *ctx = (struct app_visitor_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		const struct tgif_arg elem = tgif_arg_u32(ctx->ptr[i]);

		if (tracer_ctx->write_elem(tracer_ctx, &elem) != TGIF_VISITOR_STATUS_OK)
			return TGIF_VISITOR_STATUS_ERROR;
	}
	return TGIF_VISITOR_STATUS_OK;
}

static uint32_t testarray[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

tgif_static_event(my_provider_event_vla_visitor, "myprovider", "myvlavisit", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_vla_visitor("vlavisit", tgif_elem(tgif_type_u32(tgif_attr_list())), test_visitor, tgif_attr_list()),
		tgif_field_s64("v", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_vla_visitor(void)
{
	tgif_event_cond(my_provider_event_vla_visitor) {
		struct app_visitor_ctx ctx = {
			.ptr = testarray,
			.length = TGIF_ARRAY_SIZE(testarray),
		};
		tgif_event_call(my_provider_event_vla_visitor, tgif_arg_list(tgif_arg_vla_visitor(&ctx), tgif_arg_s64(42)));
	}
}

/* 2D array visitor */

struct app_visitor_2d_inner_ctx {
	const uint32_t *ptr;
	uint32_t length;
};

static
enum tgif_visitor_status test_inner_visitor(const struct tgif_tracer_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_visitor_2d_inner_ctx *ctx = (struct app_visitor_2d_inner_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		const struct tgif_arg elem = tgif_arg_u32(ctx->ptr[i]);

		if (tracer_ctx->write_elem(tracer_ctx, &elem) != TGIF_VISITOR_STATUS_OK)
			return TGIF_VISITOR_STATUS_ERROR;
	}
	return TGIF_VISITOR_STATUS_OK;
}

struct app_visitor_2d_outer_ctx {
	const uint32_t (*ptr)[2];
	uint32_t length;
};

static
enum tgif_visitor_status test_outer_visitor(const struct tgif_tracer_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_visitor_2d_outer_ctx *ctx = (struct app_visitor_2d_outer_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		struct app_visitor_2d_inner_ctx inner_ctx = {
			.ptr = ctx->ptr[i],
			.length = 2,
		};
		const struct tgif_arg elem = tgif_arg_vla_visitor(&inner_ctx);
		if (tracer_ctx->write_elem(tracer_ctx, &elem) != TGIF_VISITOR_STATUS_OK)
			return TGIF_VISITOR_STATUS_ERROR;
	}
	return TGIF_VISITOR_STATUS_OK;
}

static uint32_t testarray2d[][2] = {
	{ 1, 2 },
	{ 33, 44 },
	{ 55, 66 },
};

tgif_static_event(my_provider_event_vla_visitor2d, "myprovider", "myvlavisit2d", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_vla_visitor("vlavisit2d",
			tgif_elem(
				tgif_type_vla_visitor(
					tgif_elem(tgif_type_u32(tgif_attr_list())),
					test_inner_visitor,
					tgif_attr_list())
			), test_outer_visitor, tgif_attr_list()),
		tgif_field_s64("v", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_vla_visitor_2d(void)
{
	tgif_event_cond(my_provider_event_vla_visitor2d) {
		struct app_visitor_2d_outer_ctx ctx = {
			.ptr = testarray2d,
			.length = TGIF_ARRAY_SIZE(testarray2d),
		};
		tgif_event_call(my_provider_event_vla_visitor2d, tgif_arg_list(tgif_arg_vla_visitor(&ctx), tgif_arg_s64(42)));
	}
}

tgif_static_event(my_provider_event_dynamic_basic,
	"myprovider", "mydynamicbasic", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
);

static
void test_dynamic_basic_type(void)
{
	tgif_event(my_provider_event_dynamic_basic,
		tgif_arg_list(tgif_arg_dynamic_s16(-33, tgif_attr_list())));
}

tgif_static_event(my_provider_event_dynamic_vla,
	"myprovider", "mydynamicvla", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
);

static
void test_dynamic_vla(void)
{
	tgif_arg_dynamic_define_vec(myvla,
		tgif_arg_list(
			tgif_arg_dynamic_u32(1, tgif_attr_list()),
			tgif_arg_dynamic_u32(2, tgif_attr_list()),
			tgif_arg_dynamic_u32(3, tgif_attr_list()),
		),
		tgif_attr_list()
	);
	tgif_event(my_provider_event_dynamic_vla,
		tgif_arg_list(tgif_arg_dynamic_vla(&myvla)));
}

tgif_static_event(my_provider_event_dynamic_null,
	"myprovider", "mydynamicnull", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
);

static
void test_dynamic_null(void)
{
	tgif_event(my_provider_event_dynamic_null,
		tgif_arg_list(tgif_arg_dynamic_null(tgif_attr_list())));
}

tgif_static_event(my_provider_event_dynamic_struct,
	"myprovider", "mydynamicstruct", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
);

static
void test_dynamic_struct(void)
{
	tgif_arg_dynamic_define_struct(mystruct,
		tgif_arg_list(
			tgif_arg_dynamic_field("a", tgif_arg_dynamic_u32(43, tgif_attr_list())),
			tgif_arg_dynamic_field("b", tgif_arg_dynamic_string("zzz", tgif_attr_list())),
			tgif_arg_dynamic_field("c", tgif_arg_dynamic_null(tgif_attr_list())),
		),
		tgif_attr_list()
	);

	tgif_event(my_provider_event_dynamic_struct,
		tgif_arg_list(tgif_arg_dynamic_struct(&mystruct)));
}

tgif_static_event(my_provider_event_dynamic_nested_struct,
	"myprovider", "mydynamicnestedstruct", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
);

static
void test_dynamic_nested_struct(void)
{
	tgif_arg_dynamic_define_struct(nested,
		tgif_arg_list(
			tgif_arg_dynamic_field("a", tgif_arg_dynamic_u32(43, tgif_attr_list())),
			tgif_arg_dynamic_field("b", tgif_arg_dynamic_u8(55, tgif_attr_list())),
		),
		tgif_attr_list()
	);
	tgif_arg_dynamic_define_struct(nested2,
		tgif_arg_list(
			tgif_arg_dynamic_field("aa", tgif_arg_dynamic_u64(128, tgif_attr_list())),
			tgif_arg_dynamic_field("bb", tgif_arg_dynamic_u16(1, tgif_attr_list())),
		),
		tgif_attr_list()
	);
	tgif_arg_dynamic_define_struct(mystruct,
		tgif_arg_list(
			tgif_arg_dynamic_field("nested", tgif_arg_dynamic_struct(&nested)),
			tgif_arg_dynamic_field("nested2", tgif_arg_dynamic_struct(&nested2)),
		),
		tgif_attr_list()
	);
	tgif_event(my_provider_event_dynamic_nested_struct,
		tgif_arg_list(tgif_arg_dynamic_struct(&mystruct)));
}

tgif_static_event(my_provider_event_dynamic_vla_struct,
	"myprovider", "mydynamicvlastruct", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
);

static
void test_dynamic_vla_struct(void)
{
	tgif_arg_dynamic_define_struct(nested,
		tgif_arg_list(
			tgif_arg_dynamic_field("a", tgif_arg_dynamic_u32(43, tgif_attr_list())),
			tgif_arg_dynamic_field("b", tgif_arg_dynamic_u8(55, tgif_attr_list())),
		),
		tgif_attr_list()
	);
	tgif_arg_dynamic_define_vec(myvla,
		tgif_arg_list(
			tgif_arg_dynamic_struct(&nested),
			tgif_arg_dynamic_struct(&nested),
			tgif_arg_dynamic_struct(&nested),
			tgif_arg_dynamic_struct(&nested),
		),
		tgif_attr_list()
	);
	tgif_event(my_provider_event_dynamic_vla_struct,
		tgif_arg_list(tgif_arg_dynamic_vla(&myvla)));
}

tgif_static_event(my_provider_event_dynamic_struct_vla,
	"myprovider", "mydynamicstructvla", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
);

static
void test_dynamic_struct_vla(void)
{
	tgif_arg_dynamic_define_vec(myvla,
		tgif_arg_list(
			tgif_arg_dynamic_u32(1, tgif_attr_list()),
			tgif_arg_dynamic_u32(2, tgif_attr_list()),
			tgif_arg_dynamic_u32(3, tgif_attr_list()),
		),
		tgif_attr_list()
	);
	tgif_arg_dynamic_define_vec(myvla2,
		tgif_arg_list(
			tgif_arg_dynamic_u32(4, tgif_attr_list()),
			tgif_arg_dynamic_u64(5, tgif_attr_list()),
			tgif_arg_dynamic_u32(6, tgif_attr_list()),
		),
		tgif_attr_list()
	);
	tgif_arg_dynamic_define_struct(mystruct,
		tgif_arg_list(
			tgif_arg_dynamic_field("a", tgif_arg_dynamic_vla(&myvla)),
			tgif_arg_dynamic_field("b", tgif_arg_dynamic_vla(&myvla2)),
		),
		tgif_attr_list()
	);
	tgif_event(my_provider_event_dynamic_struct_vla,
		tgif_arg_list(tgif_arg_dynamic_struct(&mystruct)));
}

tgif_static_event(my_provider_event_dynamic_nested_vla,
	"myprovider", "mydynamicnestedvla", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
);

static
void test_dynamic_nested_vla(void)
{
	tgif_arg_dynamic_define_vec(nestedvla,
		tgif_arg_list(
			tgif_arg_dynamic_u32(1, tgif_attr_list()),
			tgif_arg_dynamic_u16(2, tgif_attr_list()),
			tgif_arg_dynamic_u32(3, tgif_attr_list()),
		),
		tgif_attr_list()
	);
	tgif_arg_dynamic_define_vec(nestedvla2,
		tgif_arg_list(
			tgif_arg_dynamic_u8(4, tgif_attr_list()),
			tgif_arg_dynamic_u32(5, tgif_attr_list()),
			tgif_arg_dynamic_u32(6, tgif_attr_list()),
		),
		tgif_attr_list()
	);
	tgif_arg_dynamic_define_vec(myvla,
		tgif_arg_list(
			tgif_arg_dynamic_vla(&nestedvla),
			tgif_arg_dynamic_vla(&nestedvla2),
		),
		tgif_attr_list()
	);
	tgif_event(my_provider_event_dynamic_nested_vla,
		tgif_arg_list(tgif_arg_dynamic_vla(&myvla)));
}

tgif_static_event_variadic(my_provider_event_variadic,
	"myprovider", "myvariadicevent", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(),
	tgif_attr_list()
);

static
void test_variadic(void)
{
	tgif_event_variadic(my_provider_event_variadic,
		tgif_arg_list(),
		tgif_arg_list(
			tgif_arg_dynamic_field("a", tgif_arg_dynamic_u32(55, tgif_attr_list())),
			tgif_arg_dynamic_field("b", tgif_arg_dynamic_s8(-4, tgif_attr_list())),
		),
		tgif_attr_list()
	);
}

tgif_static_event_variadic(my_provider_event_static_variadic,
	"myprovider", "mystaticvariadicevent", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_u32("abc", tgif_attr_list()),
		tgif_field_u16("def", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_static_variadic(void)
{
	tgif_event_variadic(my_provider_event_static_variadic,
		tgif_arg_list(
			tgif_arg_u32(1),
			tgif_arg_u16(2),
		),
		tgif_arg_list(
			tgif_arg_dynamic_field("a", tgif_arg_dynamic_u32(55, tgif_attr_list())),
			tgif_arg_dynamic_field("b", tgif_arg_dynamic_s8(-4, tgif_attr_list())),
		),
		tgif_attr_list()
	);
}

tgif_static_event(my_provider_event_bool, "myprovider", "myeventbool", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_bool("a_false", tgif_attr_list()),
		tgif_field_bool("b_true", tgif_attr_list()),
		tgif_field_bool("c_true", tgif_attr_list()),
		tgif_field_bool("d_true", tgif_attr_list()),
		tgif_field_bool("e_true", tgif_attr_list()),
		tgif_field_bool("f_false", tgif_attr_list()),
		tgif_field_bool("g_true", tgif_attr_list()),
	),
	tgif_attr_list()
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

	tgif_event(my_provider_event_bool,
		tgif_arg_list(
			tgif_arg_bool(a),
			tgif_arg_bool(b),
			tgif_arg_bool(c),
			tgif_arg_bool(d),
			tgif_arg_bool(e),
			tgif_arg_bool(f),
			tgif_arg_bool(g),
		)
	);
}

tgif_static_event_variadic(my_provider_event_dynamic_bool,
	"myprovider", "mydynamicbool", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(),
	tgif_attr_list()
);

static
void test_dynamic_bool(void)
{
	tgif_event_variadic(my_provider_event_dynamic_bool,
		tgif_arg_list(),
		tgif_arg_list(
			tgif_arg_dynamic_field("a_true", tgif_arg_dynamic_bool(55, tgif_attr_list())),
			tgif_arg_dynamic_field("b_true", tgif_arg_dynamic_bool(-4, tgif_attr_list())),
			tgif_arg_dynamic_field("c_false", tgif_arg_dynamic_bool(0, tgif_attr_list())),
			tgif_arg_dynamic_field("d_true", tgif_arg_dynamic_bool(256, tgif_attr_list())),
		),
		tgif_attr_list()
	);
}

tgif_static_event(my_provider_event_dynamic_vla_visitor,
	"myprovider", "mydynamicvlavisitor", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
);

struct app_dynamic_vla_visitor_ctx {
	const uint32_t *ptr;
	uint32_t length;
};

static
enum tgif_visitor_status test_dynamic_vla_visitor(const struct tgif_tracer_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_dynamic_vla_visitor_ctx *ctx = (struct app_dynamic_vla_visitor_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		const struct tgif_arg elem = tgif_arg_dynamic_u32(ctx->ptr[i], tgif_attr_list());
		if (tracer_ctx->write_elem(tracer_ctx, &elem) != TGIF_VISITOR_STATUS_OK)
			return TGIF_VISITOR_STATUS_ERROR;
	}
	return TGIF_VISITOR_STATUS_OK;
}

static uint32_t testarray_dynamic_vla[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

static
void test_dynamic_vla_with_visitor(void)
{
	tgif_event_cond(my_provider_event_dynamic_vla_visitor) {
		struct app_dynamic_vla_visitor_ctx ctx = {
			.ptr = testarray_dynamic_vla,
			.length = TGIF_ARRAY_SIZE(testarray_dynamic_vla),
		};
		tgif_event_call(my_provider_event_dynamic_vla_visitor,
			tgif_arg_list(
				tgif_arg_dynamic_vla_visitor(test_dynamic_vla_visitor, &ctx, tgif_attr_list())
			)
		);
	}
}

tgif_static_event(my_provider_event_dynamic_struct_visitor,
	"myprovider", "mydynamicstructvisitor", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_dynamic("dynamic"),
	),
	tgif_attr_list()
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
enum tgif_visitor_status test_dynamic_struct_visitor(const struct tgif_tracer_dynamic_struct_visitor_ctx *tracer_ctx, void *_ctx)
{
	struct app_dynamic_struct_visitor_ctx *ctx = (struct app_dynamic_struct_visitor_ctx *) _ctx;
	uint32_t length = ctx->length, i;

	for (i = 0; i < length; i++) {
		struct tgif_arg_dynamic_field dynamic_field = {
			.field_name = ctx->ptr[i].name,
			.elem = tgif_arg_dynamic_u32(ctx->ptr[i].value, tgif_attr_list()),
		};
		if (tracer_ctx->write_field(tracer_ctx, &dynamic_field) != TGIF_VISITOR_STATUS_OK)
			return TGIF_VISITOR_STATUS_ERROR;
	}
	return TGIF_VISITOR_STATUS_OK;
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
	tgif_event_cond(my_provider_event_dynamic_struct_visitor) {
		struct app_dynamic_struct_visitor_ctx ctx = {
			.ptr = testarray_dynamic_struct,
			.length = TGIF_ARRAY_SIZE(testarray_dynamic_struct),
		};
		tgif_event_call(my_provider_event_dynamic_struct_visitor,
			tgif_arg_list(
				tgif_arg_dynamic_struct_visitor(test_dynamic_struct_visitor, &ctx, tgif_attr_list())
			)
		);
	}
}

tgif_static_event(my_provider_event_user_attribute, "myprovider", "myevent_user_attribute", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_u32("abc", tgif_attr_list()),
		tgif_field_s64("def", tgif_attr_list()),
	),
	tgif_attr_list(
		tgif_attr("user_attribute_a", tgif_attr_string("val1")),
		tgif_attr("user_attribute_b", tgif_attr_string("val2")),
	)
);

static
void test_event_user_attribute(void)
{
	tgif_event(my_provider_event_user_attribute, tgif_arg_list(tgif_arg_u32(1), tgif_arg_s64(2)));
}

tgif_static_event(my_provider_field_user_attribute, "myprovider", "myevent_field_attribute", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_u32("abc",
			tgif_attr_list(
				tgif_attr("user_attribute_a", tgif_attr_string("val1")),
				tgif_attr("user_attribute_b", tgif_attr_u32(2)),
			)
		),
		tgif_field_s64("def",
			tgif_attr_list(
				tgif_attr("user_attribute_c", tgif_attr_string("val3")),
				tgif_attr("user_attribute_d", tgif_attr_s64(-5)),
			)
		),
	),
	tgif_attr_list()
);

static
void test_field_user_attribute(void)
{
	tgif_event(my_provider_field_user_attribute, tgif_arg_list(tgif_arg_u32(1), tgif_arg_s64(2)));
}

tgif_static_event_variadic(my_provider_event_variadic_attr,
	"myprovider", "myvariadiceventattr", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(),
	tgif_attr_list()
);

static
void test_variadic_attr(void)
{
	tgif_event_variadic(my_provider_event_variadic_attr,
		tgif_arg_list(),
		tgif_arg_list(
			tgif_arg_dynamic_field("a",
				tgif_arg_dynamic_u32(55,
					tgif_attr_list(
						tgif_attr("user_attribute_c", tgif_attr_string("valX")),
						tgif_attr("user_attribute_d", tgif_attr_u8(55)),
					)
				)
			),
			tgif_arg_dynamic_field("b",
				tgif_arg_dynamic_s8(-4,
					tgif_attr_list(
						tgif_attr("X", tgif_attr_u8(1)),
						tgif_attr("Y", tgif_attr_s8(2)),
					)
				)
			),
		),
		tgif_attr_list()
	);
}

tgif_static_event_variadic(my_provider_event_variadic_vla_attr,
	"myprovider", "myvariadiceventvlaattr", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(),
	tgif_attr_list()
);

static
void test_variadic_vla_attr(void)
{
	tgif_arg_dynamic_define_vec(myvla,
		tgif_arg_list(
			tgif_arg_dynamic_u32(1,
				tgif_attr_list(
					tgif_attr("Z", tgif_attr_u8(0)),
					tgif_attr("A", tgif_attr_u8(123)),
				)
			),
			tgif_arg_dynamic_u32(2, tgif_attr_list()),
			tgif_arg_dynamic_u32(3, tgif_attr_list()),
		),
		tgif_attr_list(
			tgif_attr("X", tgif_attr_u8(1)),
			tgif_attr("Y", tgif_attr_u8(2)),
		)
	);
	tgif_event_variadic(my_provider_event_variadic_vla_attr,
		tgif_arg_list(),
		tgif_arg_list(
			tgif_arg_dynamic_field("a", tgif_arg_dynamic_vla(&myvla)),
		),
		tgif_attr_list()
	);
}

tgif_static_event_variadic(my_provider_event_variadic_struct_attr,
	"myprovider", "myvariadiceventstructattr", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(),
	tgif_attr_list()
);

static
void test_variadic_struct_attr(void)
{
	tgif_event_cond(my_provider_event_variadic_struct_attr) {
		tgif_arg_dynamic_define_struct(mystruct,
			tgif_arg_list(
				tgif_arg_dynamic_field("a",
					tgif_arg_dynamic_u32(43,
						tgif_attr_list(
							tgif_attr("A", tgif_attr_bool(true)),
						)
					)
				),
				tgif_arg_dynamic_field("b", tgif_arg_dynamic_u8(55, tgif_attr_list())),
			),
			tgif_attr_list(
				tgif_attr("X", tgif_attr_u8(1)),
				tgif_attr("Y", tgif_attr_u8(2)),
			)
		);
		tgif_event_call_variadic(my_provider_event_variadic_struct_attr,
			tgif_arg_list(),
			tgif_arg_list(
				tgif_arg_dynamic_field("a", tgif_arg_dynamic_struct(&mystruct)),
			),
			tgif_attr_list()
		);
	}
}

tgif_static_event(my_provider_event_float, "myprovider", "myeventfloat", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
#if __HAVE_FLOAT16
		tgif_field_float_binary16("binary16", tgif_attr_list()),
		tgif_field_float_binary16_le("binary16_le", tgif_attr_list()),
		tgif_field_float_binary16_be("binary16_be", tgif_attr_list()),
#endif
#if __HAVE_FLOAT32
		tgif_field_float_binary32("binary32", tgif_attr_list()),
		tgif_field_float_binary32_le("binary32_le", tgif_attr_list()),
		tgif_field_float_binary32_be("binary32_be", tgif_attr_list()),
#endif
#if __HAVE_FLOAT64
		tgif_field_float_binary64("binary64", tgif_attr_list()),
		tgif_field_float_binary64_le("binary64_le", tgif_attr_list()),
		tgif_field_float_binary64_be("binary64_be", tgif_attr_list()),
#endif
#if __HAVE_FLOAT128
		tgif_field_float_binary128("binary128", tgif_attr_list()),
		tgif_field_float_binary128_le("binary128_le", tgif_attr_list()),
		tgif_field_float_binary128_be("binary128_be", tgif_attr_list()),
#endif
	),
	tgif_attr_list()
);

static
void test_float(void)
{
#if __HAVE_FLOAT16
	union {
		_Float16 f;
		uint16_t u;
	} float16 = {
		.f = 1.1,
	};
#endif
#if __HAVE_FLOAT32
	union {
		_Float32 f;
		uint32_t u;
	} float32 = {
		.f = 2.2,
	};
#endif
#if __HAVE_FLOAT64
	union {
		_Float64 f;
		uint64_t u;
	} float64 = {
		.f = 3.3,
	};
#endif
#if __HAVE_FLOAT128
	union {
		_Float128 f;
		char arr[16];
	} float128 = {
		.f = 4.4,
	};
#endif

#if __HAVE_FLOAT16
	float16.u = tgif_bswap_16(float16.u);
#endif
#if __HAVE_FLOAT32
	float32.u = tgif_bswap_32(float32.u);
#endif
#if __HAVE_FLOAT64
	float64.u = tgif_bswap_64(float64.u);
#endif
#if __HAVE_FLOAT128
	tgif_bswap_128p(float128.arr);
#endif

	tgif_event(my_provider_event_float,
		tgif_arg_list(
#if __HAVE_FLOAT16
			tgif_arg_float_binary16(1.1),
# if TGIF_FLOAT_WORD_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_float_binary16(1.1),
			tgif_arg_float_binary16(float16.f),
# else
			tgif_arg_float_binary16(float16.f),
			tgif_arg_float_binary16(1.1),
# endif
#endif
#if __HAVE_FLOAT32
			tgif_arg_float_binary32(2.2),
# if TGIF_FLOAT_WORD_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_float_binary32(2.2),
			tgif_arg_float_binary32(float32.f),
# else
			tgif_arg_float_binary32(float32.f),
			tgif_arg_float_binary32(2.2),
# endif
#endif
#if __HAVE_FLOAT64
			tgif_arg_float_binary64(3.3),
# if TGIF_FLOAT_WORD_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_float_binary64(3.3),
			tgif_arg_float_binary64(float64.f),
# else
			tgif_arg_float_binary64(float64.f),
			tgif_arg_float_binary64(3.3),
# endif
#endif
#if __HAVE_FLOAT128
			tgif_arg_float_binary128(4.4),
# if TGIF_FLOAT_WORD_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_float_binary128(4.4),
			tgif_arg_float_binary128(float128.f),
# else
			tgif_arg_float_binary128(float128.f),
			tgif_arg_float_binary128(4.4),
# endif
#endif
		)
	);
}

tgif_static_event_variadic(my_provider_event_variadic_float,
	"myprovider", "myvariadicfloat", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(),
	tgif_attr_list()
);

static
void test_variadic_float(void)
{
#if __HAVE_FLOAT16
	union {
		_Float16 f;
		uint16_t u;
	} float16 = {
		.f = 1.1,
	};
#endif
#if __HAVE_FLOAT32
	union {
		_Float32 f;
		uint32_t u;
	} float32 = {
		.f = 2.2,
	};
#endif
#if __HAVE_FLOAT64
	union {
		_Float64 f;
		uint64_t u;
	} float64 = {
		.f = 3.3,
	};
#endif
#if __HAVE_FLOAT128
	union {
		_Float128 f;
		char arr[16];
	} float128 = {
		.f = 4.4,
	};
#endif

#if __HAVE_FLOAT16
	float16.u = tgif_bswap_16(float16.u);
#endif
#if __HAVE_FLOAT32
	float32.u = tgif_bswap_32(float32.u);
#endif
#if __HAVE_FLOAT64
	float64.u = tgif_bswap_64(float64.u);
#endif
#if __HAVE_FLOAT128
	tgif_bswap_128p(float128.arr);
#endif

	tgif_event_variadic(my_provider_event_variadic_float,
		tgif_arg_list(),
		tgif_arg_list(
#if __HAVE_FLOAT16
			tgif_arg_dynamic_field("binary16", tgif_arg_dynamic_float_binary16(1.1, tgif_attr_list())),
# if TGIF_FLOAT_WORD_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_dynamic_field("binary16_le", tgif_arg_dynamic_float_binary16_le(1.1, tgif_attr_list())),
			tgif_arg_dynamic_field("binary16_be", tgif_arg_dynamic_float_binary16_be(float16.f, tgif_attr_list())),
# else
			tgif_arg_dynamic_field("binary16_le", tgif_arg_dynamic_float_binary16_le(float16.f, tgif_attr_list())),
			tgif_arg_dynamic_field("binary16_be", tgif_arg_dynamic_float_binary16_be(1.1, tgif_attr_list())),
# endif
#endif
#if __HAVE_FLOAT32
			tgif_arg_dynamic_field("binary32", tgif_arg_dynamic_float_binary32(2.2, tgif_attr_list())),
# if TGIF_FLOAT_WORD_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_dynamic_field("binary32_le", tgif_arg_dynamic_float_binary32_le(2.2, tgif_attr_list())),
			tgif_arg_dynamic_field("binary32_be", tgif_arg_dynamic_float_binary32_be(float32.f, tgif_attr_list())),
# else
			tgif_arg_dynamic_field("binary32_le", tgif_arg_dynamic_float_binary32_le(float32.f, tgif_attr_list())),
			tgif_arg_dynamic_field("binary32_be", tgif_arg_dynamic_float_binary32_be(2.2, tgif_attr_list())),
# endif
#endif
#if __HAVE_FLOAT64
			tgif_arg_dynamic_field("binary64", tgif_arg_dynamic_float_binary64(3.3, tgif_attr_list())),
# if TGIF_FLOAT_WORD_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_dynamic_field("binary64_le", tgif_arg_dynamic_float_binary64_le(3.3, tgif_attr_list())),
			tgif_arg_dynamic_field("binary64_be", tgif_arg_dynamic_float_binary64_be(float64.f, tgif_attr_list())),
# else
			tgif_arg_dynamic_field("binary64_le", tgif_arg_dynamic_float_binary64_le(float64.f, tgif_attr_list())),
			tgif_arg_dynamic_field("binary64_be", tgif_arg_dynamic_float_binary64_be(3.3, tgif_attr_list())),
# endif
#endif
#if __HAVE_FLOAT128
			tgif_arg_dynamic_field("binary128", tgif_arg_dynamic_float_binary128(4.4, tgif_attr_list())),
# if TGIF_FLOAT_WORD_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_dynamic_field("binary128_le", tgif_arg_dynamic_float_binary128_le(4.4, tgif_attr_list())),
			tgif_arg_dynamic_field("binary128_be", tgif_arg_dynamic_float_binary128_be(float128.f, tgif_attr_list())),
# else
			tgif_arg_dynamic_field("binary128_le", tgif_arg_dynamic_float_binary128_le(float128.f, tgif_attr_list())),
			tgif_arg_dynamic_field("binary128_be", tgif_arg_dynamic_float_binary128_be(4.4, tgif_attr_list())),
# endif
#endif
		),
		tgif_attr_list()
	);
}

static tgif_define_enum(myenum,
	tgif_enum_mapping_list(
		tgif_enum_mapping_range("one-ten", 1, 10),
		tgif_enum_mapping_range("100-200", 100, 200),
		tgif_enum_mapping_value("200", 200),
		tgif_enum_mapping_value("300", 300),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_enum, "myprovider", "myeventenum", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_enum("5", &myenum, tgif_elem(tgif_type_u32(tgif_attr_list()))),
		tgif_field_enum("400", &myenum, tgif_elem(tgif_type_u64(tgif_attr_list()))),
		tgif_field_enum("200", &myenum, tgif_elem(tgif_type_u8(tgif_attr_list()))),
		tgif_field_enum("-100", &myenum, tgif_elem(tgif_type_s8(tgif_attr_list()))),
		tgif_field_enum("6_be", &myenum, tgif_elem(tgif_type_u32_be(tgif_attr_list()))),
		tgif_field_enum("6_le", &myenum, tgif_elem(tgif_type_u32_le(tgif_attr_list()))),
	),
	tgif_attr_list()
);

static
void test_enum(void)
{
	tgif_event(my_provider_event_enum,
		tgif_arg_list(
			tgif_arg_u32(5),
			tgif_arg_u64(400),
			tgif_arg_u8(200),
			tgif_arg_s8(-100),
#if TGIF_BYTE_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_u32(tgif_bswap_32(6)),
			tgif_arg_u32(6),
#else
			tgif_arg_u32(6),
			tgif_arg_u32(tgif_bswap_32(6)),
#endif
		)
	);
}

/* A bitmap enum maps bits to labels. */
static tgif_define_enum_bitmap(myenum_bitmap,
	tgif_enum_bitmap_mapping_list(
		tgif_enum_bitmap_mapping_value("0", 0),
		tgif_enum_bitmap_mapping_range("1-2", 1, 2),
		tgif_enum_bitmap_mapping_range("2-4", 2, 4),
		tgif_enum_bitmap_mapping_value("3", 3),
		tgif_enum_bitmap_mapping_value("30", 30),
		tgif_enum_bitmap_mapping_value("63", 63),
		tgif_enum_bitmap_mapping_range("158-160", 158, 160),
		tgif_enum_bitmap_mapping_value("159", 159),
		tgif_enum_bitmap_mapping_range("500-700", 500, 700),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_enum_bitmap, "myprovider", "myeventenumbitmap", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_enum_bitmap("bit_0", &myenum_bitmap, tgif_elem(tgif_type_u32(tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_1", &myenum_bitmap, tgif_elem(tgif_type_u32(tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_2", &myenum_bitmap, tgif_elem(tgif_type_u8(tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_3", &myenum_bitmap, tgif_elem(tgif_type_u8(tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_30", &myenum_bitmap, tgif_elem(tgif_type_u32(tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_31", &myenum_bitmap, tgif_elem(tgif_type_u32(tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_63", &myenum_bitmap, tgif_elem(tgif_type_u64(tgif_attr_list()))),
		tgif_field_enum_bitmap("bits_1+63", &myenum_bitmap, tgif_elem(tgif_type_u64(tgif_attr_list()))),
		tgif_field_enum_bitmap("byte_bit_2", &myenum_bitmap, tgif_elem(tgif_type_byte(tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_159", &myenum_bitmap,
			tgif_elem(tgif_type_array(tgif_elem(tgif_type_u32(tgif_attr_list())), 5, tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_159", &myenum_bitmap,
			tgif_elem(tgif_type_vla(tgif_elem(tgif_type_u32(tgif_attr_list())), tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_2_be", &myenum_bitmap, tgif_elem(tgif_type_u32_be(tgif_attr_list()))),
		tgif_field_enum_bitmap("bit_2_le", &myenum_bitmap, tgif_elem(tgif_type_u32_le(tgif_attr_list()))),
	),
	tgif_attr_list()
);

static
void test_enum_bitmap(void)
{
	tgif_event_cond(my_provider_event_enum_bitmap) {
		tgif_arg_define_vec(myarray,
			tgif_arg_list(
				tgif_arg_u32(0),
				tgif_arg_u32(0),
				tgif_arg_u32(0),
				tgif_arg_u32(0),
				tgif_arg_u32(0x80000000),	/* bit 159 */
			)
		);
		tgif_event_call(my_provider_event_enum_bitmap,
			tgif_arg_list(
				tgif_arg_u32(1U << 0),
				tgif_arg_u32(1U << 1),
				tgif_arg_u8(1U << 2),
				tgif_arg_u8(1U << 3),
				tgif_arg_u32(1U << 30),
				tgif_arg_u32(1U << 31),
				tgif_arg_u64(1ULL << 63),
				tgif_arg_u64((1ULL << 1) | (1ULL << 63)),
				tgif_arg_byte(1U << 2),
				tgif_arg_array(&myarray),
				tgif_arg_vla(&myarray),
#if TGIF_BYTE_ORDER == TGIF_LITTLE_ENDIAN
				tgif_arg_u32(tgif_bswap_32(1U << 2)),
				tgif_arg_u32(1U << 2),
#else
				tgif_arg_u32(1U << 2),
				tgif_arg_u32(tgif_bswap_32(1U << 2)),
#endif
			)
		);
	}
}

tgif_static_event_variadic(my_provider_event_blob, "myprovider", "myeventblob", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_byte("blobfield", tgif_attr_list()),
		tgif_field_array("arrayblob", tgif_elem(tgif_type_byte(tgif_attr_list())), 3, tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_blob(void)
{
	tgif_event_cond(my_provider_event_blob) {
		tgif_arg_define_vec(myarray, tgif_arg_list(tgif_arg_byte(1), tgif_arg_byte(2), tgif_arg_byte(3)));
		tgif_arg_dynamic_define_vec(myvla,
			tgif_arg_list(
				tgif_arg_dynamic_byte(0x22, tgif_attr_list()),
				tgif_arg_dynamic_byte(0x33, tgif_attr_list()),
			),
			tgif_attr_list()
		);
		tgif_event_call_variadic(my_provider_event_blob,
			tgif_arg_list(
				tgif_arg_byte(0x55),
				tgif_arg_array(&myarray),
			),
			tgif_arg_list(
				tgif_arg_dynamic_field("varblobfield",
					tgif_arg_dynamic_byte(0x55, tgif_attr_list())
				),
				tgif_arg_dynamic_field("varblobvla", tgif_arg_dynamic_vla(&myvla)),
			),
			tgif_attr_list()
		);
	}
}

tgif_static_event_variadic(my_provider_event_format_string,
	"myprovider", "myeventformatstring", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_string("fmt", tgif_attr_list()),
	),
	tgif_attr_list(
		tgif_attr("lang.c.format_string", tgif_attr_bool(true)),
	)
);

static
void test_fmt_string(void)
{
	tgif_event_cond(my_provider_event_format_string) {
		tgif_arg_dynamic_define_vec(args,
			tgif_arg_list(
				tgif_arg_dynamic_string("blah", tgif_attr_list()),
				tgif_arg_dynamic_s32(123, tgif_attr_list()),
			),
			tgif_attr_list()
		);
		tgif_event_call_variadic(my_provider_event_format_string,
			tgif_arg_list(
				tgif_arg_string("This is a formatted string with str: %s int: %d"),
			),
			tgif_arg_list(
				tgif_arg_dynamic_field("arguments", tgif_arg_dynamic_vla(&args)),
			),
			tgif_attr_list()
		);
	}
}

tgif_static_event_variadic(my_provider_event_endian, "myprovider", "myevent_endian", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_u16_le("u16_le", tgif_attr_list()),
		tgif_field_u32_le("u32_le", tgif_attr_list()),
		tgif_field_u64_le("u64_le", tgif_attr_list()),
		tgif_field_s16_le("s16_le", tgif_attr_list()),
		tgif_field_s32_le("s32_le", tgif_attr_list()),
		tgif_field_s64_le("s64_le", tgif_attr_list()),
		tgif_field_u16_be("u16_be", tgif_attr_list()),
		tgif_field_u32_be("u32_be", tgif_attr_list()),
		tgif_field_u64_be("u64_be", tgif_attr_list()),
		tgif_field_s16_be("s16_be", tgif_attr_list()),
		tgif_field_s32_be("s32_be", tgif_attr_list()),
		tgif_field_s64_be("s64_be", tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_endian(void)
{
	tgif_event_variadic(my_provider_event_endian,
		tgif_arg_list(
#if TGIF_BYTE_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_u16(1),
			tgif_arg_u32(1),
			tgif_arg_u64(1),
			tgif_arg_s16(1),
			tgif_arg_s32(1),
			tgif_arg_s64(1),
			tgif_arg_u16(tgif_bswap_16(1)),
			tgif_arg_u32(tgif_bswap_32(1)),
			tgif_arg_u64(tgif_bswap_64(1)),
			tgif_arg_s16(tgif_bswap_16(1)),
			tgif_arg_s32(tgif_bswap_32(1)),
			tgif_arg_s64(tgif_bswap_64(1)),
#else
			tgif_arg_u16(tgif_bswap_16(1)),
			tgif_arg_u32(tgif_bswap_32(1)),
			tgif_arg_u64(tgif_bswap_64(1)),
			tgif_arg_s16(tgif_bswap_16(1)),
			tgif_arg_s32(tgif_bswap_32(1)),
			tgif_arg_s64(tgif_bswap_64(1)),
			tgif_arg_u16(1),
			tgif_arg_u32(1),
			tgif_arg_u64(1),
			tgif_arg_s16(1),
			tgif_arg_s32(1),
			tgif_arg_s64(1),
#endif
		),
		tgif_arg_list(
#if TGIF_BYTE_ORDER == TGIF_LITTLE_ENDIAN
			tgif_arg_dynamic_field("u16_le", tgif_arg_dynamic_u16_le(1, tgif_attr_list())),
			tgif_arg_dynamic_field("u32_le", tgif_arg_dynamic_u32_le(1, tgif_attr_list())),
			tgif_arg_dynamic_field("u64_le", tgif_arg_dynamic_u64_le(1, tgif_attr_list())),
			tgif_arg_dynamic_field("s16_le", tgif_arg_dynamic_s16_le(1, tgif_attr_list())),
			tgif_arg_dynamic_field("s32_le", tgif_arg_dynamic_s32_le(1, tgif_attr_list())),
			tgif_arg_dynamic_field("s64_le", tgif_arg_dynamic_s64_le(1, tgif_attr_list())),
			tgif_arg_dynamic_field("u16_be", tgif_arg_dynamic_u16_be(tgif_bswap_16(1), tgif_attr_list())),
			tgif_arg_dynamic_field("u32_be", tgif_arg_dynamic_u32_be(tgif_bswap_32(1), tgif_attr_list())),
			tgif_arg_dynamic_field("u64_be", tgif_arg_dynamic_u64_be(tgif_bswap_64(1), tgif_attr_list())),
			tgif_arg_dynamic_field("s16_be", tgif_arg_dynamic_s16_be(tgif_bswap_16(1), tgif_attr_list())),
			tgif_arg_dynamic_field("s32_be", tgif_arg_dynamic_s32_be(tgif_bswap_32(1), tgif_attr_list())),
			tgif_arg_dynamic_field("s64_be", tgif_arg_dynamic_s64_be(tgif_bswap_64(1), tgif_attr_list())),
#else
			tgif_arg_dynamic_field("u16_le", tgif_arg_dynamic_u16_le(tgif_bswap_16(1), tgif_attr_list())),
			tgif_arg_dynamic_field("u32_le", tgif_arg_dynamic_u32_le(tgif_bswap_32(1), tgif_attr_list())),
			tgif_arg_dynamic_field("u64_le", tgif_arg_dynamic_u64_le(tgif_bswap_64(1), tgif_attr_list())),
			tgif_arg_dynamic_field("s16_le", tgif_arg_dynamic_s16_le(tgif_bswap_16(1), tgif_attr_list())),
			tgif_arg_dynamic_field("s32_le", tgif_arg_dynamic_s32_le(tgif_bswap_32(1), tgif_attr_list())),
			tgif_arg_dynamic_field("s64_le", tgif_arg_dynamic_s64_le(tgif_bswap_64(1), tgif_attr_list())),
			tgif_arg_dynamic_field("u16_be", tgif_arg_dynamic_u16_be(1, tgif_attr_list())),
			tgif_arg_dynamic_field("u32_be", tgif_arg_dynamic_u32_be(1, tgif_attr_list())),
			tgif_arg_dynamic_field("u64_be", tgif_arg_dynamic_u64_be(1, tgif_attr_list())),
			tgif_arg_dynamic_field("s16_be", tgif_arg_dynamic_s16_be(1, tgif_attr_list())),
			tgif_arg_dynamic_field("s32_be", tgif_arg_dynamic_s32_be(1, tgif_attr_list())),
			tgif_arg_dynamic_field("s64_be", tgif_arg_dynamic_s64_be(1, tgif_attr_list())),
#endif
		),
		tgif_attr_list()
	);
}

tgif_static_event(my_provider_event_base, "myprovider", "myevent_base", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_u8("u8base2", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(2)))),
		tgif_field_u8("u8base8", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(8)))),
		tgif_field_u8("u8base10", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_u8("u8base16", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_u16("u16base2", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(2)))),
		tgif_field_u16("u16base8", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(8)))),
		tgif_field_u16("u16base10", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_u16("u16base16", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_u32("u32base2", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(2)))),
		tgif_field_u32("u32base8", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(8)))),
		tgif_field_u32("u32base10", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_u32("u32base16", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_u64("u64base2", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(2)))),
		tgif_field_u64("u64base8", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(8)))),
		tgif_field_u64("u64base10", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_u64("u64base16", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_s8("s8base2", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(2)))),
		tgif_field_s8("s8base8", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(8)))),
		tgif_field_s8("s8base10", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_s8("s8base16", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_s16("s16base2", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(2)))),
		tgif_field_s16("s16base8", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(8)))),
		tgif_field_s16("s16base10", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_s16("s16base16", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_s32("s32base2", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(2)))),
		tgif_field_s32("s32base8", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(8)))),
		tgif_field_s32("s32base10", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_s32("s32base16", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_s64("s64base2", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(2)))),
		tgif_field_s64("s64base8", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(8)))),
		tgif_field_s64("s64base10", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_s64("s64base16", tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
	),
	tgif_attr_list()
);

static
void test_base(void)
{
	tgif_event(my_provider_event_base,
		tgif_arg_list(
			tgif_arg_u8(55),
			tgif_arg_u8(55),
			tgif_arg_u8(55),
			tgif_arg_u8(55),
			tgif_arg_u16(55),
			tgif_arg_u16(55),
			tgif_arg_u16(55),
			tgif_arg_u16(55),
			tgif_arg_u32(55),
			tgif_arg_u32(55),
			tgif_arg_u32(55),
			tgif_arg_u32(55),
			tgif_arg_u64(55),
			tgif_arg_u64(55),
			tgif_arg_u64(55),
			tgif_arg_u64(55),
			tgif_arg_s8(-55),
			tgif_arg_s8(-55),
			tgif_arg_s8(-55),
			tgif_arg_s8(-55),
			tgif_arg_s16(-55),
			tgif_arg_s16(-55),
			tgif_arg_s16(-55),
			tgif_arg_s16(-55),
			tgif_arg_s32(-55),
			tgif_arg_s32(-55),
			tgif_arg_s32(-55),
			tgif_arg_s32(-55),
			tgif_arg_s64(-55),
			tgif_arg_s64(-55),
			tgif_arg_s64(-55),
			tgif_arg_s64(-55),
		)
	);
}

struct test {
	uint32_t a;
	uint64_t b;
	uint8_t c;
	int32_t d;
	uint16_t e;
	int8_t f;
	int16_t g;
	int32_t h;
	int64_t i;
	int64_t j;
	int64_t k;
	uint64_t test;
};

static tgif_define_struct(mystructgatherdef,
	tgif_field_list(
		tgif_field_gather_unsigned_integer("a", offsetof(struct test, a),
			tgif_struct_field_sizeof(struct test, a), 0, 0,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_signed_integer("d", offsetof(struct test, d),
			tgif_struct_field_sizeof(struct test, d), 0, 0,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_unsigned_integer("e", offsetof(struct test, e),
			tgif_struct_field_sizeof(struct test, e), 8, 4,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_gather_signed_integer("f", offsetof(struct test, f),
			tgif_struct_field_sizeof(struct test, f), 1, 4,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_gather_signed_integer("g", offsetof(struct test, g),
			tgif_struct_field_sizeof(struct test, g), 11, 4,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_gather_signed_integer("h", offsetof(struct test, h),
			tgif_struct_field_sizeof(struct test, h), 1, 31,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_gather_signed_integer("i", offsetof(struct test, i),
			tgif_struct_field_sizeof(struct test, i), 33, 20,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_gather_signed_integer("j", offsetof(struct test, j),
			tgif_struct_field_sizeof(struct test, j), 63, 1,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_gather_signed_integer("k", offsetof(struct test, k), 
			tgif_struct_field_sizeof(struct test, k), 1, 63,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
		tgif_field_gather_unsigned_integer_le("test", offsetof(struct test, test),
			tgif_struct_field_sizeof(struct test, test), 0, 64,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_gather_unsigned_integer_le("test_le", offsetof(struct test, test),
			tgif_struct_field_sizeof(struct test, test), 0, 64,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
		tgif_field_gather_unsigned_integer_be("test_be", offsetof(struct test, test),
			tgif_struct_field_sizeof(struct test, test), 0, 64,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(16)))),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_structgather, "myprovider", "myeventstructgather", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_struct("structgather", &mystructgatherdef, 0, sizeof(struct test),
				TGIF_TYPE_GATHER_ACCESS_DIRECT),
		tgif_field_gather_signed_integer("intgather", 0, sizeof(int32_t), 0, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_attr_list(tgif_attr("std.integer.base", tgif_attr_u8(10)))),
#if __HAVE_FLOAT32
		tgif_field_gather_float("f32", 0, sizeof(_Float32), TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
#endif
	),
	tgif_attr_list()
);

static
void test_struct_gather(void)
{
	tgif_event_cond(my_provider_event_structgather) {
		struct test mystruct = {
			.a = 55,
			.b = 123,
			.c = 2,
			.d = -55,
			.e = 0xABCD,
			.f = -1,
			.g = -1,
			.h = -1,
			.i = -1,
			.j = -1,
			.k = -1,
			.test = 0xFF,
		};
		int32_t val = -66;
#if __HAVE_FLOAT32
		_Float32 f32 = 1.1;
#endif
		tgif_event_call(my_provider_event_structgather,
			tgif_arg_list(
				tgif_arg_gather_struct(&mystruct),
				tgif_arg_gather_integer(&val),
#if __HAVE_FLOAT32
				tgif_arg_gather_float(&f32),
#endif
			)
		);
	}
}

struct testnest2 {
	uint8_t c;
};

struct testnest1 {
	uint64_t b;
	struct testnest2 *nest;
};

struct testnest0 {
	uint32_t a;
	struct testnest1 *nest;
};

static tgif_define_struct(mystructgathernest2,
	tgif_field_list(
		tgif_field_gather_unsigned_integer("c", offsetof(struct testnest2, c),
			tgif_struct_field_sizeof(struct testnest2, c), 0, 0,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
	),
	tgif_attr_list()
);

static tgif_define_struct(mystructgathernest1,
	tgif_field_list(
		tgif_field_gather_unsigned_integer("b", offsetof(struct testnest1, b),
			tgif_struct_field_sizeof(struct testnest1, b), 0, 0,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_struct("nest2", &mystructgathernest2,
			offsetof(struct testnest1, nest), sizeof(struct testnest2),
			TGIF_TYPE_GATHER_ACCESS_POINTER),
	),
	tgif_attr_list()
);

static tgif_define_struct(mystructgathernest0,
	tgif_field_list(
		tgif_field_gather_unsigned_integer("a", offsetof(struct testnest0, a),
			tgif_struct_field_sizeof(struct testnest0, a), 0, 0,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_struct("nest1", &mystructgathernest1,
			offsetof(struct testnest0, nest), sizeof(struct testnest1),
			TGIF_TYPE_GATHER_ACCESS_POINTER),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_structgather_nest,
	"myprovider", "myeventstructgathernest", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_struct("nest0", &mystructgathernest0, 0,
			sizeof(struct testnest0), TGIF_TYPE_GATHER_ACCESS_DIRECT),
	),
	tgif_attr_list()
);

static
void test_struct_gather_nest_ptr(void)
{
	tgif_event_cond(my_provider_event_structgather_nest) {
		struct testnest2 mystruct2 = {
			.c = 77,
		};
		struct testnest1 mystruct1 = {
			.b = 66,
			.nest = &mystruct2,
		};
		struct testnest0 mystruct = {
			.a = 55,
			.nest = &mystruct1,
		};
		tgif_event_call(my_provider_event_structgather_nest,
			tgif_arg_list(
				tgif_arg_gather_struct(&mystruct),
			)
		);
	}
}

struct testfloat {
#if __HAVE_FLOAT16
	_Float16 f16;
#endif
#if __HAVE_FLOAT32
	_Float32 f32;
#endif
#if __HAVE_FLOAT64
	_Float64 f64;
#endif
#if __HAVE_FLOAT128
	_Float128 f128;
#endif
};

static tgif_define_struct(mystructgatherfloat,
	tgif_field_list(
#if __HAVE_FLOAT16
		tgif_field_gather_float("f16", offsetof(struct testfloat, f16), tgif_struct_field_sizeof(struct testfloat, f16),
			TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_attr_list()),
#endif
#if __HAVE_FLOAT32
		tgif_field_gather_float("f32", offsetof(struct testfloat, f32), tgif_struct_field_sizeof(struct testfloat, f32),
			TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_attr_list()),
#endif
#if __HAVE_FLOAT64
		tgif_field_gather_float("f64", offsetof(struct testfloat, f64), tgif_struct_field_sizeof(struct testfloat, f64),
			TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_attr_list()),
#endif
#if __HAVE_FLOAT128
		tgif_field_gather_float("f128", offsetof(struct testfloat, f128), tgif_struct_field_sizeof(struct testfloat, f128),
			TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_attr_list()),
#endif
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_structgatherfloat,
	"myprovider", "myeventstructgatherfloat", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_struct("structgatherfloat", &mystructgatherfloat, 0,
			sizeof(struct testfloat), TGIF_TYPE_GATHER_ACCESS_DIRECT),
	),
	tgif_attr_list()
);

static
void test_struct_gather_float(void)
{
	tgif_event_cond(my_provider_event_structgatherfloat) {
		struct testfloat mystruct = {
#if __HAVE_FLOAT16
			.f16 = 1.1,
#endif
#if __HAVE_FLOAT32
			.f32 = 2.2,
#endif
#if __HAVE_FLOAT64
			.f64 = 3.3,
#endif
#if __HAVE_FLOAT128
			.f128 = 4.4,
#endif
		};
		tgif_event_call(my_provider_event_structgatherfloat,
			tgif_arg_list(
				tgif_arg_gather_struct(&mystruct),
			)
		);
	}
}

uint32_t mygatherarray[] = { 1, 2, 3, 4, 5 };

uint16_t mygatherarray2[] = { 6, 7, 8, 9 };

struct testarray {
	int a;
	uint32_t *ptr;
};

static tgif_define_struct(mystructgatherarray,
	tgif_field_list(
		tgif_field_gather_array("array",
			tgif_elem(tgif_type_gather_unsigned_integer(0, sizeof(uint32_t), 0, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			TGIF_ARRAY_SIZE(mygatherarray),
			offsetof(struct testarray, ptr),
			TGIF_TYPE_GATHER_ACCESS_POINTER,
			tgif_attr_list()),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_structgatherarray,
	"myprovider", "myeventstructgatherarray", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_struct("structgatherarray", &mystructgatherarray, 0,
				sizeof(struct testarray), TGIF_TYPE_GATHER_ACCESS_DIRECT),
		tgif_field_gather_array("array2",
			tgif_elem(tgif_type_gather_unsigned_integer(0, sizeof(uint16_t), 0, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			TGIF_ARRAY_SIZE(mygatherarray2), 0,
			TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_attr_list()
		),
	),
	tgif_attr_list()
);

static
void test_array_gather(void)
{
	tgif_event_cond(my_provider_event_structgatherarray) {
		struct testarray mystruct = {
			.a = 55,
			.ptr = mygatherarray,
		};
		tgif_event_call(my_provider_event_structgatherarray,
			tgif_arg_list(
				tgif_arg_gather_struct(&mystruct),
				tgif_arg_gather_array(&mygatherarray2),
			)
		);
	}
}

#define TESTSGNESTARRAY_LEN 4
struct testgatherstructnest1 {
	int b;
	int c[TESTSGNESTARRAY_LEN];
};

struct testgatherstructnest0 {
	struct testgatherstructnest1 nest;
	struct testgatherstructnest1 nestarray[2];
	int a;
};

static tgif_define_struct(mystructgatherstructnest1,
	tgif_field_list(
		tgif_field_gather_signed_integer("b", offsetof(struct testgatherstructnest1, b),
			tgif_struct_field_sizeof(struct testgatherstructnest1, b), 0, 0,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_array("c",
			tgif_elem(
				tgif_type_gather_signed_integer(0, sizeof(uint32_t), 0, 0, 
					TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
			),
			TESTSGNESTARRAY_LEN,
			offsetof(struct testgatherstructnest1, c),
			TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_attr_list()),
	),
	tgif_attr_list()
);

static tgif_define_struct(mystructgatherstructnest0,
	tgif_field_list(
		tgif_field_gather_signed_integer("a", offsetof(struct testgatherstructnest0, a),
			tgif_struct_field_sizeof(struct testgatherstructnest0, a), 0, 0,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_struct("structnest0", &mystructgatherstructnest1,
			offsetof(struct testgatherstructnest0, nest),
			sizeof(struct testgatherstructnest1),
			TGIF_TYPE_GATHER_ACCESS_DIRECT),
		tgif_field_gather_array("nestarray",
			tgif_elem(
				tgif_type_gather_struct(&mystructgatherstructnest1,
					0,
					sizeof(struct testgatherstructnest1),
					TGIF_TYPE_GATHER_ACCESS_DIRECT),
			),
			2,
			offsetof(struct testgatherstructnest0, nestarray),
			TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_attr_list()),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_gatherstructnest,
	"myprovider", "myeventgatherstructnest", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_struct("structgather", &mystructgatherstructnest0, 0,
				sizeof(struct testgatherstructnest0), TGIF_TYPE_GATHER_ACCESS_DIRECT),
	),
	tgif_attr_list()
);

static
void test_gather_structnest(void)
{
	tgif_event_cond(my_provider_event_gatherstructnest) {
		struct testgatherstructnest0 mystruct = {
			.nest = {
				.b = 66,
				.c = { 0, 1, 2, 3 },
			},
			.nestarray = {
				[0] = {
					.b = 77,
					.c = { 11, 12, 13, 14 },
				},
				[1] = {
					.b = 88,
					.c = { 15, 16, 17, 18 },
				},
			},
			.a = 55,
		};
		tgif_event_call(my_provider_event_gatherstructnest,
			tgif_arg_list(
				tgif_arg_gather_struct(&mystruct),
			)
		);
	}
}

uint32_t gathervla[] = { 1, 2, 3, 4 };
uint32_t gathervla2[] = { 5, 6, 7, 8, 9 };

struct testgathervla {
	int a;
	uint16_t len;
	uint32_t *p;
};

static tgif_define_struct(mystructgathervla,
	tgif_field_list(
		tgif_field_gather_signed_integer("a", offsetof(struct testgathervla, a),
			tgif_struct_field_sizeof(struct testgathervla, a), 0, 0,
			TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()
		),
		tgif_field_gather_vla("nestvla",
			tgif_elem(tgif_type_gather_unsigned_integer(0, sizeof(uint32_t), 0, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			offsetof(struct testgathervla, p),
			TGIF_TYPE_GATHER_ACCESS_POINTER,
			tgif_length(tgif_type_gather_unsigned_integer(offsetof(struct testgathervla, len),
					sizeof(uint16_t), 0, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			tgif_attr_list()
		),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_gathervla,
	"myprovider", "myeventgathervla", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_struct("structgathervla", &mystructgathervla, 0,
				sizeof(struct testgathervla), TGIF_TYPE_GATHER_ACCESS_DIRECT),
		tgif_field_gather_vla("vla",
			tgif_elem(tgif_type_gather_unsigned_integer(0, sizeof(uint32_t), 0, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			0, TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_length(tgif_type_gather_unsigned_integer(0, sizeof(uint16_t), 0, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			tgif_attr_list()
		),
	),
	tgif_attr_list()
);

static
void test_gather_vla(void)
{
	tgif_event_cond(my_provider_event_gathervla) {
		struct testgathervla mystruct = {
			.a = 55,
			.len = TGIF_ARRAY_SIZE(gathervla),
			.p = gathervla,
		};
		uint16_t vla2_len = 5;
		tgif_event_call(my_provider_event_gathervla,
			tgif_arg_list(
				tgif_arg_gather_struct(&mystruct),
				tgif_arg_gather_vla(gathervla2, &vla2_len),
			)
		);
	}
}

struct testgathervlaflex {
	uint8_t len;
	uint32_t otherfield;
	uint64_t array[];
};

static tgif_define_struct(mystructgathervlaflex,
	tgif_field_list(
		tgif_field_gather_vla("vlaflex",
			tgif_elem(tgif_type_gather_unsigned_integer(0, sizeof(uint64_t), 0, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			offsetof(struct testgathervlaflex, array),
			TGIF_TYPE_GATHER_ACCESS_DIRECT,
			tgif_length(tgif_type_gather_unsigned_integer(offsetof(struct testgathervlaflex, len),
					tgif_struct_field_sizeof(struct testgathervlaflex, len), 0, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			tgif_attr_list()
		),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_gathervlaflex,
	"myprovider", "myeventgathervlaflex", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_struct("structgathervlaflex", &mystructgathervlaflex, 0,
				sizeof(struct testgathervlaflex), TGIF_TYPE_GATHER_ACCESS_DIRECT),
	),
	tgif_attr_list()
);

#define VLAFLEXLEN	6
static
void test_gather_vla_flex(void)
{
	tgif_event_cond(my_provider_event_gathervlaflex) {
		struct testgathervlaflex *mystruct =
			(struct testgathervlaflex *) malloc(sizeof(*mystruct) + VLAFLEXLEN * sizeof(uint64_t));

		mystruct->len = VLAFLEXLEN;
		mystruct->otherfield = 0;
		mystruct->array[0] = 1;
		mystruct->array[1] = 2;
		mystruct->array[2] = 3;
		mystruct->array[3] = 4;
		mystruct->array[4] = 5;
		mystruct->array[5] = 6;
		tgif_event_call(my_provider_event_gathervlaflex,
			tgif_arg_list(
				tgif_arg_gather_struct(mystruct),
			)
		);
		free(mystruct);
	}
}

tgif_static_event(my_provider_event_gatherbyte,
	"myprovider", "myeventgatherbyte", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_byte("byte", 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_array("array",
			tgif_elem(tgif_type_gather_byte(0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			3, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()
		),
	),
	tgif_attr_list()
);

static
void test_gather_byte(void)
{
	tgif_event_cond(my_provider_event_gatherbyte) {
		uint8_t v = 0x44;
		uint8_t array[3] = { 0x1, 0x2, 0x3 };

		tgif_event_call(my_provider_event_gatherbyte,
			tgif_arg_list(
				tgif_arg_gather_byte(&v),
				tgif_arg_gather_array(array),
			)
		);
	}
}

#define ARRAYBOOLLEN 4
static bool arraybool[ARRAYBOOLLEN] = { false, true, false, true };

tgif_static_event(my_provider_event_gatherbool,
	"myprovider", "myeventgatherbool", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_bool("v1_true", 0, sizeof(bool), 0, 0,
				TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_bool("v2_false", 0, sizeof(bool), 0, 0,
				TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_bool("v3_true", 0, sizeof(uint16_t), 1, 1,
				TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_bool("v4_false", 0, sizeof(uint16_t), 1, 1,
				TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_array("arraybool",
			tgif_elem(tgif_type_gather_bool(0, sizeof(bool), 0, 0,
				TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			ARRAYBOOLLEN, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()
		),
	),
	tgif_attr_list()
);

static
void test_gather_bool(void)
{
	tgif_event_cond(my_provider_event_structgatherarray) {
		bool v1 = true;
		bool v2 = false;
		uint16_t v3 = 1U << 1;
		uint16_t v4 = 1U << 2;

		tgif_event_call(my_provider_event_gatherbool,
			tgif_arg_list(
				tgif_arg_gather_bool(&v1),
				tgif_arg_gather_bool(&v2),
				tgif_arg_gather_bool(&v3),
				tgif_arg_gather_bool(&v4),
				tgif_arg_gather_array(arraybool),
			)
		);
	}
}

tgif_static_event(my_provider_event_gatherpointer,
	"myprovider", "myeventgatherpointer", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_pointer("ptr", 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_array("array",
			tgif_elem(tgif_type_gather_pointer(0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			3, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()
		),
	),
	tgif_attr_list()
);

static
void test_gather_pointer(void)
{
	tgif_event_cond(my_provider_event_structgatherarray) {
		void *v = (void *)0x44;
		void *array[3] = { (void *)0x1, (void *)0x2, (void *)0x3 };

		tgif_event_call(my_provider_event_gatherpointer,
			tgif_arg_list(
				tgif_arg_gather_pointer(&v),
				tgif_arg_gather_array(array),
			)
		);
	}
}

static tgif_define_enum(myenumgather,
	tgif_enum_mapping_list(
		tgif_enum_mapping_range("one-ten", 1, 10),
		tgif_enum_mapping_range("100-200", 100, 200),
		tgif_enum_mapping_value("200", 200),
		tgif_enum_mapping_value("300", 300),
	),
	tgif_attr_list()
);

tgif_static_event(my_provider_event_enum_gather, "myprovider", "myeventenumgather", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_enum("5", &myenumgather,
			tgif_elem(
				tgif_type_gather_unsigned_integer(0, sizeof(uint32_t), 0, 0,
					TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())
			)
		),
		tgif_field_gather_enum("400", &myenumgather,
			tgif_elem(
				tgif_type_gather_unsigned_integer(0, sizeof(uint64_t), 0, 0,
					TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())
			)
		),
		tgif_field_gather_enum("200", &myenumgather,
			tgif_elem(
				tgif_type_gather_unsigned_integer(0, sizeof(uint8_t), 0, 0,
					TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())
			)
		),
		tgif_field_gather_enum("-100", &myenumgather,
			tgif_elem(
				tgif_type_gather_signed_integer(0, sizeof(int8_t), 0, 0,
					TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())
			)
		),
		tgif_field_gather_enum("6_be", &myenumgather,
			tgif_elem(
				tgif_type_gather_unsigned_integer_be(0, sizeof(uint32_t), 0, 0,
					TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())
			)
		),
		tgif_field_gather_enum("6_le", &myenumgather,
			tgif_elem(
				tgif_type_gather_unsigned_integer_le(0, sizeof(uint32_t), 0, 0,
					TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())
			)
		),
	),
	tgif_attr_list()
);

static
void test_gather_enum(void)
{
	uint32_t v1 = 5;
	uint64_t v2 = 400;
	uint8_t v3 = 200;
	int8_t v4 = -100;
#if TGIF_BYTE_ORDER == TGIF_LITTLE_ENDIAN
	uint32_t v5 = tgif_bswap_32(6);
	uint32_t v6 = 6;
#else
	uint32_t v5 = 6;
	uint32_t v6 = tgif_bswap_32(6);
#endif

	tgif_event(my_provider_event_enum_gather,
		tgif_arg_list(
			tgif_arg_gather_integer(&v1),
			tgif_arg_gather_integer(&v2),
			tgif_arg_gather_integer(&v3),
			tgif_arg_gather_integer(&v4),
			tgif_arg_gather_integer(&v5),
			tgif_arg_gather_integer(&v6),
		)
	);
}

tgif_static_event(my_provider_event_gatherstring,
	"myprovider", "myeventgatherstring", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_gather_string("string", 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
		tgif_field_gather_array("arrayptr",
			tgif_elem(tgif_type_gather_string(0, TGIF_TYPE_GATHER_ACCESS_POINTER, tgif_attr_list())),
			3, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()
		),
		tgif_field_gather_array("array",
			tgif_elem(tgif_type_gather_string(0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list())),
			3, 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()
		),
	),
	tgif_attr_list()
);

static
void test_gather_string(void)
{
	tgif_event_cond(my_provider_event_gatherstring) {
		const char *str1 = "abcdef";
		const char *ptrarray[3] = {
			"abc",
			"def",
			"ghi",
		};
		char flatarray[] = { 'a', 'b', '\0', 'c', 'd', '\0', 'e', 'f', '\0' };

		tgif_event_call(my_provider_event_gatherstring,
			tgif_arg_list(
				tgif_arg_gather_string(str1),
				tgif_arg_gather_array(ptrarray),
				tgif_arg_gather_array(flatarray),
			)
		);
	}
}

tgif_static_event(my_provider_event_str_utf, "myprovider", "myevent_str_utf", TGIF_LOGLEVEL_DEBUG,
	tgif_field_list(
		tgif_field_string("utf8", tgif_attr_list()),
		tgif_field_string32("utf32", tgif_attr_list()),
		tgif_field_string16("utf16", tgif_attr_list()),
		tgif_field_string32_le("utf32_le", tgif_attr_list()),
		tgif_field_string16_le("utf16_le", tgif_attr_list()),
		tgif_field_string32_be("utf32_be", tgif_attr_list()),
		tgif_field_string16_be("utf16_be", tgif_attr_list()),
		tgif_field_dynamic("dynamic_utf32"),
		tgif_field_gather_string32("gather_utf32", 0, TGIF_TYPE_GATHER_ACCESS_DIRECT, tgif_attr_list()),
	),
	tgif_attr_list()
);

static
void test_string_utf(void)
{
	/*
	 * Character '®' is:
	 * UTF-8: \c2 \ae
	 * UTF-16: U+00ae
	 * UTF-32: U+000000ae
	 */
	uint8_t str8[] = { 0xc2, 0xae, 'a', 'b', 'c', 0 };
	uint32_t str32[] = { 0x000000ae, 'a', 'b', 'c', 0 };
	uint16_t str16[] = { 0x00ae, 'a', 'b', 'c', 0 };
#if TGIF_BYTE_ORDER == TGIF_LITTLE_ENDIAN
	uint32_t str32_le[] = { 0x000000ae, 'a', 'b', 'c', 0 };
	uint16_t str16_le[] = { 0x00ae, 'a', 'b', 'c', 0 };
	uint32_t str32_be[] = { tgif_bswap_32(0x000000ae), tgif_bswap_32('a'), tgif_bswap_32('b'), tgif_bswap_32('c'), 0 };
	uint16_t str16_be[] = { tgif_bswap_16(0x00ae), tgif_bswap_16('a'), tgif_bswap_16('b'), tgif_bswap_16('c'), 0 };
#else
	uint32_t str32_le[] = { tgif_bswap_32(0x000000ae), tgif_bswap_32('a'), tgif_bswap_32('b'), tgif_bswap_32('c'), 0 };
	uint16_t str16_le[] = { tgif_bswap_16(0x00ae), tgif_bswap_16('a'), tgif_bswap_16('b'), tgif_bswap_16('c'), 0 };
	uint32_t str32_be[] = { 0x000000ae, 'a', 'b', 'c', 0 };
	uint16_t str16_be[] = { 0x00ae, 'a', 'b', 'c', 0 };
#endif

	tgif_event(my_provider_event_str_utf,
		tgif_arg_list(
			tgif_arg_string(str8),
			tgif_arg_string32(str32),
			tgif_arg_string16(str16),
			tgif_arg_string32(str32_le),
			tgif_arg_string16(str16_le),
			tgif_arg_string32(str32_be),
			tgif_arg_string16(str16_be),
			tgif_arg_dynamic_string32(str32, tgif_attr_list()),
			tgif_arg_gather_string(str32),
		)
	);
}

int main()
{
	test_fields();
	test_event_hidden();
	test_event_export();
	test_struct_literal();
	test_struct();
	test_array();
	test_vla();
	test_vla_visitor();
	test_vla_visitor_2d();
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
	test_blob();
	test_fmt_string();
	test_endian();
	test_base();
	test_struct_gather();
	test_struct_gather_nest_ptr();
	test_struct_gather_float();
	test_array_gather();
	test_gather_structnest();
	test_gather_vla();
	test_gather_vla_flex();
	test_gather_byte();
	test_gather_bool();
	test_gather_pointer();
	test_gather_enum();
	test_gather_string();
	test_string_utf();
	return 0;
}
