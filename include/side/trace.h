// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _SIDE_TRACE_H
#define _SIDE_TRACE_H

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <side/macros.h>
#include <side/endian.h>

/*
 * SIDE stands for "Software Instrumentation Dynamically Enabled"
 *
 * This is an instrumentation API for Linux user-space, which exposes an
 * instrumentation type system and facilities allowing a kernel or
 * user-space tracer to consume user-space instrumentation.
 *
 * This instrumentation API exposes 3 type systems:
 *
 * * Stack-copy type system: This is the core type system which can
 *   represent all supported types and into which all other type systems
 *   can be nested. This type system requires that every type is
 *   statically or dynamically declared and then registered, thus giving
 *   tracers a complete description of the events and their associated
 *   fields before the associated instrumentation is invoked. The
 *   application needs to copy each argument (side_arg_...()) onto the
 *   stack when calling the instrumentation.
 *
 *   This is the most expressive of the 3 type systems, althrough not the
 *   fastest due to the extra copy of the arguments.
 *
 * * Data-gathering type system: This type system requires every type to
 *   be statically or dynamically declared and registered, but does not
 *   require the application to copy its arguments onto the stack.
 *   Instead, the type description contains all the required information
 *   to fetch the data from the application memory. The only argument
 *   required from the instrumentation is the base pointer from which
 *   the data should be fetched.
 *
 *   This type system can be used as an event field, or nested within
 *   the stack-copy type system. Nesting of gather-vla within
 *   gather-array and gather-vla types is not allowed.
 *
 *   This type system is has the least overhead of the 3 type systems.
 *
 * * Dynamic type system: This type system receives both type
 *   description and actual data onto the stack at runtime. It has more
 *   overhead that the 2 other type systems, but does not require a
 *   prior registration of event field description. This makes it useful
 *   for seldom used types which are not performance critical, but for
 *   which registering each individual events would needlessly grow the
 *   number of events to declare and register.
 *
 *   Another use-case for this type system is for use by dynamically
 *   typed language runtimes, where the field type is only known when
 *   the instrumentation is called.
 *
 *   Those dynamic types can be either used as arguments to a variadic
 *   field list, or as on-stack instrumentation argument for a static
 *   type SIDE_TYPE_DYNAMIC place holder in the stack-copy type system.
 */

//TODO: as those structures will be ABI, we need to either consider them
//fixed forever, or think of a scheme that would allow their binary
//representation to be extended if need be.

struct side_arg;
struct side_arg_vec;
union side_arg_dynamic;
struct side_arg_dynamic_field;
struct side_arg_dynamic_vla;
struct side_type;
struct side_event_field;
struct side_tracer_visitor_ctx;
struct side_tracer_dynamic_struct_visitor_ctx;
struct side_event_description;
struct side_arg_dynamic_struct;
struct side_events_register_handle;
struct side_arg_variant;

enum side_type_label {
	/* Stack-copy basic types */
	SIDE_TYPE_NULL,
	SIDE_TYPE_BOOL,
	SIDE_TYPE_U8,
	SIDE_TYPE_U16,
	SIDE_TYPE_U32,
	SIDE_TYPE_U64,
	SIDE_TYPE_S8,
	SIDE_TYPE_S16,
	SIDE_TYPE_S32,
	SIDE_TYPE_S64,
	SIDE_TYPE_BYTE,
	SIDE_TYPE_POINTER,
	SIDE_TYPE_FLOAT_BINARY16,
	SIDE_TYPE_FLOAT_BINARY32,
	SIDE_TYPE_FLOAT_BINARY64,
	SIDE_TYPE_FLOAT_BINARY128,
	SIDE_TYPE_STRING_UTF8,
	SIDE_TYPE_STRING_UTF16,
	SIDE_TYPE_STRING_UTF32,

	/* Stack-copy compound types */
	SIDE_TYPE_STRUCT,
	SIDE_TYPE_VARIANT,
	SIDE_TYPE_ARRAY,
	SIDE_TYPE_VLA,
	SIDE_TYPE_VLA_VISITOR,

	/* Stack-copy enumeration types */
	SIDE_TYPE_ENUM,
	SIDE_TYPE_ENUM_BITMAP,

	/* Stack-copy place holder for dynamic types */
	SIDE_TYPE_DYNAMIC,

	/* Gather basic types */
	SIDE_TYPE_GATHER_BOOL,
	SIDE_TYPE_GATHER_INTEGER,
	SIDE_TYPE_GATHER_BYTE,
	SIDE_TYPE_GATHER_POINTER,
	SIDE_TYPE_GATHER_FLOAT,
	SIDE_TYPE_GATHER_STRING,

	/* Gather compound types */
	SIDE_TYPE_GATHER_STRUCT,
	SIDE_TYPE_GATHER_ARRAY,
	SIDE_TYPE_GATHER_VLA,

	/* Gather enumeration types */
	SIDE_TYPE_GATHER_ENUM,

	/* Dynamic basic types */
	SIDE_TYPE_DYNAMIC_NULL,
	SIDE_TYPE_DYNAMIC_BOOL,
	SIDE_TYPE_DYNAMIC_INTEGER,
	SIDE_TYPE_DYNAMIC_BYTE,
	SIDE_TYPE_DYNAMIC_POINTER,
	SIDE_TYPE_DYNAMIC_FLOAT,
	SIDE_TYPE_DYNAMIC_STRING,

	/* Dynamic compound types */
	SIDE_TYPE_DYNAMIC_STRUCT,
	SIDE_TYPE_DYNAMIC_STRUCT_VISITOR,
	SIDE_TYPE_DYNAMIC_VLA,
	SIDE_TYPE_DYNAMIC_VLA_VISITOR,
};

enum side_attr_type {
	SIDE_ATTR_TYPE_NULL,
	SIDE_ATTR_TYPE_BOOL,
	SIDE_ATTR_TYPE_U8,
	SIDE_ATTR_TYPE_U16,
	SIDE_ATTR_TYPE_U32,
	SIDE_ATTR_TYPE_U64,
	SIDE_ATTR_TYPE_S8,
	SIDE_ATTR_TYPE_S16,
	SIDE_ATTR_TYPE_S32,
	SIDE_ATTR_TYPE_S64,
	SIDE_ATTR_TYPE_FLOAT_BINARY16,
	SIDE_ATTR_TYPE_FLOAT_BINARY32,
	SIDE_ATTR_TYPE_FLOAT_BINARY64,
	SIDE_ATTR_TYPE_FLOAT_BINARY128,
	SIDE_ATTR_TYPE_STRING,
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
	SIDE_VISITOR_STATUS_OK = 0,
	SIDE_VISITOR_STATUS_ERROR = -1,
};

enum side_error {
	SIDE_ERROR_OK = 0,
	SIDE_ERROR_INVAL = 1,
	SIDE_ERROR_EXIST = 2,
	SIDE_ERROR_NOMEM = 3,
	SIDE_ERROR_NOENT = 4,
	SIDE_ERROR_EXITING = 5,
};

enum side_type_label_byte_order {
	SIDE_TYPE_BYTE_ORDER_LE = 0,
	SIDE_TYPE_BYTE_ORDER_BE = 1,
};

#if (SIDE_BYTE_ORDER == SIDE_LITTLE_ENDIAN)
# define SIDE_TYPE_BYTE_ORDER_HOST		SIDE_TYPE_BYTE_ORDER_LE
#else
# define SIDE_TYPE_BYTE_ORDER_HOST		SIDE_TYPE_BYTE_ORDER_BE
#endif

#if (SIDE_FLOAT_WORD_ORDER == SIDE_LITTLE_ENDIAN)
# define SIDE_TYPE_FLOAT_WORD_ORDER_HOST	SIDE_TYPE_BYTE_ORDER_LE
#else
# define SIDE_TYPE_FLOAT_WORD_ORDER_HOST	SIDE_TYPE_BYTE_ORDER_BE
#endif

enum side_type_gather_access_mode {
	SIDE_TYPE_GATHER_ACCESS_DIRECT,
	SIDE_TYPE_GATHER_ACCESS_POINTER,	/* Pointer dereference */
};

typedef enum side_visitor_status (*side_visitor_func)(
		const struct side_tracer_visitor_ctx *tracer_ctx,
		void *app_ctx);
typedef enum side_visitor_status (*side_dynamic_struct_visitor_func)(
		const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
		void *app_ctx);

union side_integer_value {
	uint8_t side_u8;
	uint16_t side_u16;
	uint32_t side_u32;
	uint64_t side_u64;
	int8_t side_s8;
	int16_t side_s16;
	int32_t side_s32;
	int64_t side_s64;
	uintptr_t side_uptr;
} SIDE_PACKED;

