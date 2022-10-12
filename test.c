// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

/* SIDE stands for "Static Instrumentation Dynamically Enabled" */

/* Helper macros */

#define SIDE_ARRAY_SIZE(arr)	(sizeof(arr) / sizeof((arr)[0]))

/*
 * Compound literals with static storage are needed by SIDE
 * instrumentation.
 * Compound literals are part of the C99 and C11 standards, but not
 * part of the C++ standards. They are supported by most C++ compilers
 * though.
 *
 * Example use:
 * static struct mystruct *var = LTTNG_UST_COMPOUND_LITERAL(struct mystruct, { 1, 2, 3 });
 */
#define SIDE_COMPOUND_LITERAL(type, ...)   (type[]) { __VA_ARGS__ }

#define side_likely(x)		__builtin_expect(!!(x), 1)
#define side_unlikely(x)	__builtin_expect(!!(x), 0)

struct side_arg_vec;
struct side_type_description;
struct side_event_field;

enum side_type {
	SIDE_TYPE_U8,
	SIDE_TYPE_U16,
	SIDE_TYPE_U32,
	SIDE_TYPE_U64,
	SIDE_TYPE_S8,
	SIDE_TYPE_S16,
	SIDE_TYPE_S32,
	SIDE_TYPE_S64,
	SIDE_TYPE_STRING,
	SIDE_TYPE_DYNAMIC,
	SIDE_TYPE_STRUCT,
	SIDE_TYPE_ARRAY,
	SIDE_TYPE_VLA,
	SIDE_TYPE_VLA_VISITOR,
	//TODO:
	//specialized array and vla for fixed-size integers (optimization)
	//variants (discriminated unions)
};

enum side_loglevel {
	SIDE_LOGLEVEL_EMERG = 0,
	SIDE_LOGLEVEL_ALERT = 1,
	SIDE_LOGLEVEL_CRIT = 2,
	SIDE_LOGLEVEL_ERR = 3,
	SIDE_LOGLEVEL_WARNING = 4,
	SIDE_LOGLEVEL_NOTICE = 5,
	SIDE_LOGLEVEL_INFO = 6,
	SIDE_LOGLEVEL_DEBUG = 7,
};

enum side_visitor_status {
	SIDE_VISITOR_STATUS_ERROR = -1,
	SIDE_VISITOR_STATUS_OK = 0,
	SIDE_VISITOR_STATUS_END = 1,
};

typedef enum side_visitor_status (*side_visitor_begin)(void *ctx);
typedef enum side_visitor_status (*side_visitor_end)(void *ctx);
typedef enum side_visitor_status (*side_visitor_get_next)(void *ctx, struct side_arg_vec *sav_elem);

struct side_type_description {
	enum side_type type;
	union {
		struct {
			uint32_t nr_fields;
			const struct side_event_field *fields;
		} side_struct;
		struct {
			uint32_t length;
			const struct side_type_description *elem_type;
		} side_array;
		struct {
			const struct side_type_description *elem_type;
		} side_vla;
		struct {
			const struct side_type_description *elem_type;
			side_visitor_begin begin;
			side_visitor_end end;
			side_visitor_get_next get_next;
		} side_vla_visitor;
	} u;
};

struct side_event_field {
	const char *field_name;
	struct side_type_description side_type;
};

struct side_event_description {
	uint32_t version;
	uint32_t enabled;
	uint32_t loglevel;	/* enum side_loglevel */
	uint32_t nr_fields;
	const char *provider_name;
	const char *event_name;
	const struct side_event_field *fields;
};

struct side_arg_vec {
	uint32_t type;	/* enum side_type */
	union {
		uint8_t side_u8;
		uint16_t side_u16;
		uint32_t side_u32;
		uint64_t side_u64;
		int8_t side_s8;
		int16_t side_s16;
		int32_t side_s32;
		int64_t side_s64;
		const char *string;
		const struct side_arg_vec_description *side_struct;
		const struct side_arg_vec_description *side_array;
		const struct side_arg_vec_description *side_vla;
		void *side_vla_visitor_ctx;
	} u;
};

struct side_arg_vec_description {
	const struct side_arg_vec *sav;
	uint32_t len;
};

/* Tracer example */

static
void tracer_print_struct(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc);
static
void tracer_print_array(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc);
static
void tracer_print_vla(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc);
static
void tracer_print_vla_visitor(const struct side_type_description *type_desc, void *ctx);

