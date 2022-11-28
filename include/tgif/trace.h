// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _TGIF_TRACE_H
#define _TGIF_TRACE_H

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <tgif/macros.h>
#include <tgif/endian.h>

/*
 * TGIF stands for "Trace Generation Instrumentation Framework"
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
 *   application needs to copy each argument (tgif_arg_...()) onto the
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
 *   type TGIF_TYPE_DYNAMIC place holder in the stack-copy type system.
 */

//TODO: as those structures will be ABI, we need to either contgifr them
//fixed forever, or think of a scheme that would allow their binary
//representation to be extended if need be.

struct tgif_arg;
struct tgif_arg_vec;
struct tgif_arg_dynamic;
struct tgif_arg_dynamic_field;
struct tgif_arg_dynamic_vla;
struct tgif_type;
struct tgif_event_field;
struct tgif_tracer_visitor_ctx;
struct tgif_tracer_dynamic_struct_visitor_ctx;
struct tgif_event_description;
struct tgif_arg_dynamic_struct;
struct tgif_events_register_handle;

enum tgif_type_label {
	/* Stack-copy basic types */
	TGIF_TYPE_NULL,
	TGIF_TYPE_BOOL,
	TGIF_TYPE_U8,
	TGIF_TYPE_U16,
	TGIF_TYPE_U32,
	TGIF_TYPE_U64,
	TGIF_TYPE_S8,
	TGIF_TYPE_S16,
	TGIF_TYPE_S32,
	TGIF_TYPE_S64,
	TGIF_TYPE_BYTE,
	TGIF_TYPE_POINTER,
	TGIF_TYPE_FLOAT_BINARY16,
	TGIF_TYPE_FLOAT_BINARY32,
	TGIF_TYPE_FLOAT_BINARY64,
	TGIF_TYPE_FLOAT_BINARY128,
	TGIF_TYPE_STRING_UTF8,
	TGIF_TYPE_STRING_UTF16,
	TGIF_TYPE_STRING_UTF32,

	/* Stack-copy compound types */
	TGIF_TYPE_STRUCT,
	TGIF_TYPE_ARRAY,
	TGIF_TYPE_VLA,
	TGIF_TYPE_VLA_VISITOR,

	/* Stack-copy enumeration types */
	TGIF_TYPE_ENUM,
	TGIF_TYPE_ENUM_BITMAP,

	/* Stack-copy place holder for dynamic types */
	TGIF_TYPE_DYNAMIC,

	/* Gather basic types */
	TGIF_TYPE_GATHER_BOOL,
	TGIF_TYPE_GATHER_INTEGER,
	TGIF_TYPE_GATHER_BYTE,
	TGIF_TYPE_GATHER_POINTER,
	TGIF_TYPE_GATHER_FLOAT,
	TGIF_TYPE_GATHER_STRING,

	/* Gather compound types */
	TGIF_TYPE_GATHER_STRUCT,
	TGIF_TYPE_GATHER_ARRAY,
	TGIF_TYPE_GATHER_VLA,

	/* Gather enumeration types */
	TGIF_TYPE_GATHER_ENUM,

	/* Dynamic basic types */
	TGIF_TYPE_DYNAMIC_NULL,
	TGIF_TYPE_DYNAMIC_BOOL,
	TGIF_TYPE_DYNAMIC_INTEGER,
	TGIF_TYPE_DYNAMIC_BYTE,
	TGIF_TYPE_DYNAMIC_POINTER,
	TGIF_TYPE_DYNAMIC_FLOAT,
	TGIF_TYPE_DYNAMIC_STRING,

	/* Dynamic compound types */
	TGIF_TYPE_DYNAMIC_STRUCT,
	TGIF_TYPE_DYNAMIC_STRUCT_VISITOR,
	TGIF_TYPE_DYNAMIC_VLA,
	TGIF_TYPE_DYNAMIC_VLA_VISITOR,
};

enum tgif_attr_type {
	TGIF_ATTR_TYPE_NULL,
	TGIF_ATTR_TYPE_BOOL,
	TGIF_ATTR_TYPE_U8,
	TGIF_ATTR_TYPE_U16,
	TGIF_ATTR_TYPE_U32,
	TGIF_ATTR_TYPE_U64,
	TGIF_ATTR_TYPE_S8,
	TGIF_ATTR_TYPE_S16,
	TGIF_ATTR_TYPE_S32,
	TGIF_ATTR_TYPE_S64,
	TGIF_ATTR_TYPE_FLOAT_BINARY16,
	TGIF_ATTR_TYPE_FLOAT_BINARY32,
	TGIF_ATTR_TYPE_FLOAT_BINARY64,
	TGIF_ATTR_TYPE_FLOAT_BINARY128,
	TGIF_ATTR_TYPE_STRING,
};

enum tgif_loglevel {
	TGIF_LOGLEVEL_EMERG = 0,
	TGIF_LOGLEVEL_ALERT = 1,
	TGIF_LOGLEVEL_CRIT = 2,
	TGIF_LOGLEVEL_ERR = 3,
	TGIF_LOGLEVEL_WARNING = 4,
	TGIF_LOGLEVEL_NOTICE = 5,
	TGIF_LOGLEVEL_INFO = 6,
	TGIF_LOGLEVEL_DEBUG = 7,
};

enum tgif_visitor_status {
	TGIF_VISITOR_STATUS_OK = 0,
	TGIF_VISITOR_STATUS_ERROR = -1,
};

enum tgif_error {
	TGIF_ERROR_OK = 0,
	TGIF_ERROR_INVAL = 1,
	TGIF_ERROR_EXIST = 2,
	TGIF_ERROR_NOMEM = 3,
	TGIF_ERROR_NOENT = 4,
	TGIF_ERROR_EXITING = 5,
};

enum tgif_type_label_byte_order {
	TGIF_TYPE_BYTE_ORDER_LE = 0,
	TGIF_TYPE_BYTE_ORDER_BE = 1,
};

#if (TGIF_BYTE_ORDER == TGIF_LITTLE_ENDIAN)
# define TGIF_TYPE_BYTE_ORDER_HOST		TGIF_TYPE_BYTE_ORDER_LE
#else
# define TGIF_TYPE_BYTE_ORDER_HOST		TGIF_TYPE_BYTE_ORDER_BE
#endif

#if (TGIF_FLOAT_WORD_ORDER == TGIF_LITTLE_ENDIAN)
# define TGIF_TYPE_FLOAT_WORD_ORDER_HOST	TGIF_TYPE_BYTE_ORDER_LE
#else
# define TGIF_TYPE_FLOAT_WORD_ORDER_HOST	TGIF_TYPE_BYTE_ORDER_BE
#endif

enum tgif_type_gather_access_mode {
	TGIF_TYPE_GATHER_ACCESS_DIRECT,
	TGIF_TYPE_GATHER_ACCESS_POINTER,	/* Pointer dereference */
};

typedef enum tgif_visitor_status (*tgif_visitor_func)(
		const struct tgif_tracer_visitor_ctx *tracer_ctx,
		void *app_ctx);
typedef enum tgif_visitor_status (*tgif_dynamic_struct_visitor_func)(
		const struct tgif_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
		void *app_ctx);

union tgif_integer_value {
	uint8_t tgif_u8;
	uint16_t tgif_u16;
	uint32_t tgif_u32;
	uint64_t tgif_u64;
	int8_t tgif_s8;
	int16_t tgif_s16;
	int32_t tgif_s32;
	int64_t tgif_s64;
	uintptr_t tgif_uptr;
} TGIF_PACKED;

union tgif_bool_value {
	uint8_t tgif_bool8;
	uint16_t tgif_bool16;
	uint32_t tgif_bool32;
	uint64_t tgif_bool64;
} TGIF_PACKED;

union tgif_float_value {
#if __HAVE_FLOAT16
	_Float16 tgif_float_binary16;
#endif
#if __HAVE_FLOAT32
	_Float32 tgif_float_binary32;
#endif
#if __HAVE_FLOAT64
	_Float64 tgif_float_binary64;
#endif
#if __HAVE_FLOAT128
	_Float128 tgif_float_binary128;
#endif
} TGIF_PACKED;

struct tgif_type_raw_string {
	const void *p;			/* pointer to string */
	uint8_t unit_size;		/* 1, 2, or 4 bytes */
	uint8_t byte_order;		/* enum tgif_type_label_byte_order */
} TGIF_PACKED;

struct tgif_attr_value {
	uint32_t type;	/* enum tgif_attr_type */
	union {
		uint8_t bool_value;
		struct tgif_type_raw_string string_value;
		union tgif_integer_value integer_value;
		union tgif_float_value float_value;
	} TGIF_PACKED u;
};

/* User attributes. */
struct tgif_attr {
	const struct tgif_type_raw_string key;
	const struct tgif_attr_value value;
} TGIF_PACKED;