union side_bool_value {
	uint8_t side_bool8;
	uint16_t side_bool16;
	uint32_t side_bool32;
	uint64_t side_bool64;
} SIDE_PACKED;

union side_float_value {
#if __HAVE_FLOAT16
	_Float16 side_float_binary16;
#endif
#if __HAVE_FLOAT32
	_Float32 side_float_binary32;
#endif
#if __HAVE_FLOAT64
	_Float64 side_float_binary64;
#endif
#if __HAVE_FLOAT128
	_Float128 side_float_binary128;
#endif
} SIDE_PACKED;

struct side_type_raw_string {
	const void *p;			/* pointer to string */
	uint8_t unit_size;		/* 1, 2, or 4 bytes */
	uint8_t byte_order;		/* enum side_type_label_byte_order */
} SIDE_PACKED;

struct side_attr_value {
	uint32_t type;	/* enum side_attr_type */
	union {
		uint8_t bool_value;
		struct side_type_raw_string string_value;
		union side_integer_value integer_value;
		union side_float_value float_value;
	} SIDE_PACKED u;
};

/* User attributes. */
struct side_attr {
	const struct side_type_raw_string key;
	const struct side_attr_value value;
} SIDE_PACKED;

/* Type descriptions */
struct side_type_null {
	const struct side_attr *attr;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_bool {
	const struct side_attr *attr;
	uint32_t nr_attr;
	uint16_t bool_size;		/* bytes */
	uint16_t len_bits;		/* bits. 0 for (bool_size * CHAR_BITS) */
	uint8_t byte_order;		/* enum side_type_label_byte_order */
} SIDE_PACKED;

struct side_type_byte {
	const struct side_attr *attr;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_string {
	const struct side_attr *attr;
	uint32_t nr_attr;
	uint8_t unit_size;		/* 1, 2, or 4 bytes */
	uint8_t byte_order;		/* enum side_type_label_byte_order */
} SIDE_PACKED;

struct side_type_integer {
	const struct side_attr *attr;
	uint32_t nr_attr;
	uint16_t integer_size;		/* bytes */
	uint16_t len_bits;		/* bits. 0 for (integer_size * CHAR_BITS) */
	uint8_t signedness;		/* true/false */
	uint8_t byte_order;		/* enum side_type_label_byte_order */
} SIDE_PACKED;

struct side_type_float {
	const struct side_attr *attr;
	uint32_t nr_attr;
	uint16_t float_size;		/* bytes */
	uint8_t byte_order;		/* enum side_type_label_byte_order */
} SIDE_PACKED;

struct side_enum_mapping {
	int64_t range_begin;
	int64_t range_end;
	struct side_type_raw_string label;
} SIDE_PACKED;

struct side_enum_mappings {
	const struct side_enum_mapping *mappings;
	const struct side_attr *attr;
	uint32_t nr_mappings;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_enum_bitmap_mapping {
	uint64_t range_begin;
	uint64_t range_end;
	struct side_type_raw_string label;
} SIDE_PACKED;

struct side_enum_bitmap_mappings {
	const struct side_enum_bitmap_mapping *mappings;
	const struct side_attr *attr;
	uint32_t nr_mappings;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_struct {
	const struct side_event_field *fields;
	const struct side_attr *attr;
	uint32_t nr_fields;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_array {
	const struct side_type *elem_type;
	const struct side_attr *attr;
	uint32_t length;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_vla {
	const struct side_type *elem_type;
	const struct side_attr *attr;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_vla_visitor {
	const struct side_type *elem_type;
	side_visitor_func visitor;
	const struct side_attr *attr;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_enum {
	const struct side_enum_mappings *mappings;
	const struct side_type *elem_type;
} SIDE_PACKED;

struct side_type_enum_bitmap {
	const struct side_enum_bitmap_mappings *mappings;
	const struct side_type *elem_type;
} SIDE_PACKED;

struct side_type_gather_bool {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_bool type;
	uint16_t offset_bits;	/* bits */
} SIDE_PACKED;

struct side_type_gather_byte {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_byte type;
} SIDE_PACKED;

struct side_type_gather_integer {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_integer type;
	uint16_t offset_bits;	/* bits */
} SIDE_PACKED;

struct side_type_gather_float {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_float type;
} SIDE_PACKED;

struct side_type_gather_string {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_string type;
} SIDE_PACKED;

struct side_type_gather_enum {
	const struct side_enum_mappings *mappings;
	const struct side_type *elem_type;
} SIDE_PACKED;

struct side_type_gather_struct {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	const struct side_type_struct *type;
	uint32_t size;		/* bytes */
} SIDE_PACKED;

struct side_type_gather_array {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_array type;
} SIDE_PACKED;

struct side_type_gather_vla {
	const struct side_type *length_type;	/* side_length() */

	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_vla type;
} SIDE_PACKED;

struct side_type_gather {
	union {
		struct side_type_gather_bool side_bool;
		struct side_type_gather_byte side_byte;
		struct side_type_gather_integer side_integer;
		struct side_type_gather_float side_float;
		struct side_type_gather_string side_string;
		struct side_type_gather_enum side_enum;
		struct side_type_gather_array side_array;
		struct side_type_gather_vla side_vla;
		struct side_type_gather_struct side_struct;
	} SIDE_PACKED u;
} SIDE_PACKED;

struct side_type {
	uint32_t type;	/* enum side_type_label */
	union {
		/* Stack-copy basic types */
		struct side_type_null side_null;
		struct side_type_bool side_bool;
		struct side_type_byte side_byte;
		struct side_type_string side_string;
		struct side_type_integer side_integer;
		struct side_type_float side_float;

		/* Stack-copy compound types */
		struct side_type_array side_array;
		struct side_type_vla side_vla;
		struct side_type_vla_visitor side_vla_visitor;
		const struct side_type_struct *side_struct;
		const struct side_type_variant *side_variant;

		/* Stack-copy enumeration types */
		struct side_type_enum side_enum;
		struct side_type_enum_bitmap side_enum_bitmap;

		/* Gather types */
		struct side_type_gather side_gather;
	} SIDE_PACKED u;
} SIDE_PACKED;

struct side_variant_option {
	int64_t range_begin;
	int64_t range_end;
	const struct side_type side_type;
} SIDE_PACKED;

struct side_type_variant {
	const struct side_type selector;
	const struct side_variant_option *options;
	const struct side_attr *attr;
	uint32_t nr_options;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_event_field {
	const char *field_name;
	struct side_type side_type;
} SIDE_PACKED;

enum side_event_flags {
	SIDE_EVENT_FLAG_VARIADIC = (1 << 0),
};

struct side_callback {
	union {
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			void *priv);
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_struct *var_struct,
			void *priv);
	} SIDE_PACKED u;
	void *priv;
} SIDE_PACKED;

union side_arg_static {
	/* Stack-copy basic types */
	union side_bool_value bool_value;
	uint8_t byte_value;
	uint64_t string_value;	/* const {uint8_t, uint16_t, uint32_t} * */
	union side_integer_value integer_value;
	union side_float_value float_value;

	/* Stack-copy compound types */
	const struct side_arg_vec *side_struct;
	const struct side_arg_variant *side_variant;
	const struct side_arg_vec *side_array;
	const struct side_arg_vec *side_vla;
	void *side_vla_app_visitor_ctx;

	/* Gather basic types */
	const void *side_bool_gather_ptr;
	const void *side_byte_gather_ptr;
	const void *side_integer_gather_ptr;
	const void *side_float_gather_ptr;
	const void *side_string_gather_ptr;

	/* Gather compound types */
	const void *side_array_gather_ptr;
	const void *side_struct_gather_ptr;
	struct {
		const void *ptr;
		const void *length_ptr;
	} SIDE_PACKED side_vla_gather;
} SIDE_PACKED;

struct side_arg_dynamic_vla {
	const struct side_arg *sav;
	const struct side_attr *attr;
	uint32_t len;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_arg_dynamic_struct {
	const struct side_arg_dynamic_field *fields;
	const struct side_attr *attr;
	uint32_t len;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_dynamic_struct_visitor {
	void *app_ctx;
	side_dynamic_struct_visitor_func visitor;
	const struct side_attr *attr;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_dynamic_vla_visitor {
	void *app_ctx;
	side_visitor_func visitor;
	const struct side_attr *attr;
	uint32_t nr_attr;
} SIDE_PACKED;

union side_arg_dynamic {
	/* Dynamic basic types */
	struct side_type_null side_null;
	struct {
		struct side_type_bool type;
		union side_bool_value value;
	} SIDE_PACKED side_bool;
	struct {
		struct side_type_byte type;
		uint8_t value;
	} SIDE_PACKED side_byte;
	struct {
		struct side_type_string type;
		uint64_t value;	/* const char * */
	} SIDE_PACKED side_string;
	struct {
		struct side_type_integer type;
		union side_integer_value value;
	} SIDE_PACKED side_integer;
	struct {
		struct side_type_float type;
		union side_float_value value;
	} SIDE_PACKED side_float;

	/* Dynamic compound types */
	const struct side_arg_dynamic_struct *side_dynamic_struct;
	const struct side_arg_dynamic_vla *side_dynamic_vla;

	struct side_dynamic_struct_visitor side_dynamic_struct_visitor;
	struct side_dynamic_vla_visitor side_dynamic_vla_visitor;
} SIDE_PACKED;

struct side_arg {
	uint32_t type;	/* enum side_type_label */
	union {
		union side_arg_static side_static;
		union side_arg_dynamic side_dynamic;
	} SIDE_PACKED u;
} SIDE_PACKED;

struct side_arg_variant {
	struct side_arg selector;
	struct side_arg option;
} SIDE_PACKED;

struct side_arg_vec {
	const struct side_arg *sav;
	uint32_t len;
} SIDE_PACKED;

struct side_arg_dynamic_field {
	const char *field_name;
	const struct side_arg elem;
} SIDE_PACKED;

/* The visitor pattern is a double-dispatch visitor. */
struct side_tracer_visitor_ctx {
	enum side_visitor_status (*write_elem)(
			const struct side_tracer_visitor_ctx *tracer_ctx,
			const struct side_arg *elem);
	void *priv;		/* Private tracer context. */
} SIDE_PACKED;

struct side_tracer_dynamic_struct_visitor_ctx {
	enum side_visitor_status (*write_field)(
			const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
			const struct side_arg_dynamic_field *dynamic_field);
	void *priv;		/* Private tracer context. */
} SIDE_PACKED;

struct side_event_description {
	uintptr_t *enabled;
	const char *provider_name;
	const char *event_name;
	const struct side_event_field *fields;
	const struct side_attr *attr;
	const struct side_callback *callbacks;
	uint64_t flags;
	uint32_t version;
	uint32_t loglevel;	/* enum side_loglevel */
	uint32_t nr_fields;
	uint32_t nr_attr;
	uint32_t nr_callbacks;
} SIDE_PACKED;

/* Event and type attributes */

#define side_attr(_key, _value)	\
	{ \
		.key = { \
			.p = (_key), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_TYPE_BYTE_ORDER_HOST, \
		}, \
		.value = SIDE_PARAM(_value), \
	}

#define side_attr_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_attr, __VA_ARGS__)

#define side_attr_null(_val)		{ .type = SIDE_ATTR_TYPE_NULL }
#define side_attr_bool(_val)		{ .type = SIDE_ATTR_TYPE_BOOL, .u = { .bool_value = !!(_val) } }
#define side_attr_u8(_val)		{ .type = SIDE_ATTR_TYPE_U8, .u = { .integer_value = { .side_u8 = (_val) } } }
#define side_attr_u16(_val)		{ .type = SIDE_ATTR_TYPE_U16, .u = { .integer_value = { .side_u16 = (_val) } } }
#define side_attr_u32(_val)		{ .type = SIDE_ATTR_TYPE_U32, .u = { .integer_value = { .side_u32 = (_val) } } }
#define side_attr_u64(_val)		{ .type = SIDE_ATTR_TYPE_U64, .u = { .integer_value = { .side_u64 = (_val) } } }
#define side_attr_s8(_val)		{ .type = SIDE_ATTR_TYPE_S8, .u = { .integer_value = { .side_s8 = (_val) } } }
#define side_attr_s16(_val)		{ .type = SIDE_ATTR_TYPE_S16, .u = { .integer_value = { .side_s16 = (_val) } } }
#define side_attr_s32(_val)		{ .type = SIDE_ATTR_TYPE_S32, .u = { .integer_value = { .side_s32 = (_val) } } }
#define side_attr_s64(_val)		{ .type = SIDE_ATTR_TYPE_S64, .u = { .integer_value = { .side_s64 = (_val) } } }
#define side_attr_float_binary16(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY16, .u = { .float_value = { .side_float_binary16 = (_val) } } }
#define side_attr_float_binary32(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY32, .u = { .float_value = { .side_float_binary32 = (_val) } } }
#define side_attr_float_binary64(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY64, .u = { .float_value = { .side_float_binary64 = (_val) } } }
#define side_attr_float_binary128(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY128, .u = { .float_value = { .side_float_binary128 = (_val) } } }

#define _side_attr_string(_val, _byte_order, _unit_size) \
	{ \
		.type = SIDE_ATTR_TYPE_STRING, \
		.u = { \
			.string_value = { \
				.p = (const void *) (_val), \
				.unit_size = _unit_size, \
				.byte_order = _byte_order, \
			}, \
		}, \
	}

#define side_attr_string(_val)		_side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t))
#define side_attr_string16(_val)	_side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t))
#define side_attr_string32(_val)	_side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t))

/* Stack-copy enumeration type definitions */

#define side_define_enum(_identifier, _mappings, _attr) \
	const struct side_enum_mappings _identifier = { \
		.mappings = _mappings, \
		.attr = _attr, \
		.nr_mappings = SIDE_ARRAY_SIZE(SIDE_PARAM(_mappings)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
	}

#define side_enum_mapping_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_enum_mapping, __VA_ARGS__)

#define side_enum_mapping_range(_label, _begin, _end) \
	{ \
		.range_begin = _begin, \
		.range_end = _end, \
		.label = { \
			.p = (_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_TYPE_BYTE_ORDER_HOST, \
		}, \
	}

#define side_enum_mapping_value(_label, _value) \
	{ \
		.range_begin = _value, \
		.range_end = _value, \
		.label = { \
			.p = (_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_TYPE_BYTE_ORDER_HOST, \
		}, \
	}

#define side_define_enum_bitmap(_identifier, _mappings, _attr) \
	const struct side_enum_bitmap_mappings _identifier = { \
		.mappings = _mappings, \
		.attr = _attr, \
		.nr_mappings = SIDE_ARRAY_SIZE(SIDE_PARAM(_mappings)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
	}

#define side_enum_bitmap_mapping_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_enum_bitmap_mapping, __VA_ARGS__)

#define side_enum_bitmap_mapping_range(_label, _begin, _end) \
	{ \
		.range_begin = _begin, \
		.range_end = _end, \
		.label = { \
			.p = (_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_TYPE_BYTE_ORDER_HOST, \
		}, \
	}

#define side_enum_bitmap_mapping_value(_label, _value) \
	{ \
		.range_begin = _value, \
		.range_end = _value, \
		.label = { \
			.p = (_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_TYPE_BYTE_ORDER_HOST, \
		}, \
	}

/* Stack-copy field and type definitions */

#define side_type_null(_attr) \
	{ \
		.type = SIDE_TYPE_NULL, \
		.u = { \
			.side_null = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}

#define side_type_bool(_attr) \
	{ \
		.type = SIDE_TYPE_BOOL, \
		.u = { \
			.side_bool = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.bool_size = sizeof(uint8_t), \
				.len_bits = 0, \
				.byte_order = SIDE_TYPE_BYTE_ORDER_HOST, \
			}, \
		}, \
	}

#define side_type_byte(_attr) \
	{ \
		.type = SIDE_TYPE_BYTE, \
		.u = { \
			.side_byte = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}

#define _side_type_string(_type, _byte_order, _unit_size, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_string = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.unit_size = _unit_size, \
				.byte_order = _byte_order, \
			}, \
		}, \
	}

#define side_type_dynamic() \
	{ \
		.type = SIDE_TYPE_DYNAMIC, \
	}

#define _side_type_integer(_type, _signedness, _byte_order, _integer_size, _len_bits, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_integer = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.integer_size = _integer_size, \
				.len_bits = _len_bits, \
				.signedness = _signedness, \
				.byte_order = _byte_order, \
			}, \
		}, \
	}

#define _side_type_float(_type, _byte_order, _float_size, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_float = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.float_size = _float_size, \
				.byte_order = _byte_order, \
			}, \
		}, \
	}

#define _side_field(_name, _type) \
	{ \
		.field_name = _name, \
		.side_type = _type, \
	}

#define side_option_range(_range_begin, _range_end, _type) \
	{ \
		.range_begin = _range_begin, \
		.range_end = _range_end, \
		.side_type = _type, \
	}

#define side_option(_value, _type) \
	side_option_range(_value, _value, SIDE_PARAM(_type))

/* Host endian */
#define side_type_u8(_attr)				_side_type_integer(SIDE_TYPE_U8, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, SIDE_PARAM(_attr))
#define side_type_u16(_attr)				_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), 0, SIDE_PARAM(_attr))
#define side_type_u32(_attr)				_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), 0, SIDE_PARAM(_attr))
#define side_type_u64(_attr)				_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint64_t), 0, SIDE_PARAM(_attr))
#define side_type_s8(_attr)				_side_type_integer(SIDE_TYPE_S8, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, SIDE_PARAM(_attr))
#define side_type_s16(_attr)				_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int16_t), 0, SIDE_PARAM(_attr))
#define side_type_s32(_attr)				_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int32_t), 0, SIDE_PARAM(_attr))
#define side_type_s64(_attr)				_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int64_t), 0, SIDE_PARAM(_attr))
#define side_type_pointer(_attr)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uintptr_t), 0, SIDE_PARAM(_attr))
#define side_type_float_binary16(_attr)			_side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float16), SIDE_PARAM(_attr))
#define side_type_float_binary32(_attr)			_side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float32), SIDE_PARAM(_attr))
#define side_type_float_binary64(_attr)			_side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float64), SIDE_PARAM(_attr))
#define side_type_float_binary128(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float128), SIDE_PARAM(_attr))
#define side_type_string(_attr)				_side_type_string(SIDE_TYPE_STRING_UTF8, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), SIDE_PARAM(_attr))
#define side_type_string16(_attr) 			_side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), SIDE_PARAM(_attr))
#define side_type_string32(_attr)	 		_side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), SIDE_PARAM(_attr))

#define side_field_null(_name, _attr)			_side_field(_name, side_type_null(SIDE_PARAM(_attr)))
#define side_field_bool(_name, _attr)			_side_field(_name, side_type_bool(SIDE_PARAM(_attr)))
#define side_field_u8(_name, _attr)			_side_field(_name, side_type_u8(SIDE_PARAM(_attr)))
#define side_field_u16(_name, _attr)			_side_field(_name, side_type_u16(SIDE_PARAM(_attr)))
#define side_field_u32(_name, _attr)			_side_field(_name, side_type_u32(SIDE_PARAM(_attr)))
#define side_field_u64(_name, _attr)			_side_field(_name, side_type_u64(SIDE_PARAM(_attr)))
#define side_field_s8(_name, _attr)			_side_field(_name, side_type_s8(SIDE_PARAM(_attr)))
#define side_field_s16(_name, _attr)			_side_field(_name, side_type_s16(SIDE_PARAM(_attr)))
#define side_field_s32(_name, _attr)			_side_field(_name, side_type_s32(SIDE_PARAM(_attr)))
#define side_field_s64(_name, _attr)			_side_field(_name, side_type_s64(SIDE_PARAM(_attr)))
#define side_field_byte(_name, _attr)			_side_field(_name, side_type_byte(SIDE_PARAM(_attr)))
#define side_field_pointer(_name, _attr)		_side_field(_name, side_type_pointer(SIDE_PARAM(_attr)))
#define side_field_float_binary16(_name, _attr)		_side_field(_name, side_type_float_binary16(SIDE_PARAM(_attr)))
#define side_field_float_binary32(_name, _attr)		_side_field(_name, side_type_float_binary32(SIDE_PARAM(_attr)))
#define side_field_float_binary64(_name, _attr)		_side_field(_name, side_type_float_binary64(SIDE_PARAM(_attr)))
#define side_field_float_binary128(_name, _attr)	_side_field(_name, side_type_float_binary128(SIDE_PARAM(_attr)))
#define side_field_string(_name, _attr)			_side_field(_name, side_type_string(SIDE_PARAM(_attr)))
#define side_field_string16(_name, _attr)		_side_field(_name, side_type_string16(SIDE_PARAM(_attr)))
#define side_field_string32(_name, _attr)		_side_field(_name, side_type_string32(SIDE_PARAM(_attr)))
#define side_field_dynamic(_name)			_side_field(_name, side_type_dynamic())

/* Little endian */
#define side_type_u16_le(_attr)				_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), 0, SIDE_PARAM(_attr))
#define side_type_u32_le(_attr)				_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), 0, SIDE_PARAM(_attr))
#define side_type_u64_le(_attr)				_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint64_t), 0, SIDE_PARAM(_attr))
#define side_type_s16_le(_attr)				_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int16_t), 0, SIDE_PARAM(_attr))
#define side_type_s32_le(_attr)				_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int32_t), 0, SIDE_PARAM(_attr))
#define side_type_s64_le(_attr)				_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int64_t), 0, SIDE_PARAM(_attr))
#define side_type_pointer_le(_attr)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uintptr_t), 0, SIDE_PARAM(_attr))
#define side_type_float_binary16_le(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float16), SIDE_PARAM(_attr))
#define side_type_float_binary32_le(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float32), SIDE_PARAM(_attr))
#define side_type_float_binary64_le(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float64), SIDE_PARAM(_attr))
#define side_type_float_binary128_le(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float128), SIDE_PARAM(_attr))
#define side_type_string16_le(_attr) 			_side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), SIDE_PARAM(_attr))
#define side_type_string32_le(_attr)		 	_side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), SIDE_PARAM(_attr))

#define side_field_u16_le(_name, _attr)			_side_field(_name, side_type_u16_le(SIDE_PARAM(_attr)))
#define side_field_u32_le(_name, _attr)			_side_field(_name, side_type_u32_le(SIDE_PARAM(_attr)))
#define side_field_u64_le(_name, _attr)			_side_field(_name, side_type_u64_le(SIDE_PARAM(_attr)))
#define side_field_s16_le(_name, _attr)			_side_field(_name, side_type_s16_le(SIDE_PARAM(_attr)))
#define side_field_s32_le(_name, _attr)			_side_field(_name, side_type_s32_le(SIDE_PARAM(_attr)))
#define side_field_s64_le(_name, _attr)			_side_field(_name, side_type_s64_le(SIDE_PARAM(_attr)))
#define side_field_pointer_le(_name, _attr)		_side_field(_name, side_type_pointer_le(SIDE_PARAM(_attr)))
#define side_field_float_binary16_le(_name, _attr)	_side_field(_name, side_type_float_binary16_le(SIDE_PARAM(_attr)))
#define side_field_float_binary32_le(_name, _attr)	_side_field(_name, side_type_float_binary32_le(SIDE_PARAM(_attr)))
#define side_field_float_binary64_le(_name, _attr)	_side_field(_name, side_type_float_binary64_le(SIDE_PARAM(_attr)))
#define side_field_float_binary128_le(_name, _attr)	_side_field(_name, side_type_float_binary128_le(SIDE_PARAM(_attr)))
#define side_field_string16_le(_name, _attr)		_side_field(_name, side_type_string16_le(SIDE_PARAM(_attr)))
#define side_field_string32_le(_name, _attr)		_side_field(_name, side_type_string32_le(SIDE_PARAM(_attr)))

/* Big endian */
#define side_type_u16_be(_attr)				_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), 0, SIDE_PARAM(_attr))
#define side_type_u32_be(_attr)				_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), 0, SIDE_PARAM(_attr))
#define side_type_u64_be(_attr)				_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint64_t), 0, SIDE_PARAM(_attr))
#define side_type_s16_be(_attr)				_side_type_integer(SIDE_TYPE_S16, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int16_t), 0, SIDE_PARAM(_attr))
#define side_type_s32_be(_attr)				_side_type_integer(SIDE_TYPE_S32, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int32_t), 0, SIDE_PARAM(_attr))
#define side_type_s64_be(_attr)				_side_type_integer(SIDE_TYPE_S64, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int64_t), 0, SIDE_PARAM(_attr))
#define side_type_pointer_be(_attr)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uintptr_t), 0, SIDE_PARAM(_attr))
#define side_type_float_binary16_be(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float16), SIDE_PARAM(_attr))
#define side_type_float_binary32_be(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float32), SIDE_PARAM(_attr))
#define side_type_float_binary64_be(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float64), SIDE_PARAM(_attr))
#define side_type_float_binary128_be(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float128), SIDE_PARAM(_attr))
#define side_type_string16_be(_attr) 			_side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), SIDE_PARAM(_attr))
#define side_type_string32_be(_attr)		 	_side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), SIDE_PARAM(_attr))

#define side_field_u16_be(_name, _attr)			_side_field(_name, side_type_u16_be(SIDE_PARAM(_attr)))
#define side_field_u32_be(_name, _attr)			_side_field(_name, side_type_u32_be(SIDE_PARAM(_attr)))
#define side_field_u64_be(_name, _attr)			_side_field(_name, side_type_u64_be(SIDE_PARAM(_attr)))
#define side_field_s16_be(_name, _attr)			_side_field(_name, side_type_s16_be(SIDE_PARAM(_attr)))
#define side_field_s32_be(_name, _attr)			_side_field(_name, side_type_s32_be(SIDE_PARAM(_attr)))
#define side_field_s64_be(_name, _attr)			_side_field(_name, side_type_s64_be(SIDE_PARAM(_attr)))
#define side_field_pointer_be(_name, _attr)		_side_field(_name, side_type_pointer_be(SIDE_PARAM(_attr)))
#define side_field_float_binary16_be(_name, _attr)	_side_field(_name, side_type_float_binary16_be(SIDE_PARAM(_attr)))
#define side_field_float_binary32_be(_name, _attr)	_side_field(_name, side_type_float_binary32_be(SIDE_PARAM(_attr)))
#define side_field_float_binary64_be(_name, _attr)	_side_field(_name, side_type_float_binary64_be(SIDE_PARAM(_attr)))
#define side_field_float_binary128_be(_name, _attr)	_side_field(_name, side_type_float_binary128_be(SIDE_PARAM(_attr)))
#define side_field_string16_be(_name, _attr)		_side_field(_name, side_type_string16_be(SIDE_PARAM(_attr)))
#define side_field_string32_be(_name, _attr)		_side_field(_name, side_type_string32_be(SIDE_PARAM(_attr)))

#define side_type_enum(_mappings, _elem_type) \
	{ \
		.type = SIDE_TYPE_ENUM, \
		.u = { \
			.side_enum = { \
				.mappings = _mappings, \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define side_field_enum(_name, _mappings, _elem_type) \
	_side_field(_name, side_type_enum(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define side_type_enum_bitmap(_mappings, _elem_type) \
	{ \
		.type = SIDE_TYPE_ENUM_BITMAP, \
		.u = { \
			.side_enum_bitmap = { \
				.mappings = _mappings, \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define side_field_enum_bitmap(_name, _mappings, _elem_type) \
	_side_field(_name, side_type_enum_bitmap(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define side_type_struct(_struct) \
	{ \
		.type = SIDE_TYPE_STRUCT, \
		.u = { \
			.side_struct = _struct, \
		}, \
	}
#define side_field_struct(_name, _struct) \
	_side_field(_name, side_type_struct(SIDE_PARAM(_struct)))

#define _side_type_struct_define(_fields, _attr) \
	{ \
		.fields = _fields, \
		.attr = _attr, \
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
		.nr_attr  = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
	}

#define side_define_struct(_identifier, _fields, _attr) \
	const struct side_type_struct _identifier = _side_type_struct_define(SIDE_PARAM(_fields), SIDE_PARAM(_attr))

#define side_struct_literal(_fields, _attr) \
	SIDE_COMPOUND_LITERAL(const struct side_type_struct, \
		_side_type_struct_define(SIDE_PARAM(_fields), SIDE_PARAM(_attr)))

#define side_type_variant(_variant) \
	{ \
		.type = SIDE_TYPE_VARIANT, \
		.u = { \
			.side_variant = _variant, \
		}, \
	}
#define side_field_variant(_name, _variant) \
	_side_field(_name, side_type_variant(SIDE_PARAM(_variant)))

#define _side_type_variant_define(_selector, _options, _attr) \
	{ \
		.selector = _selector, \
		.options = _options, \
		.attr = _attr, \
		.nr_options = SIDE_ARRAY_SIZE(SIDE_PARAM(_options)), \
		.nr_attr  = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
	}

#define side_define_variant(_identifier, _selector, _options, _attr) \
	const struct side_type_variant _identifier = \
		_side_type_variant_define(SIDE_PARAM(_selector), SIDE_PARAM(_options), SIDE_PARAM(_attr))

#define side_variant_literal(_selector, _options, _attr) \
	SIDE_COMPOUND_LITERAL(const struct side_type_variant, \
		_side_type_variant_define(SIDE_PARAM(_selector), SIDE_PARAM(_options), SIDE_PARAM(_attr)))

#define side_type_array(_elem_type, _length, _attr) \
	{ \
		.type = SIDE_TYPE_ARRAY, \
		.u = { \
			.side_array = { \
				.elem_type = _elem_type, \
				.attr = _attr, \
				.length = _length, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}
#define side_field_array(_name, _elem_type, _length, _attr) \
	_side_field(_name, side_type_array(SIDE_PARAM(_elem_type), _length, SIDE_PARAM(_attr)))

#define side_type_vla(_elem_type, _attr) \
	{ \
		.type = SIDE_TYPE_VLA, \
		.u = { \
			.side_vla = { \
				.elem_type = _elem_type, \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}
#define side_field_vla(_name, _elem_type, _attr) \
	_side_field(_name, side_type_vla(SIDE_PARAM(_elem_type), SIDE_PARAM(_attr)))

#define side_type_vla_visitor(_elem_type, _visitor, _attr) \
	{ \
		.type = SIDE_TYPE_VLA_VISITOR, \
		.u = { \
			.side_vla_visitor = { \
				.elem_type = SIDE_PARAM(_elem_type), \
				.visitor = _visitor, \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}
#define side_field_vla_visitor(_name, _elem_type, _visitor, _attr) \
	_side_field(_name, side_type_vla_visitor(SIDE_PARAM(_elem_type), _visitor, SIDE_PARAM(_attr)))

/* Gather field and type definitions */

#define side_type_gather_byte(_offset, _access_mode, _attr) \
	{ \
		.type = SIDE_TYPE_GATHER_BYTE, \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_byte = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_field_gather_byte(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_byte(_offset, _access_mode, SIDE_PARAM(_attr)))

#define _side_type_gather_bool(_byte_order, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	{ \
		.type = SIDE_TYPE_GATHER_BOOL, \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_bool = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
							.bool_size = _bool_size, \
							.len_bits = _len_bits, \
							.byte_order = _byte_order, \
						}, \
						.offset_bits = _offset_bits, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_HOST, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr)
#define side_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_LE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr)
#define side_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_BE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr)

#define side_field_gather_bool(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_field(_name, side_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr)))
#define side_field_gather_bool_le(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_field(_name, side_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr)))
#define side_field_gather_bool_be(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_field(_name, side_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr)))

#define _side_type_gather_integer(_type, _signedness, _byte_order, _offset, \
		_integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_integer = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
							.integer_size = _integer_size, \
							.len_bits = _len_bits, \
							.signedness = _signedness, \
							.byte_order = _byte_order, \
						}, \
						.offset_bits = _offset_bits, \
					}, \
				}, \
			}, \
		}, \
	}

#define side_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr))
#define side_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr))

#define side_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr))
#define side_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr))

#define side_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr))
#define side_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr))

#define side_field_gather_unsigned_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_field(_name, side_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr)))
#define side_field_gather_signed_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_field(_name, side_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr)))

#define side_field_gather_unsigned_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_field(_name, side_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr)))
#define side_field_gather_signed_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_field(_name, side_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr)))

#define side_field_gather_unsigned_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_field(_name, side_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr)))
#define side_field_gather_signed_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_side_field(_name, side_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(_attr)))

#define side_type_gather_pointer(_offset, _access_mode, _attr) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_HOST, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_PARAM(_attr))
#define side_field_gather_pointer(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_pointer(_offset, _access_mode, SIDE_PARAM(_attr)))

#define side_type_gather_pointer_le(_offset, _access_mode, _attr) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_LE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_PARAM(_attr))
#define side_field_gather_pointer_le(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_pointer_le(_offset, _access_mode, SIDE_PARAM(_attr)))

#define side_type_gather_pointer_be(_offset, _access_mode, _attr) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_BE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_PARAM(_attr))
#define side_field_gather_pointer_be(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_pointer_be(_offset, _access_mode, SIDE_PARAM(_attr)))

#define _side_type_gather_float(_byte_order, _offset, _float_size, _access_mode, _attr) \
	{ \
		.type = SIDE_TYPE_GATHER_FLOAT, \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_float = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
							.float_size = _float_size, \
							.byte_order = _byte_order, \
						}, \
					}, \
				}, \
			}, \
		}, \
	}

#define side_type_gather_float(_offset, _float_size, _access_mode, _attr) \
	_side_type_gather_float(SIDE_TYPE_FLOAT_WORD_ORDER_HOST, _offset, _float_size, _access_mode, _attr)
#define side_type_gather_float_le(_offset, _float_size, _access_mode, _attr) \
	_side_type_gather_float(SIDE_TYPE_BYTE_ORDER_LE, _offset, _float_size, _access_mode, _attr)
#define side_type_gather_float_be(_offset, _float_size, _access_mode, _attr) \
	_side_type_gather_float(SIDE_TYPE_BYTE_ORDER_BE, _offset, _float_size, _access_mode, _attr)

#define side_field_gather_float(_name, _offset, _float_size, _access_mode, _attr) \
	_side_field(_name, side_type_gather_float(_offset, _float_size, _access_mode, _attr))
#define side_field_gather_float_le(_name, _offset, _float_size, _access_mode, _attr) \
	_side_field(_name, side_type_gather_float_le(_offset, _float_size, _access_mode, _attr))
#define side_field_gather_float_be(_name, _offset, _float_size, _access_mode, _attr) \
	_side_field(_name, side_type_gather_float_be(_offset, _float_size, _access_mode, _attr))

#define _side_type_gather_string(_offset, _byte_order, _unit_size, _access_mode, _attr) \
	{ \
		.type = SIDE_TYPE_GATHER_STRING, \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_string = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
							.unit_size = _unit_size, \
							.byte_order = _byte_order, \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_type_gather_string(_offset, _access_mode, _attr) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), _access_mode, SIDE_PARAM(_attr))
#define side_field_gather_string(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_string(_offset, _access_mode, SIDE_PARAM(_attr)))

#define side_type_gather_string16(_offset, _access_mode, _attr) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), _access_mode, SIDE_PARAM(_attr))
#define side_type_gather_string16_le(_offset, _access_mode, _attr) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), _access_mode, SIDE_PARAM(_attr))
#define side_type_gather_string16_be(_offset, _access_mode, _attr) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), _access_mode, SIDE_PARAM(_attr))

#define side_field_gather_string16(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_string16(_offset, _access_mode, SIDE_PARAM(_attr)))
#define side_field_gather_string16_le(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_string16_le(_offset, _access_mode, SIDE_PARAM(_attr)))
#define side_field_gather_string16_be(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_string16_be(_offset, _access_mode, SIDE_PARAM(_attr)))

#define side_type_gather_string32(_offset, _access_mode, _attr) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), _access_mode, SIDE_PARAM(_attr))
#define side_type_gather_string32_le(_offset, _access_mode, _attr) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), _access_mode, SIDE_PARAM(_attr))
#define side_type_gather_string32_be(_offset, _access_mode, _attr) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), _access_mode, SIDE_PARAM(_attr))

#define side_field_gather_string32(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_string32(_offset, _access_mode, SIDE_PARAM(_attr)))
#define side_field_gather_string32_le(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_string32_le(_offset, _access_mode, SIDE_PARAM(_attr)))
#define side_field_gather_string32_be(_name, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_string32_be(_offset, _access_mode, SIDE_PARAM(_attr)))

#define side_type_gather_enum(_mappings, _elem_type) \
	{ \
		.type = SIDE_TYPE_GATHER_ENUM, \
		.u = { \
			.side_enum = { \
				.mappings = _mappings, \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define side_field_gather_enum(_name, _mappings, _elem_type) \
	_side_field(_name, side_type_gather_enum(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define side_type_gather_struct(_struct_gather, _offset, _size, _access_mode) \
	{ \
		.type = SIDE_TYPE_GATHER_STRUCT, \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_struct = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = _struct_gather, \
						.size = _size, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_field_gather_struct(_name, _struct_gather, _offset, _size, _access_mode) \
	_side_field(_name, side_type_gather_struct(SIDE_PARAM(_struct_gather), _offset, _size, _access_mode))

#define side_type_gather_array(_elem_type_gather, _length, _offset, _access_mode, _attr) \
	{ \
		.type = SIDE_TYPE_GATHER_ARRAY, \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_array = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.elem_type = _elem_type_gather, \
							.attr = _attr, \
							.length = _length, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_field_gather_array(_name, _elem_type, _length, _offset, _access_mode, _attr) \
	_side_field(_name, side_type_gather_array(SIDE_PARAM(_elem_type), _length, _offset, _access_mode, SIDE_PARAM(_attr)))

#define side_type_gather_vla(_elem_type_gather, _offset, _access_mode, _length_type_gather, _attr) \
	{ \
		.type = SIDE_TYPE_GATHER_VLA, \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_vla = { \
						.length_type = _length_type_gather, \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.elem_type = _elem_type_gather, \
							.attr = _attr, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_field_gather_vla(_name, _elem_type_gather, _offset, _access_mode, _length_type_gather, _attr) \
	_side_field(_name, side_type_gather_vla(SIDE_PARAM(_elem_type_gather), _offset, _access_mode, SIDE_PARAM(_length_type_gather), SIDE_PARAM(_attr)))

#define side_elem(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type, __VA_ARGS__)

#define side_length(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type, __VA_ARGS__)

#define side_field_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_event_field, __VA_ARGS__)

#define side_option_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_variant_option, __VA_ARGS__)

/* Stack-copy field arguments */

#define side_arg_null(_val)		{ .type = SIDE_TYPE_NULL }
#define side_arg_bool(_val)		{ .type = SIDE_TYPE_BOOL, .u = { .side_static = { .bool_value = { .side_bool8 = !!(_val) } } } }
#define side_arg_byte(_val)		{ .type = SIDE_TYPE_BYTE, .u = { .side_static = { .byte_value = (_val) } } }
#define side_arg_string(_val)		{ .type = SIDE_TYPE_STRING_UTF8, .u = { .side_static = { .string_value = (uintptr_t) (_val) } } }
#define side_arg_string16(_val)		{ .type = SIDE_TYPE_STRING_UTF16, .u = { .side_static = { .string_value = (uintptr_t) (_val) } } }
#define side_arg_string32(_val)		{ .type = SIDE_TYPE_STRING_UTF32, .u = { .side_static = { .string_value = (uintptr_t) (_val) } } }

#define side_arg_u8(_val)		{ .type = SIDE_TYPE_U8, .u = { .side_static = {  .integer_value = { .side_u8 = (_val) } } } }
#define side_arg_u16(_val)		{ .type = SIDE_TYPE_U16, .u = { .side_static = { .integer_value = { .side_u16 = (_val) } } } }
#define side_arg_u32(_val)		{ .type = SIDE_TYPE_U32, .u = { .side_static = { .integer_value = { .side_u32 = (_val) } } } }
#define side_arg_u64(_val)		{ .type = SIDE_TYPE_U64, .u = { .side_static = { .integer_value = { .side_u64 = (_val) } } } }
#define side_arg_s8(_val)		{ .type = SIDE_TYPE_S8, .u = { .side_static = { .integer_value = { .side_s8 = (_val) } } } }
#define side_arg_s16(_val)		{ .type = SIDE_TYPE_S16, .u = { .side_static = { .integer_value = { .side_s16 = (_val) } } } }
#define side_arg_s32(_val)		{ .type = SIDE_TYPE_S32, .u = { .side_static = { .integer_value = { .side_s32 = (_val) } } } }
#define side_arg_s64(_val)		{ .type = SIDE_TYPE_S64, .u = { .side_static = { .integer_value = { .side_s64 = (_val) } } } }
#define side_arg_pointer(_val)		{ .type = SIDE_TYPE_POINTER, .u = { .side_static = { .integer_value = { .side_uptr = (uintptr_t) (_val) } } } }
#define side_arg_float_binary16(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY16, .u = { .side_static = { .float_value = { .side_float_binary16 = (_val) } } } }
#define side_arg_float_binary32(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY32, .u = { .side_static = { .float_value = { .side_float_binary32 = (_val) } } } }
#define side_arg_float_binary64(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY64, .u = { .side_static = { .float_value = { .side_float_binary64 = (_val) } } } }
#define side_arg_float_binary128(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY128, .u = { .side_static = { .float_value = { .side_float_binary128 = (_val) } } } }

#define side_arg_struct(_side_type)	{ .type = SIDE_TYPE_STRUCT, .u = { .side_static = { .side_struct = (_side_type) } } }

#define side_arg_define_variant(_identifier, _selector_val, _option) \
	const struct side_arg_variant _identifier = { \
		.selector = _selector_val, \
		.option = _option, \
	}
#define side_arg_variant(_side_variant) \
	{ \
		.type = SIDE_TYPE_VARIANT, \
		.u = { \
			.side_static = { \
				.side_variant = (_side_variant), \
			}, \
		}, \
	}

#define side_arg_array(_side_type)	{ .type = SIDE_TYPE_ARRAY, .u = { .side_static = { .side_array = (_side_type) } } }
#define side_arg_vla(_side_type)	{ .type = SIDE_TYPE_VLA, .u = { .side_static = { .side_vla = (_side_type) } } }
#define side_arg_vla_visitor(_ctx)	{ .type = SIDE_TYPE_VLA_VISITOR, .u = { .side_static = { .side_vla_app_visitor_ctx = (_ctx) } } }

/* Gather field arguments */

#define side_arg_gather_bool(_ptr)		{ .type = SIDE_TYPE_GATHER_BOOL, .u = { .side_static = { .side_bool_gather_ptr = (_ptr) } } }
#define side_arg_gather_byte(_ptr)		{ .type = SIDE_TYPE_GATHER_BYTE, .u = { .side_static = { .side_byte_gather_ptr = (_ptr) } } }
#define side_arg_gather_pointer(_ptr)		{ .type = SIDE_TYPE_GATHER_POINTER, .u = { .side_static = { .side_integer_gather_ptr = (_ptr) } } }
#define side_arg_gather_integer(_ptr)		{ .type = SIDE_TYPE_GATHER_INTEGER, .u = { .side_static = { .side_integer_gather_ptr = (_ptr) } } }
#define side_arg_gather_float(_ptr)		{ .type = SIDE_TYPE_GATHER_FLOAT, .u = { .side_static = { .side_float_gather_ptr = (_ptr) } } }
#define side_arg_gather_string(_ptr)		{ .type = SIDE_TYPE_GATHER_STRING, .u = { .side_static = { .side_string_gather_ptr = (_ptr) } } }
#define side_arg_gather_struct(_ptr)		{ .type = SIDE_TYPE_GATHER_STRUCT, .u = { .side_static = { .side_struct_gather_ptr = (_ptr) } } }
#define side_arg_gather_array(_ptr)		{ .type = SIDE_TYPE_GATHER_ARRAY, .u = { .side_static = { .side_array_gather_ptr = (_ptr) } } }
#define side_arg_gather_vla(_ptr, _length_ptr)	{ .type = SIDE_TYPE_GATHER_VLA, .u = { .side_static = { .side_vla_gather = { .ptr = (_ptr), .length_ptr = (_length_ptr) } } } }

/* Dynamic field arguments */

#define side_arg_dynamic_null(_attr) \
	{ \
		.type = SIDE_TYPE_DYNAMIC_NULL, \
		.u = { \
			.side_dynamic = { \
				.side_null = { \
					.attr = _attr, \
					.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_bool(_val, _attr) \
	{ \
		.type = SIDE_TYPE_DYNAMIC_BOOL, \
		.u = { \
			.side_dynamic = { \
				.side_bool = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
						.bool_size = sizeof(uint8_t), \
						.len_bits = 0, \
						.byte_order = SIDE_TYPE_BYTE_ORDER_HOST, \
					}, \
					.value = { \
						.side_bool8 = !!(_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_byte(_val, _attr) \
	{ \
		.type = SIDE_TYPE_DYNAMIC_BYTE, \
		.u = { \
			.side_dynamic = { \
				.side_byte = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
					}, \
					.value = (_val), \
				}, \
			}, \
		}, \
	}
#define _side_arg_dynamic_string(_val, _byte_order, _unit_size, _attr) \
	{ \
		.type = SIDE_TYPE_DYNAMIC_STRING, \
		.u = { \
			.side_dynamic = { \
				.side_string = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
						.unit_size = _unit_size, \
						.byte_order = _byte_order, \
					}, \
					.value = (uintptr_t) (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_string(_val, _attr) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), SIDE_PARAM(_attr))
#define side_arg_dynamic_string16(_val, _attr) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), SIDE_PARAM(_attr))
#define side_arg_dynamic_string16_le(_val, _attr) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), SIDE_PARAM(_attr))
#define side_arg_dynamic_string16_be(_val, _attr) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), SIDE_PARAM(_attr))
#define side_arg_dynamic_string32(_val, _attr) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), SIDE_PARAM(_attr))
#define side_arg_dynamic_string32_le(_val, _attr) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), SIDE_PARAM(_attr))
#define side_arg_dynamic_string32_be(_val, _attr) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), SIDE_PARAM(_attr))

#define _side_arg_dynamic_integer(_field, _val, _type, _signedness, _byte_order, _integer_size, _len_bits, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_dynamic = { \
				.side_integer = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
						.integer_size = _integer_size, \
						.len_bits = _len_bits, \
						.signedness = _signedness, \
						.byte_order = _byte_order, \
					}, \
					.value = { \
						_field = (_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_u8(_val, _attr) \
	_side_arg_dynamic_integer(.side_u8, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, SIDE_PARAM(_attr))
#define side_arg_dynamic_s8(_val, _attr) \
	_side_arg_dynamic_integer(.side_s8, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, SIDE_PARAM(_attr))

#define _side_arg_dynamic_u16(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_u16, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint16_t), 0, SIDE_PARAM(_attr))
#define _side_arg_dynamic_u32(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_u32, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint32_t), 0, SIDE_PARAM(_attr))
#define _side_arg_dynamic_u64(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_u64, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint64_t), 0, SIDE_PARAM(_attr))

#define _side_arg_dynamic_s16(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_s16, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int16_t), 0, SIDE_PARAM(_attr))
#define _side_arg_dynamic_s32(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_s32, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int32_t), 0, SIDE_PARAM(_attr))
#define _side_arg_dynamic_s64(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_s64, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int64_t), 0, SIDE_PARAM(_attr))

#define _side_arg_dynamic_pointer(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_uptr, (uintptr_t) (_val), SIDE_TYPE_DYNAMIC_POINTER, false, _byte_order, \
			sizeof(uintptr_t), 0, SIDE_PARAM(_attr))

#define _side_arg_dynamic_float(_field, _val, _type, _byte_order, _float_size, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_dynamic = { \
				.side_float = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
						.float_size = _float_size, \
						.byte_order = _byte_order, \
					}, \
					.value = { \
						_field = (_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_float_binary16(_val, _byte_order, _attr) \
	_side_arg_dynamic_float(.side_float_binary16, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float16), SIDE_PARAM(_attr))
#define _side_arg_dynamic_float_binary32(_val, _byte_order, _attr) \
	_side_arg_dynamic_float(.side_float_binary32, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float32), SIDE_PARAM(_attr))
#define _side_arg_dynamic_float_binary64(_val, _byte_order, _attr) \
	_side_arg_dynamic_float(.side_float_binary64, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float64), SIDE_PARAM(_attr))
#define _side_arg_dynamic_float_binary128(_val, _byte_order, _attr) \
	_side_arg_dynamic_float(.side_float_binary128, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float128), SIDE_PARAM(_attr))

/* Host endian */
#define side_arg_dynamic_u16(_val, _attr) 		_side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_u32(_val, _attr) 		_side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_u64(_val, _attr) 		_side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_s16(_val, _attr) 		_side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_s32(_val, _attr) 		_side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_s64(_val, _attr) 		_side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_pointer(_val, _attr) 		_side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary16(_val, _attr)	_side_arg_dynamic_float_binary16(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary32(_val, _attr)	_side_arg_dynamic_float_binary32(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary64(_val, _attr)	_side_arg_dynamic_float_binary64(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary128(_val, _attr)	_side_arg_dynamic_float_binary128(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_PARAM(_attr))

/* Little endian */
#define side_arg_dynamic_u16_le(_val, _attr) 			_side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_u32_le(_val, _attr) 			_side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_u64_le(_val, _attr) 			_side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_s16_le(_val, _attr) 			_side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_s32_le(_val, _attr) 			_side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_s64_le(_val, _attr) 			_side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_pointer_le(_val, _attr) 		_side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary16_le(_val, _attr)		_side_arg_dynamic_float_binary16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary32_le(_val, _attr)		_side_arg_dynamic_float_binary32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary64_le(_val, _attr)		_side_arg_dynamic_float_binary64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary128_le(_val, _attr)	_side_arg_dynamic_float_binary128(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM(_attr))

/* Big endian */
#define side_arg_dynamic_u16_be(_val, _attr) 			_side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_u32_be(_val, _attr) 			_side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_u64_be(_val, _attr) 			_side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_s16_be(_val, _attr) 			_side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_s32_be(_val, _attr) 			_side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_s64_be(_val, _attr) 			_side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_pointer_be(_val, _attr) 		_side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary16_be(_val, _attr)		_side_arg_dynamic_float_binary16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary32_be(_val, _attr)		_side_arg_dynamic_float_binary32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary64_be(_val, _attr)		_side_arg_dynamic_float_binary64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))
#define side_arg_dynamic_float_binary128_be(_val, _attr)	_side_arg_dynamic_float_binary128(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM(_attr))

#define side_arg_dynamic_vla(_vla) \
	{ \
		.type = SIDE_TYPE_DYNAMIC_VLA, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_vla = (_vla), \
			}, \
		}, \
	}

#define side_arg_dynamic_vla_visitor(_dynamic_vla_visitor, _ctx, _attr) \
	{ \
		.type = SIDE_TYPE_DYNAMIC_VLA_VISITOR, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_vla_visitor = { \
					.app_ctx = _ctx, \
					.visitor = _dynamic_vla_visitor, \
					.attr = _attr, \
					.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_struct(_struct) \
	{ \
		.type = SIDE_TYPE_DYNAMIC_STRUCT, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_struct = (_struct), \
			}, \
		}, \
	}

#define side_arg_dynamic_struct_visitor(_dynamic_struct_visitor, _ctx, _attr) \
	{ \
		.type = SIDE_TYPE_DYNAMIC_STRUCT_VISITOR, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_struct_visitor = { \
					.app_ctx = _ctx, \
					.visitor = _dynamic_struct_visitor, \
					.attr = _attr, \
					.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_define_vec(_identifier, _sav, _attr) \
	const struct side_arg _identifier##_vec[] = { _sav }; \
	const struct side_arg_dynamic_vla _identifier = { \
		.sav = _identifier##_vec, \
		.attr = _attr, \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
	}

#define side_arg_dynamic_define_struct(_identifier, _struct_fields, _attr) \
	const struct side_arg_dynamic_field _identifier##_fields[] = { _struct_fields }; \
	const struct side_arg_dynamic_struct _identifier = { \
		.fields = _identifier##_fields, \
		.attr = _attr, \
		.len = SIDE_ARRAY_SIZE(_identifier##_fields), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
	}

#define side_arg_define_vec(_identifier, _sav) \
	const struct side_arg _identifier##_vec[] = { _sav }; \
	const struct side_arg_vec _identifier = { \
		.sav = _identifier##_vec, \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
	}

#define side_arg_dynamic_field(_name, _elem) \
	{ \
		.field_name = _name, \
		.elem = _elem, \
	}

/*
 * Event instrumentation description registration, runtime enabled state
 * check, and instrumentation invocation.
 */

#define side_arg_list(...)	__VA_ARGS__

#define side_event_cond(_identifier) \
	if (side_unlikely(__atomic_load_n(&side_event_enable__##_identifier, \
					__ATOMIC_RELAXED)))

#define side_event_call(_identifier, _sav) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = side_sav, \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		side_call(&(_identifier), &side_arg_vec); \
	}

#define side_event(_identifier, _sav) \
	side_event_cond(_identifier) \
		side_event_call(_identifier, SIDE_PARAM(_sav))

#define side_event_call_variadic(_identifier, _sav, _var_fields, _attr) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = side_sav, \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		const struct side_arg_dynamic_field side_fields[] = { _var_fields }; \
		const struct side_arg_dynamic_struct var_struct = { \
			.fields = side_fields, \
			.attr = _attr, \
			.len = SIDE_ARRAY_SIZE(side_fields), \
			.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		}; \
		side_call_variadic(&(_identifier), &side_arg_vec, &var_struct); \
	}

#define side_event_variadic(_identifier, _sav, _var, _attr) \
	side_event_cond(_identifier) \
		side_event_call_variadic(_identifier, SIDE_PARAM(_sav), SIDE_PARAM(_var), SIDE_PARAM(_attr))

#define _side_define_event(_linkage, _identifier, _provider, _event, _loglevel, _fields, _attr, _flags) \
	_linkage uintptr_t side_event_enable__##_identifier __attribute__((section("side_event_enable"))); \
	_linkage struct side_event_description __attribute__((section("side_event_description"))) \
			_identifier = { \
		.enabled = &(side_event_enable__##_identifier), \
		.provider_name = _provider, \
		.event_name = _event, \
		.fields = _fields, \
		.attr = _attr, \
		.callbacks = &side_empty_callback, \
		.flags = (_flags), \
		.version = 0, \
		.loglevel = _loglevel, \
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		.nr_callbacks = 0, \
	}; \
	static const struct side_event_description *side_event_ptr__##_identifier \
		__attribute__((section("side_event_description_ptr"), used)) = &(_identifier);

#define side_static_event(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(static, _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_PARAM(_attr), 0)

#define side_static_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(static, _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_PARAM(_attr), SIDE_EVENT_FLAG_VARIADIC)

#define side_hidden_event(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(__attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_PARAM(_attr), 0)

#define side_hidden_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(__attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_PARAM(_attr), SIDE_EVENT_FLAG_VARIADIC)

#define side_export_event(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(__attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_PARAM(_attr), 0)

#define side_export_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(__attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_PARAM(_attr), SIDE_EVENT_FLAG_VARIADIC)

#define side_declare_event(_identifier) \
	extern uintptr_t side_event_enable_##_identifier; \
	extern struct side_event_description _identifier

#ifdef __cplusplus
extern "C" {
#endif

extern const struct side_callback side_empty_callback;

void side_call(const struct side_event_description *desc,
	const struct side_arg_vec *side_arg_vec);
void side_call_variadic(const struct side_event_description *desc,
	const struct side_arg_vec *side_arg_vec,
	const struct side_arg_dynamic_struct *var_struct);

struct side_events_register_handle *side_events_register(struct side_event_description **events,
		uint32_t nr_events);
void side_events_unregister(struct side_events_register_handle *handle);

/*
 * Userspace tracer registration API. This allows userspace tracers to
 * register event notification callbacks to be notified of the currently
 * registered instrumentation, and to register their callbacks to
 * specific events.
 */
typedef void (*side_tracer_callback_func)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			void *priv);
typedef void (*side_tracer_callback_variadic_func)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_struct *var_struct,
			void *priv);

int side_tracer_callback_register(struct side_event_description *desc,
		side_tracer_callback_func call,
		void *priv);
int side_tracer_callback_variadic_register(struct side_event_description *desc,
		side_tracer_callback_variadic_func call_variadic,
		void *priv);
int side_tracer_callback_unregister(struct side_event_description *desc,
		side_tracer_callback_func call,
		void *priv);
int side_tracer_callback_variadic_unregister(struct side_event_description *desc,
		side_tracer_callback_variadic_func call_variadic,
		void *priv);

enum side_tracer_notification {
	SIDE_TRACER_NOTIFICATION_INSERT_EVENTS,
	SIDE_TRACER_NOTIFICATION_REMOVE_EVENTS,
};

/* Callback is invoked with side library internal lock held. */
struct side_tracer_handle *side_tracer_event_notification_register(
		void (*cb)(enum side_tracer_notification notif,
			struct side_event_description **events, uint32_t nr_events, void *priv),
		void *priv);
void side_tracer_event_notification_unregister(struct side_tracer_handle *handle);

/*
 * Explicit hooks to initialize/finalize the side instrumentation
 * library. Those are also library constructor/destructor.
 */
void side_init(void) __attribute__((constructor));
void side_exit(void) __attribute__((destructor));

/*
 * The following constructors/destructors perform automatic registration
 * of the declared side events. Those may have to be called explicitly
 * in a statically linked library.
 */

/*
 * These weak symbols, the constructor, and destructor take care of
 * registering only _one_ instance of the side instrumentation per
 * shared-ojbect (or for the whole main program).
 */
extern struct side_event_description * __start_side_event_description_ptr[]
	__attribute__((weak, visibility("hidden")));
extern struct side_event_description * __stop_side_event_description_ptr[]
	__attribute__((weak, visibility("hidden")));
int side_event_description_ptr_registered
        __attribute__((weak, visibility("hidden")));
struct side_events_register_handle *side_events_handle
	__attribute__((weak, visibility("hidden")));

static void
side_event_description_ptr_init(void)
	__attribute__((no_instrument_function))
	__attribute__((constructor));
static void
side_event_description_ptr_init(void)
{
	if (side_event_description_ptr_registered++)
		return;
	side_events_handle = side_events_register(__start_side_event_description_ptr,
		__stop_side_event_description_ptr - __start_side_event_description_ptr);
}

static void
side_event_description_ptr_exit(void)
	__attribute__((no_instrument_function))
	__attribute__((destructor));
static void
side_event_description_ptr_exit(void)
{
	if (--side_event_description_ptr_registered)
		return;
	side_events_unregister(side_events_handle);
	side_events_handle = NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* _SIDE_TRACE_H */