static
void tracer_print_type(const struct side_type_description *type_desc, const struct side_arg_vec *item)
{
	if (type_desc->type != SIDE_TYPE_DYNAMIC && type_desc->type != item->type) {
		printf("ERROR: type mismatch between description and arguments\n");
		abort();
	}

	switch (item->type) {
	case SIDE_TYPE_U8:
		printf("%" PRIu8, item->u.side_u8);
		break;
	case SIDE_TYPE_U16:
		printf("%" PRIu16, item->u.side_u16);
		break;
	case SIDE_TYPE_U32:
		printf("%" PRIu32, item->u.side_u32);
		break;
	case SIDE_TYPE_U64:
		printf("%" PRIu64, item->u.side_u64);
		break;
	case SIDE_TYPE_S8:
		printf("%" PRId8, item->u.side_s8);
		break;
	case SIDE_TYPE_S16:
		printf("%" PRId16, item->u.side_s16);
		break;
	case SIDE_TYPE_S32:
		printf("%" PRId32, item->u.side_s32);
		break;
	case SIDE_TYPE_S64:
		printf("%" PRId64, item->u.side_s64);
		break;
	case SIDE_TYPE_STRING:
		printf("%s", item->u.string);
		break;
	case SIDE_TYPE_STRUCT:
		tracer_print_struct(type_desc, item->u.side_struct);
		break;
	case SIDE_TYPE_ARRAY:
		tracer_print_array(type_desc, item->u.side_array);
		break;
	case SIDE_TYPE_VLA:
		tracer_print_vla(type_desc, item->u.side_vla);
		break;
	case SIDE_TYPE_VLA_VISITOR:
		tracer_print_vla_visitor(type_desc, item->u.side_vla_visitor_ctx);
		break;
	default:
		printf("<UNKNOWN TYPE>");
		abort();
	}
}

static
void tracer_print_field(const struct side_event_field *item_desc, const struct side_arg_vec *item)
{
	printf("(\"%s\", ", item_desc->field_name);
	tracer_print_type(&item_desc->side_type, item);
	printf(")");
}

static
void tracer_print_struct(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc)
{
	const struct side_arg_vec *sav = sav_desc->sav;
	uint32_t side_sav_len = sav_desc->len;
	int i;

	if (type_desc->u.side_struct.nr_fields != side_sav_len) {
		printf("ERROR: number of fields mismatch between description and arguments of structure\n");
		abort();
	}
	printf("{ ");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_field(&type_desc->u.side_struct.fields[i], &sav[i]);
	}
	printf(" }");
}

static
void tracer_print_array(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc)
{
	const struct side_arg_vec *sav = sav_desc->sav;
	uint32_t side_sav_len = sav_desc->len;
	int i;

	if (type_desc->u.side_array.length != side_sav_len) {
		printf("ERROR: length mismatch between description and arguments of array\n");
		abort();
	}
	printf("[ ");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_type(type_desc->u.side_array.elem_type, &sav[i]);
	}
	printf(" ]");
}

static
void tracer_print_vla(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc)
{
	const struct side_arg_vec *sav = sav_desc->sav;
	uint32_t side_sav_len = sav_desc->len;
	int i;

	printf("[ ");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_type(type_desc->u.side_vla.elem_type, &sav[i]);
	}
	printf(" ]");
}

static
void tracer_print_vla_visitor(const struct side_type_description *type_desc, void *ctx)
{
	enum side_visitor_status status;
	int i;

	status = type_desc->u.side_vla_visitor.begin(ctx);
	if (status != SIDE_VISITOR_STATUS_OK) {
		printf("ERROR: Visitor error\n");
		abort();
	}

	printf("[ ");
	status = SIDE_VISITOR_STATUS_OK;
	for (i = 0; status == SIDE_VISITOR_STATUS_OK; i++) {
		struct side_arg_vec sav_elem;

		status = type_desc->u.side_vla_visitor.get_next(ctx, &sav_elem);
		switch (status) {
		case SIDE_VISITOR_STATUS_OK:
			break;
		case SIDE_VISITOR_STATUS_ERROR:
			printf("ERROR: Visitor error\n");
			abort();
		case SIDE_VISITOR_STATUS_END:
			continue;
		}
		printf("%s", i ? ", " : "");
		tracer_print_type(type_desc->u.side_vla_visitor.elem_type, &sav_elem);
	}
	printf(" ]");
	if (type_desc->u.side_vla_visitor.end) {
		status = type_desc->u.side_vla_visitor.end(ctx);
		if (status != SIDE_VISITOR_STATUS_OK) {
			printf("ERROR: Visitor error\n");
			abort();
		}
	}
}

__attribute__((noinline))
void tracer_call(const struct side_event_description *desc, const struct side_arg_vec_description *sav_desc)
{
	const struct side_arg_vec *sav = sav_desc->sav;
	uint32_t side_sav_len = sav_desc->len;
	int i;

	printf("provider: %s, event: %s, ", desc->provider_name, desc->event_name);
	if (desc->nr_fields != side_sav_len) {
		printf("ERROR: number of fields mismatch between description and arguments\n");
		abort();
	}
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_field(&desc->fields[i], &sav[i]);
	}
	printf("\n");
}

#define SIDE_PARAM(...)	__VA_ARGS__

#define side_type_decl(_type)			{ .type = _type }
#define side_field(_type, _name)		{ .field_name = _name, .side_type = side_type_decl(_type) }