/* Type descriptions */
struct tgif_type_null {
	const struct tgif_attr *attr;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_type_bool {
	const struct tgif_attr *attr;
	uint32_t nr_attr;
	uint16_t bool_size;		/* bytes */
	uint16_t len_bits;		/* bits. 0 for (bool_size * CHAR_BITS) */
	uint8_t byte_order;		/* enum tgif_type_label_byte_order */
} TGIF_PACKED;

struct tgif_type_byte {
	const struct tgif_attr *attr;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_type_string {
	const struct tgif_attr *attr;
	uint32_t nr_attr;
	uint8_t unit_size;		/* 1, 2, or 4 bytes */
	uint8_t byte_order;		/* enum tgif_type_label_byte_order */
} TGIF_PACKED;

struct tgif_type_integer {
	const struct tgif_attr *attr;
	uint32_t nr_attr;
	uint16_t integer_size;		/* bytes */
	uint16_t len_bits;		/* bits. 0 for (integer_size * CHAR_BITS) */
	uint8_t signedness;		/* true/false */
	uint8_t byte_order;		/* enum tgif_type_label_byte_order */
} TGIF_PACKED;

struct tgif_type_float {
	const struct tgif_attr *attr;
	uint32_t nr_attr;
	uint16_t float_size;		/* bytes */
	uint8_t byte_order;		/* enum tgif_type_label_byte_order */
} TGIF_PACKED;

struct tgif_enum_mapping {
	int64_t range_begin;
	int64_t range_end;
	struct tgif_type_raw_string label;
} TGIF_PACKED;

struct tgif_enum_mappings {
	const struct tgif_enum_mapping *mappings;
	const struct tgif_attr *attr;
	uint32_t nr_mappings;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_enum_bitmap_mapping {
	uint64_t range_begin;
	uint64_t range_end;
	struct tgif_type_raw_string label;
} TGIF_PACKED;

struct tgif_enum_bitmap_mappings {
	const struct tgif_enum_bitmap_mapping *mappings;
	const struct tgif_attr *attr;
	uint32_t nr_mappings;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_type_struct {
	const struct tgif_event_field *fields;
	const struct tgif_attr *attr;
	uint32_t nr_fields;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_type_array {
	const struct tgif_type *elem_type;
	const struct tgif_attr *attr;
	uint32_t length;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_type_vla {
	const struct tgif_type *elem_type;
	const struct tgif_attr *attr;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_type_vla_visitor {
	const struct tgif_type *elem_type;
	tgif_visitor_func visitor;
	const struct tgif_attr *attr;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_type_enum {
	const struct tgif_enum_mappings *mappings;
	const struct tgif_type *elem_type;
} TGIF_PACKED;

struct tgif_type_enum_bitmap {
	const struct tgif_enum_bitmap_mappings *mappings;
	const struct tgif_type *elem_type;
} TGIF_PACKED;

struct tgif_type_gather_bool {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum tgif_type_gather_access_mode */
	struct tgif_type_bool type;
	uint16_t offset_bits;	/* bits */
} TGIF_PACKED;

struct tgif_type_gather_byte {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum tgif_type_gather_access_mode */
	struct tgif_type_byte type;
} TGIF_PACKED;

struct tgif_type_gather_integer {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum tgif_type_gather_access_mode */
	struct tgif_type_integer type;
	uint16_t offset_bits;	/* bits */
} TGIF_PACKED;

struct tgif_type_gather_float {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum tgif_type_gather_access_mode */
	struct tgif_type_float type;
} TGIF_PACKED;

struct tgif_type_gather_string {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum tgif_type_gather_access_mode */
	struct tgif_type_string type;
} TGIF_PACKED;

struct tgif_type_gather_enum {
	const struct tgif_enum_mappings *mappings;
	const struct tgif_type *elem_type;
} TGIF_PACKED;

struct tgif_type_gather_struct {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum tgif_type_gather_access_mode */
	const struct tgif_type_struct *type;
	uint32_t size;		/* bytes */
} TGIF_PACKED;

struct tgif_type_gather_array {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum tgif_type_gather_access_mode */
	struct tgif_type_array type;
} TGIF_PACKED;

struct tgif_type_gather_vla {
	const struct tgif_type *length_type;	/* tgif_length() */

	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum tgif_type_gather_access_mode */
	struct tgif_type_vla type;
} TGIF_PACKED;

struct tgif_type_gather {
	union {
		struct tgif_type_gather_bool tgif_bool;
		struct tgif_type_gather_byte tgif_byte;
		struct tgif_type_gather_integer tgif_integer;
		struct tgif_type_gather_float tgif_float;
		struct tgif_type_gather_string tgif_string;
		struct tgif_type_gather_enum tgif_enum;
		struct tgif_type_gather_array tgif_array;
		struct tgif_type_gather_vla tgif_vla;
		struct tgif_type_gather_struct tgif_struct;
	} TGIF_PACKED u;
} TGIF_PACKED;

struct tgif_type {
	uint32_t type;	/* enum tgif_type_label */
	union {
		/* Stack-copy basic types */
		struct tgif_type_null tgif_null;
		struct tgif_type_bool tgif_bool;
		struct tgif_type_byte tgif_byte;
		struct tgif_type_string tgif_string;
		struct tgif_type_integer tgif_integer;
		struct tgif_type_float tgif_float;

		/* Stack-copy compound types */
		struct tgif_type_array tgif_array;
		struct tgif_type_vla tgif_vla;
		struct tgif_type_vla_visitor tgif_vla_visitor;
		const struct tgif_type_struct *tgif_struct;

		/* Stack-copy enumeration types */
		struct tgif_type_enum tgif_enum;
		struct tgif_type_enum_bitmap tgif_enum_bitmap;

		/* Gather types */
		struct tgif_type_gather tgif_gather;
	} TGIF_PACKED u;
} TGIF_PACKED;

struct tgif_event_field {
	const char *field_name;
	struct tgif_type tgif_type;
} TGIF_PACKED;

enum tgif_event_flags {
	TGIF_EVENT_FLAG_VARIADIC = (1 << 0),
};

struct tgif_callback {
	union {
		void (*call)(const struct tgif_event_description *desc,
			const struct tgif_arg_vec *tgif_arg_vec,
			void *priv);
		void (*call_variadic)(const struct tgif_event_description *desc,
			const struct tgif_arg_vec *tgif_arg_vec,
			const struct tgif_arg_dynamic_struct *var_struct,
			void *priv);
	} TGIF_PACKED u;
	void *priv;
} TGIF_PACKED;

struct tgif_arg_static {
	/* Stack-copy basic types */
	union tgif_bool_value bool_value;
	uint8_t byte_value;
	uint64_t string_value;	/* const {uint8_t, uint16_t, uint32_t} * */
	union tgif_integer_value integer_value;
	union tgif_float_value float_value;

	/* Stack-copy compound types */
	const struct tgif_arg_vec *tgif_struct;
	const struct tgif_arg_vec *tgif_array;
	const struct tgif_arg_vec *tgif_vla;
	void *tgif_vla_app_visitor_ctx;

	/* Gather basic types */
	const void *tgif_bool_gather_ptr;
	const void *tgif_byte_gather_ptr;
	const void *tgif_integer_gather_ptr;
	const void *tgif_float_gather_ptr;
	const void *tgif_string_gather_ptr;

	/* Gather compound types */
	const void *tgif_array_gather_ptr;
	const void *tgif_struct_gather_ptr;
	struct {
		const void *ptr;
		const void *length_ptr;
	} TGIF_PACKED tgif_vla_gather;
} TGIF_PACKED;

struct tgif_arg_dynamic_vla {
	const struct tgif_arg *sav;
	const struct tgif_attr *attr;
	uint32_t len;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_arg_dynamic_struct {
	const struct tgif_arg_dynamic_field *fields;
	const struct tgif_attr *attr;
	uint32_t len;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_dynamic_struct_visitor {
	void *app_ctx;
	tgif_dynamic_struct_visitor_func visitor;
	const struct tgif_attr *attr;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_dynamic_vla_visitor {
	void *app_ctx;
	tgif_visitor_func visitor;
	const struct tgif_attr *attr;
	uint32_t nr_attr;
} TGIF_PACKED;

struct tgif_arg_dynamic {
	/* Dynamic basic types */
	struct tgif_type_null tgif_null;
	struct {
		struct tgif_type_bool type;
		union tgif_bool_value value;
	} TGIF_PACKED tgif_bool;
	struct {
		struct tgif_type_byte type;
		uint8_t value;
	} TGIF_PACKED tgif_byte;
	struct {
		struct tgif_type_string type;
		uint64_t value;	/* const char * */
	} TGIF_PACKED tgif_string;
	struct {
		struct tgif_type_integer type;
		union tgif_integer_value value;
	} TGIF_PACKED tgif_integer;
	struct {
		struct tgif_type_float type;
		union tgif_float_value value;
	} TGIF_PACKED tgif_float;

	/* Dynamic compound types */
	const struct tgif_arg_dynamic_struct *tgif_dynamic_struct;
	const struct tgif_arg_dynamic_vla *tgif_dynamic_vla;

	struct tgif_dynamic_struct_visitor tgif_dynamic_struct_visitor;
	struct tgif_dynamic_vla_visitor tgif_dynamic_vla_visitor;
} TGIF_PACKED;

struct tgif_arg {
	uint32_t type;	/* enum tgif_type_label */
	union {
		struct tgif_arg_static tgif_static;
		struct tgif_arg_dynamic tgif_dynamic;
	} TGIF_PACKED u;
} TGIF_PACKED;

struct tgif_arg_vec {
	const struct tgif_arg *sav;
	uint32_t len;
} TGIF_PACKED;

struct tgif_arg_dynamic_field {
	const char *field_name;
	const struct tgif_arg elem;
} TGIF_PACKED;

/* The visitor pattern is a double-dispatch visitor. */
struct tgif_tracer_visitor_ctx {
	enum tgif_visitor_status (*write_elem)(
			const struct tgif_tracer_visitor_ctx *tracer_ctx,
			const struct tgif_arg *elem);
	void *priv;		/* Private tracer context. */
} TGIF_PACKED;

struct tgif_tracer_dynamic_struct_visitor_ctx {
	enum tgif_visitor_status (*write_field)(
			const struct tgif_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
			const struct tgif_arg_dynamic_field *dynamic_field);
	void *priv;		/* Private tracer context. */
} TGIF_PACKED;

struct tgif_event_description {
	uintptr_t *enabled;
	const char *provider_name;
	const char *event_name;
	const struct tgif_event_field *fields;
	const struct tgif_attr *attr;
	const struct tgif_callback *callbacks;
	uint64_t flags;
	uint32_t version;
	uint32_t loglevel;	/* enum tgif_loglevel */
	uint32_t nr_fields;
	uint32_t nr_attr;
	uint32_t nr_callbacks;
} TGIF_PACKED;

/* Event and type attributes */

#define tgif_attr(_key, _value)	\
	{ \
		.key = { \
			.p = (_key), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = TGIF_TYPE_BYTE_ORDER_HOST, \
		}, \
		.value = TGIF_PARAM(_value), \
	}

#define tgif_attr_list(...) \
	TGIF_COMPOUND_LITERAL(const struct tgif_attr, __VA_ARGS__)

#define tgif_attr_null(_val)		{ .type = TGIF_ATTR_TYPE_NULL }
#define tgif_attr_bool(_val)		{ .type = TGIF_ATTR_TYPE_BOOL, .u = { .bool_value = !!(_val) } }
#define tgif_attr_u8(_val)		{ .type = TGIF_ATTR_TYPE_U8, .u = { .integer_value = { .tgif_u8 = (_val) } } }
#define tgif_attr_u16(_val)		{ .type = TGIF_ATTR_TYPE_U16, .u = { .integer_value = { .tgif_u16 = (_val) } } }
#define tgif_attr_u32(_val)		{ .type = TGIF_ATTR_TYPE_U32, .u = { .integer_value = { .tgif_u32 = (_val) } } }
#define tgif_attr_u64(_val)		{ .type = TGIF_ATTR_TYPE_U64, .u = { .integer_value = { .tgif_u64 = (_val) } } }
#define tgif_attr_s8(_val)		{ .type = TGIF_ATTR_TYPE_S8, .u = { .integer_value = { .tgif_s8 = (_val) } } }
#define tgif_attr_s16(_val)		{ .type = TGIF_ATTR_TYPE_S16, .u = { .integer_value = { .tgif_s16 = (_val) } } }
#define tgif_attr_s32(_val)		{ .type = TGIF_ATTR_TYPE_S32, .u = { .integer_value = { .tgif_s32 = (_val) } } }
#define tgif_attr_s64(_val)		{ .type = TGIF_ATTR_TYPE_S64, .u = { .integer_value = { .tgif_s64 = (_val) } } }
#define tgif_attr_float_binary16(_val)	{ .type = TGIF_ATTR_TYPE_FLOAT_BINARY16, .u = { .float_value = { .tgif_float_binary16 = (_val) } } }
#define tgif_attr_float_binary32(_val)	{ .type = TGIF_ATTR_TYPE_FLOAT_BINARY32, .u = { .float_value = { .tgif_float_binary32 = (_val) } } }
#define tgif_attr_float_binary64(_val)	{ .type = TGIF_ATTR_TYPE_FLOAT_BINARY64, .u = { .float_value = { .tgif_float_binary64 = (_val) } } }
#define tgif_attr_float_binary128(_val)	{ .type = TGIF_ATTR_TYPE_FLOAT_BINARY128, .u = { .float_value = { .tgif_float_binary128 = (_val) } } }

#define _tgif_attr_string(_val, _byte_order, _unit_size) \
	{ \
		.type = TGIF_ATTR_TYPE_STRING, \
		.u = { \
			.string_value = { \
				.p = (const void *) (_val), \
				.unit_size = _unit_size, \
				.byte_order = _byte_order, \
			}, \
		}, \
	}

#define tgif_attr_string(_val)		_tgif_attr_string(_val, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t))
#define tgif_attr_string16(_val)	_tgif_attr_string(_val, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t))
#define tgif_attr_string32(_val)	_tgif_attr_string(_val, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t))

/* Stack-copy enumeration type definitions */

#define tgif_define_enum(_identifier, _mappings, _attr) \
	const struct tgif_enum_mappings _identifier = { \
		.mappings = _mappings, \
		.attr = _attr, \
		.nr_mappings = TGIF_ARRAY_SIZE(TGIF_PARAM(_mappings)), \
		.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
	}

#define tgif_enum_mapping_list(...) \
	TGIF_COMPOUND_LITERAL(const struct tgif_enum_mapping, __VA_ARGS__)

#define tgif_enum_mapping_range(_label, _begin, _end) \
	{ \
		.range_begin = _begin, \
		.range_end = _end, \
		.label = { \
			.p = (_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = TGIF_TYPE_BYTE_ORDER_HOST, \
		}, \
	}

#define tgif_enum_mapping_value(_label, _value) \
	{ \
		.range_begin = _value, \
		.range_end = _value, \
		.label = { \
			.p = (_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = TGIF_TYPE_BYTE_ORDER_HOST, \
		}, \
	}

#define tgif_define_enum_bitmap(_identifier, _mappings, _attr) \
	const struct tgif_enum_bitmap_mappings _identifier = { \
		.mappings = _mappings, \
		.attr = _attr, \
		.nr_mappings = TGIF_ARRAY_SIZE(TGIF_PARAM(_mappings)), \
		.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
	}

#define tgif_enum_bitmap_mapping_list(...) \
	TGIF_COMPOUND_LITERAL(const struct tgif_enum_bitmap_mapping, __VA_ARGS__)

#define tgif_enum_bitmap_mapping_range(_label, _begin, _end) \
	{ \
		.range_begin = _begin, \
		.range_end = _end, \
		.label = { \
			.p = (_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = TGIF_TYPE_BYTE_ORDER_HOST, \
		}, \
	}

#define tgif_enum_bitmap_mapping_value(_label, _value) \
	{ \
		.range_begin = _value, \
		.range_end = _value, \
		.label = { \
			.p = (_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = TGIF_TYPE_BYTE_ORDER_HOST, \
		}, \
	}

/* Stack-copy field and type definitions */

#define tgif_type_null(_attr) \
	{ \
		.type = TGIF_TYPE_NULL, \
		.u = { \
			.tgif_null = { \
				.attr = _attr, \
				.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
			}, \
		}, \
	}

#define tgif_type_bool(_attr) \
	{ \
		.type = TGIF_TYPE_BOOL, \
		.u = { \
			.tgif_bool = { \
				.attr = _attr, \
				.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
				.bool_size = sizeof(uint8_t), \
				.len_bits = 0, \
				.byte_order = TGIF_TYPE_BYTE_ORDER_HOST, \
			}, \
		}, \
	}

#define tgif_type_byte(_attr) \
	{ \
		.type = TGIF_TYPE_BYTE, \
		.u = { \
			.tgif_byte = { \
				.attr = _attr, \
				.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
			}, \
		}, \
	}

#define _tgif_type_string(_type, _byte_order, _unit_size, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.tgif_string = { \
				.attr = _attr, \
				.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
				.unit_size = _unit_size, \
				.byte_order = _byte_order, \
			}, \
		}, \
	}

#define tgif_type_dynamic() \
	{ \
		.type = TGIF_TYPE_DYNAMIC, \
	}

#define _tgif_type_integer(_type, _signedness, _byte_order, _integer_size, _len_bits, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.tgif_integer = { \
				.attr = _attr, \
				.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
				.integer_size = _integer_size, \
				.len_bits = _len_bits, \
				.signedness = _signedness, \
				.byte_order = _byte_order, \
			}, \
		}, \
	}

#define _tgif_type_float(_type, _byte_order, _float_size, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.tgif_float = { \
				.attr = _attr, \
				.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
				.float_size = _float_size, \
				.byte_order = _byte_order, \
			}, \
		}, \
	}

#define _tgif_field(_name, _type) \
	{ \
		.field_name = _name, \
		.tgif_type = _type, \
	}

/* Host endian */
#define tgif_type_u8(_attr)				_tgif_type_integer(TGIF_TYPE_U8, false, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, TGIF_PARAM(_attr))
#define tgif_type_u16(_attr)				_tgif_type_integer(TGIF_TYPE_U16, false, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), 0, TGIF_PARAM(_attr))
#define tgif_type_u32(_attr)				_tgif_type_integer(TGIF_TYPE_U32, false, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), 0, TGIF_PARAM(_attr))
#define tgif_type_u64(_attr)				_tgif_type_integer(TGIF_TYPE_U64, false, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint64_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s8(_attr)				_tgif_type_integer(TGIF_TYPE_S8, true, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s16(_attr)				_tgif_type_integer(TGIF_TYPE_S16, true, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(int16_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s32(_attr)				_tgif_type_integer(TGIF_TYPE_S32, true, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(int32_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s64(_attr)				_tgif_type_integer(TGIF_TYPE_S64, true, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(int64_t), 0, TGIF_PARAM(_attr))
#define tgif_type_pointer(_attr)			_tgif_type_integer(TGIF_TYPE_POINTER, false, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uintptr_t), 0, TGIF_PARAM(_attr))
#define tgif_type_float_binary16(_attr)			_tgif_type_float(TGIF_TYPE_FLOAT_BINARY16, TGIF_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float16), TGIF_PARAM(_attr))
#define tgif_type_float_binary32(_attr)			_tgif_type_float(TGIF_TYPE_FLOAT_BINARY32, TGIF_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float32), TGIF_PARAM(_attr))
#define tgif_type_float_binary64(_attr)			_tgif_type_float(TGIF_TYPE_FLOAT_BINARY64, TGIF_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float64), TGIF_PARAM(_attr))
#define tgif_type_float_binary128(_attr)		_tgif_type_float(TGIF_TYPE_FLOAT_BINARY128, TGIF_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float128), TGIF_PARAM(_attr))
#define tgif_type_string(_attr)				_tgif_type_string(TGIF_TYPE_STRING_UTF8, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), TGIF_PARAM(_attr))
#define tgif_type_string16(_attr) 			_tgif_type_string(TGIF_TYPE_STRING_UTF16, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), TGIF_PARAM(_attr))
#define tgif_type_string32(_attr)	 		_tgif_type_string(TGIF_TYPE_STRING_UTF32, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), TGIF_PARAM(_attr))

#define tgif_field_null(_name, _attr)			_tgif_field(_name, tgif_type_null(TGIF_PARAM(_attr)))
#define tgif_field_bool(_name, _attr)			_tgif_field(_name, tgif_type_bool(TGIF_PARAM(_attr)))
#define tgif_field_u8(_name, _attr)			_tgif_field(_name, tgif_type_u8(TGIF_PARAM(_attr)))
#define tgif_field_u16(_name, _attr)			_tgif_field(_name, tgif_type_u16(TGIF_PARAM(_attr)))
#define tgif_field_u32(_name, _attr)			_tgif_field(_name, tgif_type_u32(TGIF_PARAM(_attr)))
#define tgif_field_u64(_name, _attr)			_tgif_field(_name, tgif_type_u64(TGIF_PARAM(_attr)))
#define tgif_field_s8(_name, _attr)			_tgif_field(_name, tgif_type_s8(TGIF_PARAM(_attr)))
#define tgif_field_s16(_name, _attr)			_tgif_field(_name, tgif_type_s16(TGIF_PARAM(_attr)))
#define tgif_field_s32(_name, _attr)			_tgif_field(_name, tgif_type_s32(TGIF_PARAM(_attr)))
#define tgif_field_s64(_name, _attr)			_tgif_field(_name, tgif_type_s64(TGIF_PARAM(_attr)))
#define tgif_field_byte(_name, _attr)			_tgif_field(_name, tgif_type_byte(TGIF_PARAM(_attr)))
#define tgif_field_pointer(_name, _attr)		_tgif_field(_name, tgif_type_pointer(TGIF_PARAM(_attr)))
#define tgif_field_float_binary16(_name, _attr)		_tgif_field(_name, tgif_type_float_binary16(TGIF_PARAM(_attr)))
#define tgif_field_float_binary32(_name, _attr)		_tgif_field(_name, tgif_type_float_binary32(TGIF_PARAM(_attr)))
#define tgif_field_float_binary64(_name, _attr)		_tgif_field(_name, tgif_type_float_binary64(TGIF_PARAM(_attr)))
#define tgif_field_float_binary128(_name, _attr)	_tgif_field(_name, tgif_type_float_binary128(TGIF_PARAM(_attr)))
#define tgif_field_string(_name, _attr)			_tgif_field(_name, tgif_type_string(TGIF_PARAM(_attr)))
#define tgif_field_string16(_name, _attr)		_tgif_field(_name, tgif_type_string16(TGIF_PARAM(_attr)))
#define tgif_field_string32(_name, _attr)		_tgif_field(_name, tgif_type_string32(TGIF_PARAM(_attr)))
#define tgif_field_dynamic(_name)			_tgif_field(_name, tgif_type_dynamic())

/* Little endian */
#define tgif_type_u16_le(_attr)				_tgif_type_integer(TGIF_TYPE_U16, false, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), 0, TGIF_PARAM(_attr))
#define tgif_type_u32_le(_attr)				_tgif_type_integer(TGIF_TYPE_U32, false, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), 0, TGIF_PARAM(_attr))
#define tgif_type_u64_le(_attr)				_tgif_type_integer(TGIF_TYPE_U64, false, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uint64_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s16_le(_attr)				_tgif_type_integer(TGIF_TYPE_S16, true, TGIF_TYPE_BYTE_ORDER_LE, sizeof(int16_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s32_le(_attr)				_tgif_type_integer(TGIF_TYPE_S32, true, TGIF_TYPE_BYTE_ORDER_LE, sizeof(int32_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s64_le(_attr)				_tgif_type_integer(TGIF_TYPE_S64, true, TGIF_TYPE_BYTE_ORDER_LE, sizeof(int64_t), 0, TGIF_PARAM(_attr))
#define tgif_type_pointer_le(_attr)			_tgif_type_integer(TGIF_TYPE_POINTER, false, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uintptr_t), 0, TGIF_PARAM(_attr))
#define tgif_type_float_binary16_le(_attr)		_tgif_type_float(TGIF_TYPE_FLOAT_BINARY16, TGIF_TYPE_BYTE_ORDER_LE, sizeof(_Float16), TGIF_PARAM(_attr))
#define tgif_type_float_binary32_le(_attr)		_tgif_type_float(TGIF_TYPE_FLOAT_BINARY32, TGIF_TYPE_BYTE_ORDER_LE, sizeof(_Float32), TGIF_PARAM(_attr))
#define tgif_type_float_binary64_le(_attr)		_tgif_type_float(TGIF_TYPE_FLOAT_BINARY64, TGIF_TYPE_BYTE_ORDER_LE, sizeof(_Float64), TGIF_PARAM(_attr))
#define tgif_type_float_binary128_le(_attr)		_tgif_type_float(TGIF_TYPE_FLOAT_BINARY128, TGIF_TYPE_BYTE_ORDER_LE, sizeof(_Float128), TGIF_PARAM(_attr))
#define tgif_type_string16_le(_attr) 			_tgif_type_string(TGIF_TYPE_STRING_UTF16, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), TGIF_PARAM(_attr))
#define tgif_type_string32_le(_attr)		 	_tgif_type_string(TGIF_TYPE_STRING_UTF32, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), TGIF_PARAM(_attr))

#define tgif_field_u16_le(_name, _attr)			_tgif_field(_name, tgif_type_u16_le(TGIF_PARAM(_attr)))
#define tgif_field_u32_le(_name, _attr)			_tgif_field(_name, tgif_type_u32_le(TGIF_PARAM(_attr)))
#define tgif_field_u64_le(_name, _attr)			_tgif_field(_name, tgif_type_u64_le(TGIF_PARAM(_attr)))
#define tgif_field_s16_le(_name, _attr)			_tgif_field(_name, tgif_type_s16_le(TGIF_PARAM(_attr)))
#define tgif_field_s32_le(_name, _attr)			_tgif_field(_name, tgif_type_s32_le(TGIF_PARAM(_attr)))
#define tgif_field_s64_le(_name, _attr)			_tgif_field(_name, tgif_type_s64_le(TGIF_PARAM(_attr)))
#define tgif_field_pointer_le(_name, _attr)		_tgif_field(_name, tgif_type_pointer_le(TGIF_PARAM(_attr)))
#define tgif_field_float_binary16_le(_name, _attr)	_tgif_field(_name, tgif_type_float_binary16_le(TGIF_PARAM(_attr)))
#define tgif_field_float_binary32_le(_name, _attr)	_tgif_field(_name, tgif_type_float_binary32_le(TGIF_PARAM(_attr)))
#define tgif_field_float_binary64_le(_name, _attr)	_tgif_field(_name, tgif_type_float_binary64_le(TGIF_PARAM(_attr)))
#define tgif_field_float_binary128_le(_name, _attr)	_tgif_field(_name, tgif_type_float_binary128_le(TGIF_PARAM(_attr)))
#define tgif_field_string16_le(_name, _attr)		_tgif_field(_name, tgif_type_string16_le(TGIF_PARAM(_attr)))
#define tgif_field_string32_le(_name, _attr)		_tgif_field(_name, tgif_type_string32_le(TGIF_PARAM(_attr)))

/* Big endian */
#define tgif_type_u16_be(_attr)				_tgif_type_integer(TGIF_TYPE_U16, false, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), 0, TGIF_PARAM(_attr))
#define tgif_type_u32_be(_attr)				_tgif_type_integer(TGIF_TYPE_U32, false, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), 0, TGIF_PARAM(_attr))
#define tgif_type_u64_be(_attr)				_tgif_type_integer(TGIF_TYPE_U64, false, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uint64_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s16_be(_attr)				_tgif_type_integer(TGIF_TYPE_S16, false, TGIF_TYPE_BYTE_ORDER_BE, sizeof(int16_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s32_be(_attr)				_tgif_type_integer(TGIF_TYPE_S32, false, TGIF_TYPE_BYTE_ORDER_BE, sizeof(int32_t), 0, TGIF_PARAM(_attr))
#define tgif_type_s64_be(_attr)				_tgif_type_integer(TGIF_TYPE_S64, false, TGIF_TYPE_BYTE_ORDER_BE, sizeof(int64_t), 0, TGIF_PARAM(_attr))
#define tgif_type_pointer_be(_attr)			_tgif_type_integer(TGIF_TYPE_POINTER, false, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uintptr_t), 0, TGIF_PARAM(_attr))
#define tgif_type_float_binary16_be(_attr)		_tgif_type_float(TGIF_TYPE_FLOAT_BINARY16, TGIF_TYPE_BYTE_ORDER_BE, sizeof(_Float16), TGIF_PARAM(_attr))
#define tgif_type_float_binary32_be(_attr)		_tgif_type_float(TGIF_TYPE_FLOAT_BINARY32, TGIF_TYPE_BYTE_ORDER_BE, sizeof(_Float32), TGIF_PARAM(_attr))
#define tgif_type_float_binary64_be(_attr)		_tgif_type_float(TGIF_TYPE_FLOAT_BINARY64, TGIF_TYPE_BYTE_ORDER_BE, sizeof(_Float64), TGIF_PARAM(_attr))
#define tgif_type_float_binary128_be(_attr)		_tgif_type_float(TGIF_TYPE_FLOAT_BINARY128, TGIF_TYPE_BYTE_ORDER_BE, sizeof(_Float128), TGIF_PARAM(_attr))
#define tgif_type_string16_be(_attr) 			_tgif_type_string(TGIF_TYPE_STRING_UTF16, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), TGIF_PARAM(_attr))
#define tgif_type_string32_be(_attr)		 	_tgif_type_string(TGIF_TYPE_STRING_UTF32, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), TGIF_PARAM(_attr))

#define tgif_field_u16_be(_name, _attr)			_tgif_field(_name, tgif_type_u16_be(TGIF_PARAM(_attr)))
#define tgif_field_u32_be(_name, _attr)			_tgif_field(_name, tgif_type_u32_be(TGIF_PARAM(_attr)))
#define tgif_field_u64_be(_name, _attr)			_tgif_field(_name, tgif_type_u64_be(TGIF_PARAM(_attr)))
#define tgif_field_s16_be(_name, _attr)			_tgif_field(_name, tgif_type_s16_be(TGIF_PARAM(_attr)))
#define tgif_field_s32_be(_name, _attr)			_tgif_field(_name, tgif_type_s32_be(TGIF_PARAM(_attr)))
#define tgif_field_s64_be(_name, _attr)			_tgif_field(_name, tgif_type_s64_be(TGIF_PARAM(_attr)))
#define tgif_field_pointer_be(_name, _attr)		_tgif_field(_name, tgif_type_pointer_be(TGIF_PARAM(_attr)))
#define tgif_field_float_binary16_be(_name, _attr)	_tgif_field(_name, tgif_type_float_binary16_be(TGIF_PARAM(_attr)))
#define tgif_field_float_binary32_be(_name, _attr)	_tgif_field(_name, tgif_type_float_binary32_be(TGIF_PARAM(_attr)))
#define tgif_field_float_binary64_be(_name, _attr)	_tgif_field(_name, tgif_type_float_binary64_be(TGIF_PARAM(_attr)))
#define tgif_field_float_binary128_be(_name, _attr)	_tgif_field(_name, tgif_type_float_binary128_be(TGIF_PARAM(_attr)))
#define tgif_field_string16_be(_name, _attr)		_tgif_field(_name, tgif_type_string16_be(TGIF_PARAM(_attr)))
#define tgif_field_string32_be(_name, _attr)		_tgif_field(_name, tgif_type_string32_be(TGIF_PARAM(_attr)))

#define tgif_type_enum(_mappings, _elem_type) \
	{ \
		.type = TGIF_TYPE_ENUM, \
		.u = { \
			.tgif_enum = { \
				.mappings = _mappings, \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define tgif_field_enum(_name, _mappings, _elem_type) \
	_tgif_field(_name, tgif_type_enum(TGIF_PARAM(_mappings), TGIF_PARAM(_elem_type)))

#define tgif_type_enum_bitmap(_mappings, _elem_type) \
	{ \
		.type = TGIF_TYPE_ENUM_BITMAP, \
		.u = { \
			.tgif_enum_bitmap = { \
				.mappings = _mappings, \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define tgif_field_enum_bitmap(_name, _mappings, _elem_type) \
	_tgif_field(_name, tgif_type_enum_bitmap(TGIF_PARAM(_mappings), TGIF_PARAM(_elem_type)))

#define tgif_type_struct(_struct) \
	{ \
		.type = TGIF_TYPE_STRUCT, \
		.u = { \
			.tgif_struct = _struct, \
		}, \
	}
#define tgif_field_struct(_name, _struct) \
	_tgif_field(_name, tgif_type_struct(TGIF_PARAM(_struct)))

#define _tgif_type_struct_define(_fields, _attr) \
	{ \
		.fields = _fields, \
		.attr = _attr, \
		.nr_fields = TGIF_ARRAY_SIZE(TGIF_PARAM(_fields)), \
		.nr_attr  = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
	}

#define tgif_define_struct(_identifier, _fields, _attr) \
	const struct tgif_type_struct _identifier = _tgif_type_struct_define(TGIF_PARAM(_fields), TGIF_PARAM(_attr))

#define tgif_struct_literal(_fields, _attr) \
	TGIF_COMPOUND_LITERAL(const struct tgif_type_struct, \
		_tgif_type_struct_define(TGIF_PARAM(_fields), TGIF_PARAM(_attr)))

#define tgif_type_array(_elem_type, _length, _attr) \
	{ \
		.type = TGIF_TYPE_ARRAY, \
		.u = { \
			.tgif_array = { \
				.elem_type = _elem_type, \
				.attr = _attr, \
				.length = _length, \
				.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
			}, \
		}, \
	}
#define tgif_field_array(_name, _elem_type, _length, _attr) \
	_tgif_field(_name, tgif_type_array(TGIF_PARAM(_elem_type), _length, TGIF_PARAM(_attr)))

#define tgif_type_vla(_elem_type, _attr) \
	{ \
		.type = TGIF_TYPE_VLA, \
		.u = { \
			.tgif_vla = { \
				.elem_type = _elem_type, \
				.attr = _attr, \
				.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
			}, \
		}, \
	}
#define tgif_field_vla(_name, _elem_type, _attr) \
	_tgif_field(_name, tgif_type_vla(TGIF_PARAM(_elem_type), TGIF_PARAM(_attr)))

#define tgif_type_vla_visitor(_elem_type, _visitor, _attr) \
	{ \
		.type = TGIF_TYPE_VLA_VISITOR, \
		.u = { \
			.tgif_vla_visitor = { \
				.elem_type = TGIF_PARAM(_elem_type), \
				.visitor = _visitor, \
				.attr = _attr, \
				.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
			}, \
		}, \
	}
#define tgif_field_vla_visitor(_name, _elem_type, _visitor, _attr) \
	_tgif_field(_name, tgif_type_vla_visitor(TGIF_PARAM(_elem_type), _visitor, TGIF_PARAM(_attr)))

/* Gather field and type definitions */

#define tgif_type_gather_byte(_offset, _access_mode, _attr) \
	{ \
		.type = TGIF_TYPE_GATHER_BYTE, \
		.u = { \
			.tgif_gather = { \
				.u = { \
					.tgif_byte = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define tgif_field_gather_byte(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_byte(_offset, _access_mode, TGIF_PARAM(_attr)))

#define _tgif_type_gather_bool(_byte_order, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	{ \
		.type = TGIF_TYPE_GATHER_BOOL, \
		.u = { \
			.tgif_gather = { \
				.u = { \
					.tgif_bool = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
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
#define tgif_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_type_gather_bool(TGIF_TYPE_BYTE_ORDER_HOST, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr)
#define tgif_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_type_gather_bool(TGIF_TYPE_BYTE_ORDER_LE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr)
#define tgif_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_type_gather_bool(TGIF_TYPE_BYTE_ORDER_BE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr)

#define tgif_field_gather_bool(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr)))
#define tgif_field_gather_bool_le(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr)))
#define tgif_field_gather_bool_be(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr)))

#define _tgif_type_gather_integer(_type, _signedness, _byte_order, _offset, \
		_integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.tgif_gather = { \
				.u = { \
					.tgif_integer = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
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

#define tgif_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_type_gather_integer(TGIF_TYPE_GATHER_INTEGER, false,  TGIF_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr))
#define tgif_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_type_gather_integer(TGIF_TYPE_GATHER_INTEGER, true, TGIF_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr))

#define tgif_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_type_gather_integer(TGIF_TYPE_GATHER_INTEGER, false,  TGIF_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr))
#define tgif_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_type_gather_integer(TGIF_TYPE_GATHER_INTEGER, true, TGIF_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr))

#define tgif_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_type_gather_integer(TGIF_TYPE_GATHER_INTEGER, false,  TGIF_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr))
#define tgif_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_type_gather_integer(TGIF_TYPE_GATHER_INTEGER, true, TGIF_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr))

#define tgif_field_gather_unsigned_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr)))
#define tgif_field_gather_signed_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr)))

#define tgif_field_gather_unsigned_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr)))
#define tgif_field_gather_signed_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr)))

#define tgif_field_gather_unsigned_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr)))
#define tgif_field_gather_signed_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, TGIF_PARAM(_attr)))

#define tgif_type_gather_pointer(_offset, _access_mode, _attr) \
	_tgif_type_gather_integer(TGIF_TYPE_GATHER_POINTER, false, TGIF_TYPE_BYTE_ORDER_HOST, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, TGIF_PARAM(_attr))
#define tgif_field_gather_pointer(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_pointer(_offset, _access_mode, TGIF_PARAM(_attr)))

#define tgif_type_gather_pointer_le(_offset, _access_mode, _attr) \
	_tgif_type_gather_integer(TGIF_TYPE_GATHER_POINTER, false, TGIF_TYPE_BYTE_ORDER_LE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, TGIF_PARAM(_attr))
#define tgif_field_gather_pointer_le(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_pointer_le(_offset, _access_mode, TGIF_PARAM(_attr)))

#define tgif_type_gather_pointer_be(_offset, _access_mode, _attr) \
	_tgif_type_gather_integer(TGIF_TYPE_GATHER_POINTER, false, TGIF_TYPE_BYTE_ORDER_BE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, TGIF_PARAM(_attr))
#define tgif_field_gather_pointer_be(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_pointer_be(_offset, _access_mode, TGIF_PARAM(_attr)))

#define _tgif_type_gather_float(_byte_order, _offset, _float_size, _access_mode, _attr) \
	{ \
		.type = TGIF_TYPE_GATHER_FLOAT, \
		.u = { \
			.tgif_gather = { \
				.u = { \
					.tgif_float = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
							.float_size = _float_size, \
							.byte_order = _byte_order, \
						}, \
					}, \
				}, \
			}, \
		}, \
	}

#define tgif_type_gather_float(_offset, _float_size, _access_mode, _attr) \
	_tgif_type_gather_float(TGIF_TYPE_FLOAT_WORD_ORDER_HOST, _offset, _float_size, _access_mode, _attr)
#define tgif_type_gather_float_le(_offset, _float_size, _access_mode, _attr) \
	_tgif_type_gather_float(TGIF_TYPE_BYTE_ORDER_LE, _offset, _float_size, _access_mode, _attr)
#define tgif_type_gather_float_be(_offset, _float_size, _access_mode, _attr) \
	_tgif_type_gather_float(TGIF_TYPE_BYTE_ORDER_BE, _offset, _float_size, _access_mode, _attr)

#define tgif_field_gather_float(_name, _offset, _float_size, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_float(_offset, _float_size, _access_mode, _attr))
#define tgif_field_gather_float_le(_name, _offset, _float_size, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_float_le(_offset, _float_size, _access_mode, _attr))
#define tgif_field_gather_float_be(_name, _offset, _float_size, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_float_be(_offset, _float_size, _access_mode, _attr))

#define _tgif_type_gather_string(_offset, _byte_order, _unit_size, _access_mode, _attr) \
	{ \
		.type = TGIF_TYPE_GATHER_STRING, \
		.u = { \
			.tgif_gather = { \
				.u = { \
					.tgif_string = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = _attr, \
							.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
							.unit_size = _unit_size, \
							.byte_order = _byte_order, \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define tgif_type_gather_string(_offset, _access_mode, _attr) \
	_tgif_type_gather_string(_offset, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), _access_mode, TGIF_PARAM(_attr))
#define tgif_field_gather_string(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_string(_offset, _access_mode, TGIF_PARAM(_attr)))

#define tgif_type_gather_string16(_offset, _access_mode, _attr) \
	_tgif_type_gather_string(_offset, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), _access_mode, TGIF_PARAM(_attr))
#define tgif_type_gather_string16_le(_offset, _access_mode, _attr) \
	_tgif_type_gather_string(_offset, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), _access_mode, TGIF_PARAM(_attr))
#define tgif_type_gather_string16_be(_offset, _access_mode, _attr) \
	_tgif_type_gather_string(_offset, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), _access_mode, TGIF_PARAM(_attr))

#define tgif_field_gather_string16(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_string16(_offset, _access_mode, TGIF_PARAM(_attr)))
#define tgif_field_gather_string16_le(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_string16_le(_offset, _access_mode, TGIF_PARAM(_attr)))
#define tgif_field_gather_string16_be(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_string16_be(_offset, _access_mode, TGIF_PARAM(_attr)))

#define tgif_type_gather_string32(_offset, _access_mode, _attr) \
	_tgif_type_gather_string(_offset, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), _access_mode, TGIF_PARAM(_attr))
#define tgif_type_gather_string32_le(_offset, _access_mode, _attr) \
	_tgif_type_gather_string(_offset, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), _access_mode, TGIF_PARAM(_attr))
#define tgif_type_gather_string32_be(_offset, _access_mode, _attr) \
	_tgif_type_gather_string(_offset, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), _access_mode, TGIF_PARAM(_attr))

#define tgif_field_gather_string32(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_string32(_offset, _access_mode, TGIF_PARAM(_attr)))
#define tgif_field_gather_string32_le(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_string32_le(_offset, _access_mode, TGIF_PARAM(_attr)))
#define tgif_field_gather_string32_be(_name, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_string32_be(_offset, _access_mode, TGIF_PARAM(_attr)))

#define tgif_type_gather_enum(_mappings, _elem_type) \
	{ \
		.type = TGIF_TYPE_GATHER_ENUM, \
		.u = { \
			.tgif_enum = { \
				.mappings = _mappings, \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define tgif_field_gather_enum(_name, _mappings, _elem_type) \
	_tgif_field(_name, tgif_type_gather_enum(TGIF_PARAM(_mappings), TGIF_PARAM(_elem_type)))

#define tgif_type_gather_struct(_struct_gather, _offset, _size, _access_mode) \
	{ \
		.type = TGIF_TYPE_GATHER_STRUCT, \
		.u = { \
			.tgif_gather = { \
				.u = { \
					.tgif_struct = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = _struct_gather, \
						.size = _size, \
					}, \
				}, \
			}, \
		}, \
	}
#define tgif_field_gather_struct(_name, _struct_gather, _offset, _size, _access_mode) \
	_tgif_field(_name, tgif_type_gather_struct(TGIF_PARAM(_struct_gather), _offset, _size, _access_mode))

#define tgif_type_gather_array(_elem_type_gather, _length, _offset, _access_mode, _attr) \
	{ \
		.type = TGIF_TYPE_GATHER_ARRAY, \
		.u = { \
			.tgif_gather = { \
				.u = { \
					.tgif_array = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.elem_type = _elem_type_gather, \
							.attr = _attr, \
							.length = _length, \
							.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define tgif_field_gather_array(_name, _elem_type, _length, _offset, _access_mode, _attr) \
	_tgif_field(_name, tgif_type_gather_array(TGIF_PARAM(_elem_type), _length, _offset, _access_mode, TGIF_PARAM(_attr)))

#define tgif_type_gather_vla(_elem_type_gather, _offset, _access_mode, _length_type_gather, _attr) \
	{ \
		.type = TGIF_TYPE_GATHER_VLA, \
		.u = { \
			.tgif_gather = { \
				.u = { \
					.tgif_vla = { \
						.length_type = _length_type_gather, \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.elem_type = _elem_type_gather, \
							.attr = _attr, \
							.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define tgif_field_gather_vla(_name, _elem_type_gather, _offset, _access_mode, _length_type_gather, _attr) \
	_tgif_field(_name, tgif_type_gather_vla(TGIF_PARAM(_elem_type_gather), _offset, _access_mode, TGIF_PARAM(_length_type_gather), TGIF_PARAM(_attr)))

#define tgif_elem(...) \
	TGIF_COMPOUND_LITERAL(const struct tgif_type, __VA_ARGS__)

#define tgif_length(...) \
	TGIF_COMPOUND_LITERAL(const struct tgif_type, __VA_ARGS__)

#define tgif_field_list(...) \
	TGIF_COMPOUND_LITERAL(const struct tgif_event_field, __VA_ARGS__)

/* Stack-copy field arguments */

#define tgif_arg_null(_val)		{ .type = TGIF_TYPE_NULL }
#define tgif_arg_bool(_val)		{ .type = TGIF_TYPE_BOOL, .u = { .tgif_static = { .bool_value = { .tgif_bool8 = !!(_val) } } } }
#define tgif_arg_byte(_val)		{ .type = TGIF_TYPE_BYTE, .u = { .tgif_static = { .byte_value = (_val) } } }
#define tgif_arg_string(_val)		{ .type = TGIF_TYPE_STRING_UTF8, .u = { .tgif_static = { .string_value = (uintptr_t) (_val) } } }
#define tgif_arg_string16(_val)		{ .type = TGIF_TYPE_STRING_UTF16, .u = { .tgif_static = { .string_value = (uintptr_t) (_val) } } }
#define tgif_arg_string32(_val)		{ .type = TGIF_TYPE_STRING_UTF32, .u = { .tgif_static = { .string_value = (uintptr_t) (_val) } } }

#define tgif_arg_u8(_val)		{ .type = TGIF_TYPE_U8, .u = { .tgif_static = {  .integer_value = { .tgif_u8 = (_val) } } } }
#define tgif_arg_u16(_val)		{ .type = TGIF_TYPE_U16, .u = { .tgif_static = { .integer_value = { .tgif_u16 = (_val) } } } }
#define tgif_arg_u32(_val)		{ .type = TGIF_TYPE_U32, .u = { .tgif_static = { .integer_value = { .tgif_u32 = (_val) } } } }
#define tgif_arg_u64(_val)		{ .type = TGIF_TYPE_U64, .u = { .tgif_static = { .integer_value = { .tgif_u64 = (_val) } } } }
#define tgif_arg_s8(_val)		{ .type = TGIF_TYPE_S8, .u = { .tgif_static = { .integer_value = { .tgif_s8 = (_val) } } } }
#define tgif_arg_s16(_val)		{ .type = TGIF_TYPE_S16, .u = { .tgif_static = { .integer_value = { .tgif_s16 = (_val) } } } }
#define tgif_arg_s32(_val)		{ .type = TGIF_TYPE_S32, .u = { .tgif_static = { .integer_value = { .tgif_s32 = (_val) } } } }
#define tgif_arg_s64(_val)		{ .type = TGIF_TYPE_S64, .u = { .tgif_static = { .integer_value = { .tgif_s64 = (_val) } } } }
#define tgif_arg_pointer(_val)		{ .type = TGIF_TYPE_POINTER, .u = { .tgif_static = { .integer_value = { .tgif_uptr = (uintptr_t) (_val) } } } }
#define tgif_arg_float_binary16(_val)	{ .type = TGIF_TYPE_FLOAT_BINARY16, .u = { .tgif_static = { .float_value = { .tgif_float_binary16 = (_val) } } } }
#define tgif_arg_float_binary32(_val)	{ .type = TGIF_TYPE_FLOAT_BINARY32, .u = { .tgif_static = { .float_value = { .tgif_float_binary32 = (_val) } } } }
#define tgif_arg_float_binary64(_val)	{ .type = TGIF_TYPE_FLOAT_BINARY64, .u = { .tgif_static = { .float_value = { .tgif_float_binary64 = (_val) } } } }
#define tgif_arg_float_binary128(_val)	{ .type = TGIF_TYPE_FLOAT_BINARY128, .u = { .tgif_static = { .float_value = { .tgif_float_binary128 = (_val) } } } }

#define tgif_arg_struct(_tgif_type)	{ .type = TGIF_TYPE_STRUCT, .u = { .tgif_static = { .tgif_struct = (_tgif_type) } } }
#define tgif_arg_array(_tgif_type)	{ .type = TGIF_TYPE_ARRAY, .u = { .tgif_static = { .tgif_array = (_tgif_type) } } }
#define tgif_arg_vla(_tgif_type)	{ .type = TGIF_TYPE_VLA, .u = { .tgif_static = { .tgif_vla = (_tgif_type) } } }
#define tgif_arg_vla_visitor(_ctx)	{ .type = TGIF_TYPE_VLA_VISITOR, .u = { .tgif_static = { .tgif_vla_app_visitor_ctx = (_ctx) } } }

/* Gather field arguments */

#define tgif_arg_gather_bool(_ptr)		{ .type = TGIF_TYPE_GATHER_BOOL, .u = { .tgif_static = { .tgif_bool_gather_ptr = (_ptr) } } }
#define tgif_arg_gather_byte(_ptr)		{ .type = TGIF_TYPE_GATHER_BYTE, .u = { .tgif_static = { .tgif_byte_gather_ptr = (_ptr) } } }
#define tgif_arg_gather_pointer(_ptr)		{ .type = TGIF_TYPE_GATHER_POINTER, .u = { .tgif_static = { .tgif_integer_gather_ptr = (_ptr) } } }
#define tgif_arg_gather_integer(_ptr)		{ .type = TGIF_TYPE_GATHER_INTEGER, .u = { .tgif_static = { .tgif_integer_gather_ptr = (_ptr) } } }
#define tgif_arg_gather_float(_ptr)		{ .type = TGIF_TYPE_GATHER_FLOAT, .u = { .tgif_static = { .tgif_float_gather_ptr = (_ptr) } } }
#define tgif_arg_gather_string(_ptr)		{ .type = TGIF_TYPE_GATHER_STRING, .u = { .tgif_static = { .tgif_string_gather_ptr = (_ptr) } } }
#define tgif_arg_gather_struct(_ptr)		{ .type = TGIF_TYPE_GATHER_STRUCT, .u = { .tgif_static = { .tgif_struct_gather_ptr = (_ptr) } } }
#define tgif_arg_gather_array(_ptr)		{ .type = TGIF_TYPE_GATHER_ARRAY, .u = { .tgif_static = { .tgif_array_gather_ptr = (_ptr) } } }
#define tgif_arg_gather_vla(_ptr, _length_ptr)	{ .type = TGIF_TYPE_GATHER_VLA, .u = { .tgif_static = { .tgif_vla_gather = { .ptr = (_ptr), .length_ptr = (_length_ptr) } } } }

/* Dynamic field arguments */

#define tgif_arg_dynamic_null(_attr) \
	{ \
		.type = TGIF_TYPE_DYNAMIC_NULL, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_null = { \
					.attr = _attr, \
					.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
				}, \
			}, \
		}, \
	}

#define tgif_arg_dynamic_bool(_val, _attr) \
	{ \
		.type = TGIF_TYPE_DYNAMIC_BOOL, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_bool = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
						.bool_size = sizeof(uint8_t), \
						.len_bits = 0, \
						.byte_order = TGIF_TYPE_BYTE_ORDER_HOST, \
					}, \
					.value = { \
						.tgif_bool8 = !!(_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define tgif_arg_dynamic_byte(_val, _attr) \
	{ \
		.type = TGIF_TYPE_DYNAMIC_BYTE, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_byte = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
					}, \
					.value = (_val), \
				}, \
			}, \
		}, \
	}
#define _tgif_arg_dynamic_string(_val, _byte_order, _unit_size, _attr) \
	{ \
		.type = TGIF_TYPE_DYNAMIC_STRING, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_string = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
						.unit_size = _unit_size, \
						.byte_order = _byte_order, \
					}, \
					.value = (uintptr_t) (_val), \
				}, \
			}, \
		}, \
	}
#define tgif_arg_dynamic_string(_val, _attr) \
	_tgif_arg_dynamic_string(_val, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), TGIF_PARAM(_attr))
#define tgif_arg_dynamic_string16(_val, _attr) \
	_tgif_arg_dynamic_string(_val, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), TGIF_PARAM(_attr))
#define tgif_arg_dynamic_string16_le(_val, _attr) \
	_tgif_arg_dynamic_string(_val, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), TGIF_PARAM(_attr))
#define tgif_arg_dynamic_string16_be(_val, _attr) \
	_tgif_arg_dynamic_string(_val, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), TGIF_PARAM(_attr))
#define tgif_arg_dynamic_string32(_val, _attr) \
	_tgif_arg_dynamic_string(_val, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), TGIF_PARAM(_attr))
#define tgif_arg_dynamic_string32_le(_val, _attr) \
	_tgif_arg_dynamic_string(_val, TGIF_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), TGIF_PARAM(_attr))
#define tgif_arg_dynamic_string32_be(_val, _attr) \
	_tgif_arg_dynamic_string(_val, TGIF_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), TGIF_PARAM(_attr))

#define _tgif_arg_dynamic_integer(_field, _val, _type, _signedness, _byte_order, _integer_size, _len_bits, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_integer = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
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

#define tgif_arg_dynamic_u8(_val, _attr) \
	_tgif_arg_dynamic_integer(.tgif_u8, _val, TGIF_TYPE_DYNAMIC_INTEGER, false, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s8(_val, _attr) \
	_tgif_arg_dynamic_integer(.tgif_s8, _val, TGIF_TYPE_DYNAMIC_INTEGER, true, TGIF_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, TGIF_PARAM(_attr))

#define _tgif_arg_dynamic_u16(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_integer(.tgif_u16, _val, TGIF_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint16_t), 0, TGIF_PARAM(_attr))
#define _tgif_arg_dynamic_u32(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_integer(.tgif_u32, _val, TGIF_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint32_t), 0, TGIF_PARAM(_attr))
#define _tgif_arg_dynamic_u64(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_integer(.tgif_u64, _val, TGIF_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint64_t), 0, TGIF_PARAM(_attr))

#define _tgif_arg_dynamic_s16(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_integer(.tgif_s16, _val, TGIF_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int16_t), 0, TGIF_PARAM(_attr))
#define _tgif_arg_dynamic_s32(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_integer(.tgif_s32, _val, TGIF_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int32_t), 0, TGIF_PARAM(_attr))
#define _tgif_arg_dynamic_s64(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_integer(.tgif_s64, _val, TGIF_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int64_t), 0, TGIF_PARAM(_attr))

#define _tgif_arg_dynamic_pointer(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_integer(.tgif_uptr, (uintptr_t) (_val), TGIF_TYPE_DYNAMIC_POINTER, false, _byte_order, \
			sizeof(uintptr_t), 0, TGIF_PARAM(_attr))

#define _tgif_arg_dynamic_float(_field, _val, _type, _byte_order, _float_size, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_float = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
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

#define _tgif_arg_dynamic_float_binary16(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_float(.tgif_float_binary16, _val, TGIF_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float16), TGIF_PARAM(_attr))
#define _tgif_arg_dynamic_float_binary32(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_float(.tgif_float_binary32, _val, TGIF_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float32), TGIF_PARAM(_attr))
#define _tgif_arg_dynamic_float_binary64(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_float(.tgif_float_binary64, _val, TGIF_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float64), TGIF_PARAM(_attr))
#define _tgif_arg_dynamic_float_binary128(_val, _byte_order, _attr) \
	_tgif_arg_dynamic_float(.tgif_float_binary128, _val, TGIF_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float128), TGIF_PARAM(_attr))

/* Host endian */
#define tgif_arg_dynamic_u16(_val, _attr) 		_tgif_arg_dynamic_u16(_val, TGIF_TYPE_BYTE_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_u32(_val, _attr) 		_tgif_arg_dynamic_u32(_val, TGIF_TYPE_BYTE_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_u64(_val, _attr) 		_tgif_arg_dynamic_u64(_val, TGIF_TYPE_BYTE_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s16(_val, _attr) 		_tgif_arg_dynamic_s16(_val, TGIF_TYPE_BYTE_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s32(_val, _attr) 		_tgif_arg_dynamic_s32(_val, TGIF_TYPE_BYTE_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s64(_val, _attr) 		_tgif_arg_dynamic_s64(_val, TGIF_TYPE_BYTE_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_pointer(_val, _attr) 		_tgif_arg_dynamic_pointer(_val, TGIF_TYPE_BYTE_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary16(_val, _attr)	_tgif_arg_dynamic_float_binary16(_val, TGIF_TYPE_FLOAT_WORD_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary32(_val, _attr)	_tgif_arg_dynamic_float_binary32(_val, TGIF_TYPE_FLOAT_WORD_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary64(_val, _attr)	_tgif_arg_dynamic_float_binary64(_val, TGIF_TYPE_FLOAT_WORD_ORDER_HOST, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary128(_val, _attr)	_tgif_arg_dynamic_float_binary128(_val, TGIF_TYPE_FLOAT_WORD_ORDER_HOST, TGIF_PARAM(_attr))

/* Little endian */
#define tgif_arg_dynamic_u16_le(_val, _attr) 			_tgif_arg_dynamic_u16(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_u32_le(_val, _attr) 			_tgif_arg_dynamic_u32(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_u64_le(_val, _attr) 			_tgif_arg_dynamic_u64(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s16_le(_val, _attr) 			_tgif_arg_dynamic_s16(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s32_le(_val, _attr) 			_tgif_arg_dynamic_s32(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s64_le(_val, _attr) 			_tgif_arg_dynamic_s64(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_pointer_le(_val, _attr) 		_tgif_arg_dynamic_pointer(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary16_le(_val, _attr)		_tgif_arg_dynamic_float_binary16(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary32_le(_val, _attr)		_tgif_arg_dynamic_float_binary32(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary64_le(_val, _attr)		_tgif_arg_dynamic_float_binary64(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary128_le(_val, _attr)	_tgif_arg_dynamic_float_binary128(_val, TGIF_TYPE_BYTE_ORDER_LE, TGIF_PARAM(_attr))

/* Big endian */
#define tgif_arg_dynamic_u16_be(_val, _attr) 			_tgif_arg_dynamic_u16(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_u32_be(_val, _attr) 			_tgif_arg_dynamic_u32(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_u64_be(_val, _attr) 			_tgif_arg_dynamic_u64(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s16_be(_val, _attr) 			_tgif_arg_dynamic_s16(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s32_be(_val, _attr) 			_tgif_arg_dynamic_s32(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_s64_be(_val, _attr) 			_tgif_arg_dynamic_s64(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_pointer_be(_val, _attr) 		_tgif_arg_dynamic_pointer(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary16_be(_val, _attr)		_tgif_arg_dynamic_float_binary16(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary32_be(_val, _attr)		_tgif_arg_dynamic_float_binary32(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary64_be(_val, _attr)		_tgif_arg_dynamic_float_binary64(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))
#define tgif_arg_dynamic_float_binary128_be(_val, _attr)	_tgif_arg_dynamic_float_binary128(_val, TGIF_TYPE_BYTE_ORDER_BE, TGIF_PARAM(_attr))

#define tgif_arg_dynamic_vla(_vla) \
	{ \
		.type = TGIF_TYPE_DYNAMIC_VLA, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_dynamic_vla = (_vla), \
			}, \
		}, \
	}

#define tgif_arg_dynamic_vla_visitor(_dynamic_vla_visitor, _ctx, _attr) \
	{ \
		.type = TGIF_TYPE_DYNAMIC_VLA_VISITOR, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_dynamic_vla_visitor = { \
					.app_ctx = _ctx, \
					.visitor = _dynamic_vla_visitor, \
					.attr = _attr, \
					.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
				}, \
			}, \
		}, \
	}

#define tgif_arg_dynamic_struct(_struct) \
	{ \
		.type = TGIF_TYPE_DYNAMIC_STRUCT, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_dynamic_struct = (_struct), \
			}, \
		}, \
	}

#define tgif_arg_dynamic_struct_visitor(_dynamic_struct_visitor, _ctx, _attr) \
	{ \
		.type = TGIF_TYPE_DYNAMIC_STRUCT_VISITOR, \
		.u = { \
			.tgif_dynamic = { \
				.tgif_dynamic_struct_visitor = { \
					.app_ctx = _ctx, \
					.visitor = _dynamic_struct_visitor, \
					.attr = _attr, \
					.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
				}, \
			}, \
		}, \
	}

#define tgif_arg_dynamic_define_vec(_identifier, _sav, _attr) \
	const struct tgif_arg _identifier##_vec[] = { _sav }; \
	const struct tgif_arg_dynamic_vla _identifier = { \
		.sav = _identifier##_vec, \
		.attr = _attr, \
		.len = TGIF_ARRAY_SIZE(_identifier##_vec), \
		.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
	}

#define tgif_arg_dynamic_define_struct(_identifier, _struct_fields, _attr) \
	const struct tgif_arg_dynamic_field _identifier##_fields[] = { _struct_fields }; \
	const struct tgif_arg_dynamic_struct _identifier = { \
		.fields = _identifier##_fields, \
		.attr = _attr, \
		.len = TGIF_ARRAY_SIZE(_identifier##_fields), \
		.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
	}

#define tgif_arg_define_vec(_identifier, _sav) \
	const struct tgif_arg _identifier##_vec[] = { _sav }; \
	const struct tgif_arg_vec _identifier = { \
		.sav = _identifier##_vec, \
		.len = TGIF_ARRAY_SIZE(_identifier##_vec), \
	}

#define tgif_arg_dynamic_field(_name, _elem) \
	{ \
		.field_name = _name, \
		.elem = _elem, \
	}

/*
 * Event instrumentation description registration, runtime enabled state
 * check, and instrumentation invocation.
 */

#define tgif_arg_list(...)	__VA_ARGS__

#define tgif_event_cond(_identifier) \
	if (tgif_unlikely(__atomic_load_n(&tgif_event_enable__##_identifier, \
					__ATOMIC_RELAXED)))

#define tgif_event_call(_identifier, _sav) \
	{ \
		const struct tgif_arg tgif_sav[] = { _sav }; \
		const struct tgif_arg_vec tgif_arg_vec = { \
			.sav = tgif_sav, \
			.len = TGIF_ARRAY_SIZE(tgif_sav), \
		}; \
		tgif_call(&(_identifier), &tgif_arg_vec); \
	}

#define tgif_event(_identifier, _sav) \
	tgif_event_cond(_identifier) \
		tgif_event_call(_identifier, TGIF_PARAM(_sav))

#define tgif_event_call_variadic(_identifier, _sav, _var_fields, _attr) \
	{ \
		const struct tgif_arg tgif_sav[] = { _sav }; \
		const struct tgif_arg_vec tgif_arg_vec = { \
			.sav = tgif_sav, \
			.len = TGIF_ARRAY_SIZE(tgif_sav), \
		}; \
		const struct tgif_arg_dynamic_field tgif_fields[] = { _var_fields }; \
		const struct tgif_arg_dynamic_struct var_struct = { \
			.fields = tgif_fields, \
			.attr = _attr, \
			.len = TGIF_ARRAY_SIZE(tgif_fields), \
			.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
		}; \
		tgif_call_variadic(&(_identifier), &tgif_arg_vec, &var_struct); \
	}

#define tgif_event_variadic(_identifier, _sav, _var, _attr) \
	tgif_event_cond(_identifier) \
		tgif_event_call_variadic(_identifier, TGIF_PARAM(_sav), TGIF_PARAM(_var), TGIF_PARAM(_attr))

#define _tgif_define_event(_linkage, _identifier, _provider, _event, _loglevel, _fields, _attr, _flags) \
	_linkage uintptr_t tgif_event_enable__##_identifier __attribute__((section("tgif_event_enable"))); \
	_linkage struct tgif_event_description __attribute__((section("tgif_event_description"))) \
			_identifier = { \
		.enabled = &(tgif_event_enable__##_identifier), \
		.provider_name = _provider, \
		.event_name = _event, \
		.fields = _fields, \
		.attr = _attr, \
		.callbacks = &tgif_empty_callback, \
		.flags = (_flags), \
		.version = 0, \
		.loglevel = _loglevel, \
		.nr_fields = TGIF_ARRAY_SIZE(TGIF_PARAM(_fields)), \
		.nr_attr = TGIF_ARRAY_SIZE(TGIF_PARAM(_attr)), \
		.nr_callbacks = 0, \
	}; \
	static const struct tgif_event_description *tgif_event_ptr__##_identifier \
		__attribute__((section("tgif_event_description_ptr"), used)) = &(_identifier);

#define tgif_static_event(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_tgif_define_event(static, _identifier, _provider, _event, _loglevel, TGIF_PARAM(_fields), \
			TGIF_PARAM(_attr), 0)

#define tgif_static_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_tgif_define_event(static, _identifier, _provider, _event, _loglevel, TGIF_PARAM(_fields), \
			TGIF_PARAM(_attr), TGIF_EVENT_FLAG_VARIADIC)

#define tgif_hidden_event(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_tgif_define_event(__attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, TGIF_PARAM(_fields), TGIF_PARAM(_attr), 0)

#define tgif_hidden_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_tgif_define_event(__attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, TGIF_PARAM(_fields), TGIF_PARAM(_attr), TGIF_EVENT_FLAG_VARIADIC)

#define tgif_export_event(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_tgif_define_event(__attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, TGIF_PARAM(_fields), TGIF_PARAM(_attr), 0)

#define tgif_export_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_tgif_define_event(__attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, TGIF_PARAM(_fields), TGIF_PARAM(_attr), TGIF_EVENT_FLAG_VARIADIC)

#define tgif_declare_event(_identifier) \
	extern uintptr_t tgif_event_enable_##_identifier; \
	extern struct tgif_event_description _identifier

#ifdef __cplusplus
extern "C" {
#endif

extern const struct tgif_callback tgif_empty_callback;

void tgif_call(const struct tgif_event_description *desc,
	const struct tgif_arg_vec *tgif_arg_vec);
void tgif_call_variadic(const struct tgif_event_description *desc,
	const struct tgif_arg_vec *tgif_arg_vec,
	const struct tgif_arg_dynamic_struct *var_struct);

struct tgif_events_register_handle *tgif_events_register(struct tgif_event_description **events,
		uint32_t nr_events);
void tgif_events_unregister(struct tgif_events_register_handle *handle);

/*
 * Userspace tracer registration API. This allows userspace tracers to
 * register event notification callbacks to be notified of the currently
 * registered instrumentation, and to register their callbacks to
 * specific events.
 */
typedef void (*tgif_tracer_callback_func)(const struct tgif_event_description *desc,
			const struct tgif_arg_vec *tgif_arg_vec,
			void *priv);
typedef void (*tgif_tracer_callback_variadic_func)(const struct tgif_event_description *desc,
			const struct tgif_arg_vec *tgif_arg_vec,
			const struct tgif_arg_dynamic_struct *var_struct,
			void *priv);

int tgif_tracer_callback_register(struct tgif_event_description *desc,
		tgif_tracer_callback_func call,
		void *priv);
int tgif_tracer_callback_variadic_register(struct tgif_event_description *desc,
		tgif_tracer_callback_variadic_func call_variadic,
		void *priv);
int tgif_tracer_callback_unregister(struct tgif_event_description *desc,
		tgif_tracer_callback_func call,
		void *priv);
int tgif_tracer_callback_variadic_unregister(struct tgif_event_description *desc,
		tgif_tracer_callback_variadic_func call_variadic,
		void *priv);

enum tgif_tracer_notification {
	TGIF_TRACER_NOTIFICATION_INSERT_EVENTS,
	TGIF_TRACER_NOTIFICATION_REMOVE_EVENTS,
};

/* Callback is invoked with tgif library internal lock held. */
struct tgif_tracer_handle *tgif_tracer_event_notification_register(
		void (*cb)(enum tgif_tracer_notification notif,
			struct tgif_event_description **events, uint32_t nr_events, void *priv),
		void *priv);
void tgif_tracer_event_notification_unregister(struct tgif_tracer_handle *handle);

/*
 * Explicit hooks to initialize/finalize the tgif instrumentation
 * library. Those are also library constructor/destructor.
 */
void tgif_init(void) __attribute__((constructor));
void tgif_exit(void) __attribute__((destructor));

/*
 * The following constructors/destructors perform automatic registration
 * of the declared tgif events. Those may have to be called explicitly
 * in a statically linked library.
 */

/*
 * These weak symbols, the constructor, and destructor take care of
 * registering only _one_ instance of the tgif instrumentation per
 * shared-ojbect (or for the whole main program).
 */
extern struct tgif_event_description * __start_tgif_event_description_ptr[]
	__attribute__((weak, visibility("hidden")));
extern struct tgif_event_description * __stop_tgif_event_description_ptr[]
	__attribute__((weak, visibility("hidden")));
int tgif_event_description_ptr_registered
        __attribute__((weak, visibility("hidden")));
struct tgif_events_register_handle *tgif_events_handle
	__attribute__((weak, visibility("hidden")));

static void
tgif_event_description_ptr_init(void)
	__attribute__((no_instrument_function))
	__attribute__((constructor));
static void
tgif_event_description_ptr_init(void)
{
	if (tgif_event_description_ptr_registered++)
		return;
	tgif_events_handle = tgif_events_register(__start_tgif_event_description_ptr,
		__stop_tgif_event_description_ptr - __start_tgif_event_description_ptr);
}

static void
tgif_event_description_ptr_exit(void)
	__attribute__((no_instrument_function))
	__attribute__((destructor));
static void
tgif_event_description_ptr_exit(void)
{
	if (--tgif_event_description_ptr_registered)
		return;
	tgif_events_unregister(tgif_events_handle);
	tgif_events_handle = NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* _TGIF_TRACE_H */