#define side_type_struct_decl(_fields) \
	{ \
		.type = SIDE_TYPE_STRUCT, \
		.u = { \
			.side_struct = { \
				.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
				.fields = _fields, \
			}, \
		}, \
	}
#define side_field_struct(_name, _fields) \
	{ \
		.field_name = _name, \
		.side_type = side_type_struct_decl(SIDE_PARAM(_fields)), \
	}

#define side_type_array_decl(_elem_type, _length) \
	{ \
		.type = SIDE_TYPE_ARRAY, \
		.u = { \
			.side_array = { \
				.length = _length, \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define side_field_array(_name, _elem_type, _length) \
	{ \
		.field_name = _name, \
		.side_type = side_type_array_decl(_elem_type, _length), \
	}

#define side_type_vla_decl(_elem_type) \
	{ \
		.type = SIDE_TYPE_VLA, \
		.u = { \
			.side_vla = { \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define side_field_vla(_name, _elem_type) \
	{ \
		.field_name = _name, \
		.side_type = side_type_vla_decl(_elem_type), \
	}

#define side_type_vla_visitor_decl(_elem_type, _begin, _end, _get_next) \
	{ \
		.type = SIDE_TYPE_VLA_VISITOR, \
		.u = { \
			.side_vla_visitor = { \
				.elem_type = _elem_type, \
				.begin = _begin, \
				.end = _end, \
				.get_next = _get_next, \
			}, \
		}, \
	}
#define side_field_vla_visitor(_name, _elem_type, _begin, _end, _get_next) \
	{ \
		.field_name = _name, \
		.side_type = side_type_vla_visitor_decl(_elem_type, _begin, _end, _get_next), \
	}

#define side_vla_elem(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type_description, side_type_decl(__VA_ARGS__))

#define side_vla_visitor_elem(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type_description, side_type_decl(__VA_ARGS__))

#define side_array_elem(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type_description, side_type_decl(__VA_ARGS__))

#define side_field_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_event_field, __VA_ARGS__)

#define side_arg_u8(val)	{ .type = SIDE_TYPE_U8, .u = { .side_u8 = (val) } }
#define side_arg_u16(val)	{ .type = SIDE_TYPE_U16, .u = { .side_u16 = (val) } }
#define side_arg_u32(val)	{ .type = SIDE_TYPE_U32, .u = { .side_u32 = (val) } }
#define side_arg_u64(val)	{ .type = SIDE_TYPE_U64, .u = { .side_u64 = (val) } }
#define side_arg_s8(val)	{ .type = SIDE_TYPE_S8, .u = { .side_s8 = (val) } }
#define side_arg_s16(val)	{ .type = SIDE_TYPE_S16, .u = { .side_s16 = (val) } }
#define side_arg_s32(val)	{ .type = SIDE_TYPE_S32, .u = { .side_s32 = (val) } }
#define side_arg_s64(val)	{ .type = SIDE_TYPE_S64, .u = { .side_s64 = (val) } }
#define side_arg_string(val)	{ .type = SIDE_TYPE_STRING, .u = { .string = (val) } }
#define side_arg_struct(_side_type)	{ .type = SIDE_TYPE_STRUCT, .u = { .side_struct = (_side_type) } }
#define side_arg_array(_side_type)	{ .type = SIDE_TYPE_ARRAY, .u = { .side_array = (_side_type) } }
#define side_arg_vla(_side_type)	{ .type = SIDE_TYPE_VLA, .u = { .side_vla = (_side_type) } }
#define side_arg_vla_visitor(_ctx)	{ .type = SIDE_TYPE_VLA_VISITOR, .u = { .side_vla_visitor_ctx = (_ctx) } }

#define side_arg_define_vec(_identifier, _sav) \
	const struct side_arg_vec _identifier##_vec[] = { _sav }; \
	const struct side_arg_vec_description _identifier = { \
		.sav = _identifier##_vec, \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
	}
	
#define side_arg_list(...)	__VA_ARGS__

#define side_event_cond(desc) if (side_unlikely((desc)->enabled))
#define side_event_call(desc, _sav) \
	{ \
		const struct side_arg_vec side_sav[] = { _sav }; \
		const struct side_arg_vec_description sav_desc = { \
			.sav = side_sav, \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		tracer_call(desc, &sav_desc); \
	}

#define side_event(desc, sav) \
	side_event_cond(desc) \
		side_event_call(desc, SIDE_PARAM(sav)); \

#define side_define_event(_identifier, _provider, _event, _loglevel, _fields) \
	struct side_event_description _identifier = { \
		.version = 0, \
		.enabled = 0, \
		.loglevel = _loglevel, \
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
		.provider_name = _provider, \
		.event_name = _event, \
		.fields = _fields, \
	}

#define side_declare_event(_identifier) \
	struct side_event_description _identifier

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
