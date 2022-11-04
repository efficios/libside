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
#include <side/macros.h>
#include <side/endian.h>

/* SIDE stands for "Static Instrumentation Dynamically Enabled" */

//TODO: as those structures will be ABI, we need to either consider them
//fixed forever, or think of a scheme that would allow their binary
//representation to be extended if need be.

struct side_arg;
struct side_arg_vec;
struct side_arg_dynamic;
struct side_arg_dynamic_vla;
struct side_type;
struct side_event_field;
struct side_tracer_visitor_ctx;
struct side_tracer_dynamic_struct_visitor_ctx;
struct side_tracer_dynamic_vla_visitor_ctx;
struct side_event_description;
struct side_arg_dynamic_event_struct;
struct side_events_register_handle;

enum side_type_label {
	/* Statically declared types */
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
	SIDE_TYPE_POINTER32,
	SIDE_TYPE_POINTER64,
	SIDE_TYPE_FLOAT_BINARY16,
	SIDE_TYPE_FLOAT_BINARY32,
	SIDE_TYPE_FLOAT_BINARY64,
	SIDE_TYPE_FLOAT_BINARY128,
	SIDE_TYPE_STRING,

	/* Statically declared compound types */
	SIDE_TYPE_STRUCT,
	SIDE_TYPE_STRUCT_SG,
	SIDE_TYPE_ARRAY,
	SIDE_TYPE_VLA,
	SIDE_TYPE_VLA_VISITOR,

	SIDE_TYPE_ARRAY_U8,
	SIDE_TYPE_ARRAY_U16,
	SIDE_TYPE_ARRAY_U32,
	SIDE_TYPE_ARRAY_U64,
	SIDE_TYPE_ARRAY_S8,
	SIDE_TYPE_ARRAY_S16,
	SIDE_TYPE_ARRAY_S32,
	SIDE_TYPE_ARRAY_S64,
	SIDE_TYPE_ARRAY_BYTE,
	SIDE_TYPE_ARRAY_POINTER32,
	SIDE_TYPE_ARRAY_POINTER64,

	SIDE_TYPE_VLA_U8,
	SIDE_TYPE_VLA_U16,
	SIDE_TYPE_VLA_U32,
	SIDE_TYPE_VLA_U64,
	SIDE_TYPE_VLA_S8,
	SIDE_TYPE_VLA_S16,
	SIDE_TYPE_VLA_S32,
	SIDE_TYPE_VLA_S64,
	SIDE_TYPE_VLA_BYTE,
	SIDE_TYPE_VLA_POINTER32,
	SIDE_TYPE_VLA_POINTER64,

	/* Statically declared enumeration types */
	SIDE_TYPE_ENUM,
	SIDE_TYPE_ENUM_BITMAP,

	SIDE_TYPE_DYNAMIC,

	/* Scatter-gather types */
	SIDE_TYPE_SG_UNSIGNED_INT,
	SIDE_TYPE_SG_SIGNED_INT,
	SIDE_TYPE_SG_STRUCT,

	/* Dynamic types */
	SIDE_TYPE_DYNAMIC_NULL,
	SIDE_TYPE_DYNAMIC_BOOL,
	SIDE_TYPE_DYNAMIC_U8,
	SIDE_TYPE_DYNAMIC_U16,
	SIDE_TYPE_DYNAMIC_U32,
	SIDE_TYPE_DYNAMIC_U64,
	SIDE_TYPE_DYNAMIC_S8,
	SIDE_TYPE_DYNAMIC_S16,
	SIDE_TYPE_DYNAMIC_S32,
	SIDE_TYPE_DYNAMIC_S64,
	SIDE_TYPE_DYNAMIC_BYTE,
	SIDE_TYPE_DYNAMIC_POINTER32,
	SIDE_TYPE_DYNAMIC_POINTER64,
	SIDE_TYPE_DYNAMIC_FLOAT_BINARY16,
	SIDE_TYPE_DYNAMIC_FLOAT_BINARY32,
	SIDE_TYPE_DYNAMIC_FLOAT_BINARY64,
	SIDE_TYPE_DYNAMIC_FLOAT_BINARY128,
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
	SIDE_ATTR_TYPE_POINTER32,
	SIDE_ATTR_TYPE_POINTER64,
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

typedef enum side_visitor_status (*side_visitor)(
		const struct side_tracer_visitor_ctx *tracer_ctx,
		void *app_ctx);
typedef enum side_visitor_status (*side_dynamic_struct_visitor)(
		const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
		void *app_ctx);
typedef enum side_visitor_status (*side_dynamic_vla_visitor)(
		const struct side_tracer_dynamic_vla_visitor_ctx *tracer_ctx,
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

struct side_attr_value {
	uint32_t type;	/* enum side_attr_type */
	union {
		uint8_t bool_value;
		uint64_t string_value;	/* const char * */
		union side_integer_value integer_value;
		union side_float_value float_value;
	} SIDE_PACKED u;
};

/* User attributes. */
struct side_attr {
	const char *key;
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
} SIDE_PACKED;

struct side_type_byte {
	const struct side_attr *attr;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_string {
	const struct side_attr *attr;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_integer {
	const struct side_attr *attr;
	uint32_t nr_attr;
	uint16_t integer_size_bits;	/* bits */
	uint16_t len_bits;		/* bits */
	uint8_t signedness;		/* true/false */
	uint8_t byte_order;		/* enum side_type_label_byte_order */
} SIDE_PACKED;

struct side_type_float {
	const struct side_attr *attr;
	uint32_t nr_attr;
	uint32_t byte_order;		/* enum side_type_label_byte_order */
	uint16_t float_size_bits;	/* bits */
} SIDE_PACKED;

struct side_enum_mapping {
	int64_t range_begin;
	int64_t range_end;
	const char *label;
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
	const char *label;
} SIDE_PACKED;

struct side_enum_bitmap_mappings {
	const struct side_enum_bitmap_mapping *mappings;
	const struct side_attr *attr;
	uint32_t nr_mappings;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_type_struct {
	uint32_t nr_fields;
	uint32_t nr_attr;
	const struct side_event_field *fields;
	const struct side_attr *attr;
} SIDE_PACKED;

struct side_type_sg {
	uint64_t offset;	/* bytes */
	union {
		struct {
			struct side_type_integer type;
			uint16_t offset_bits;		/* bits */
		} SIDE_PACKED side_integer;
		const struct side_type_struct *side_struct;
	} SIDE_PACKED u;
} SIDE_PACKED;

/* Statically defined types. */
struct side_type {
	uint32_t type;	/* enum side_type_label */
	union {
		/* Basic types */
		struct side_type_null side_null;
		struct side_type_bool side_bool;
		struct side_type_byte side_byte;
		struct side_type_string side_string;
		struct side_type_integer side_integer;
		struct side_type_float side_float;

		/* Compound types */
		struct {
			const struct side_type *elem_type;
			const struct side_attr *attr;
			uint32_t length;
			uint32_t nr_attr;
		} SIDE_PACKED side_array;
		struct {
			const struct side_type *elem_type;
			const struct side_attr *attr;
			uint32_t nr_attr;
		} SIDE_PACKED side_vla;
		struct {
			const struct side_type *elem_type;
			side_visitor visitor;
			const struct side_attr *attr;
			uint32_t nr_attr;
		} SIDE_PACKED side_vla_visitor;
		const struct side_type_struct *side_struct;

		/* Enumeration types */
		struct {
			const struct side_enum_mappings *mappings;
			const struct side_type *elem_type;
		} SIDE_PACKED side_enum;
		struct {
			const struct side_enum_bitmap_mappings *mappings;
			const struct side_type *elem_type;
		} SIDE_PACKED side_enum_bitmap;

		/* Scatter-gather type */
		struct side_type_sg side_sg;
	} SIDE_PACKED u;
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
			const struct side_arg_dynamic_event_struct *var_struct,
			void *priv);
	} SIDE_PACKED u;
	void *priv;
} SIDE_PACKED;

struct side_arg_static {
	/* Basic types */
	uint8_t bool_value;
	uint8_t byte_value;
	uint64_t string_value;	/* const char * */
	union side_integer_value integer_value;
	union side_float_value float_value;

	/* Compound types */
	const struct side_arg_vec *side_struct;
	const struct side_arg_vec *side_array;
	const struct side_arg_vec *side_vla;
	void *side_vla_app_visitor_ctx;
	void *side_array_fixint;
	struct {
		void *p;
		uint32_t length;
	} SIDE_PACKED side_vla_fixint;

	/* Scatter-gather */
	void *side_integer_sg_ptr;
	void *side_struct_sg_ptr;
} SIDE_PACKED;

struct side_arg_dynamic {
	/* Basic types */
	struct side_type_null side_null;
	struct {
		struct side_type_bool type;
		uint8_t value;
	} SIDE_PACKED side_bool;

	struct {
		struct side_type_byte type;
		uint8_t value;
	} SIDE_PACKED side_byte;

	struct {
		struct side_type_string type;
		uint64_t value;	/* const char * */
	} SIDE_PACKED side_string;

	/* Integer type */
	struct {
		struct side_type_integer type;
		union side_integer_value value;
	} SIDE_PACKED side_integer;

	struct {
		struct side_type_float type;
		union side_float_value value;
	} SIDE_PACKED side_float;

	/* Compound types */
	const struct side_arg_dynamic_event_struct *side_dynamic_struct;
	struct {
		void *app_ctx;
		side_dynamic_struct_visitor visitor;
		const struct side_attr *attr;
		uint32_t nr_attr;
	} SIDE_PACKED side_dynamic_struct_visitor;
	const struct side_arg_dynamic_vla *side_dynamic_vla;
	struct {
		void *app_ctx;
		side_dynamic_vla_visitor visitor;
		const struct side_attr *attr;
		uint32_t nr_attr;
	} SIDE_PACKED side_dynamic_vla_visitor;
} SIDE_PACKED;

struct side_arg {
	enum side_type_label type;
	union {
		struct side_arg_static side_static;
		struct side_arg_dynamic side_dynamic;
	} SIDE_PACKED u;
} SIDE_PACKED;

struct side_arg_vec {
	const struct side_arg *sav;
	uint32_t len;
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

struct side_arg_dynamic_vla {
	const struct side_arg *sav;
	const struct side_attr *attr;
	uint32_t len;
	uint32_t nr_attr;
} SIDE_PACKED;

struct side_arg_dynamic_event_field {
	const char *field_name;
	const struct side_arg elem;
} SIDE_PACKED;

struct side_arg_dynamic_event_struct {
	const struct side_arg_dynamic_event_field *fields;
	const struct side_attr *attr;
	uint32_t len;
	uint32_t nr_attr;
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
			const struct side_arg_dynamic_event_field *dynamic_field);
	void *priv;		/* Private tracer context. */
} SIDE_PACKED;

struct side_tracer_dynamic_vla_visitor_ctx {
	enum side_visitor_status (*write_elem)(
			const struct side_tracer_dynamic_vla_visitor_ctx *tracer_ctx,
			const struct side_arg *elem);
	void *priv;		/* Private tracer context. */
} SIDE_PACKED;

/* Event and type attributes */

#if SIDE_BITS_PER_LONG == 64
# define SIDE_TYPE_POINTER_HOST		SIDE_TYPE_POINTER64
# define SIDE_TYPE_ARRAY_POINTER_HOST	SIDE_TYPE_ARRAY_POINTER64
# define SIDE_TYPE_VLA_POINTER_HOST	SIDE_TYPE_VLA_POINTER64
# define SIDE_TYPE_DYNAMIC_POINTER_HOST	SIDE_TYPE_DYNAMIC_POINTER64
# define SIDE_ATTR_TYPE_POINTER_HOST	SIDE_ATTR_TYPE_POINTER64
# define SIDE_PTR_HOST			.side_u64
#else
# define SIDE_TYPE_POINTER_HOST		SIDE_TYPE_POINTER32
# define SIDE_TYPE_ARRAY_POINTER_HOST	SIDE_TYPE_ARRAY_POINTER32
# define SIDE_TYPE_VLA_POINTER_HOST	SIDE_TYPE_VLA_POINTER32
# define SIDE_TYPE_DYNAMIC_POINTER_HOST	SIDE_TYPE_DYNAMIC_POINTER32
# define SIDE_ATTR_TYPE_POINTER_HOST	SIDE_ATTR_TYPE_POINTER32
# define SIDE_PTR_HOST			.side_u32
#endif

#define side_attr(_key, _value)	\
	{ \
		.key = _key, \
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
#define side_attr_pointer(_val)		{ .type = SIDE_ATTR_TYPE_POINTER_HOST, .u = { .integer_value = { SIDE_PTR_HOST = (uintptr_t) (_val) } } }
#define side_attr_float_binary16(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY16, .u = { .float_value = { .side_float_binary16 = (_val) } } }
#define side_attr_float_binary32(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY32, .u = { .float_value = { .side_float_binary32 = (_val) } } }
#define side_attr_float_binary64(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY64, .u = { .float_value = { .side_float_binary64 = (_val) } } }
#define side_attr_float_binary128(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY128, .u = { .float_value = { .side_float_binary128 = (_val) } } }
#define side_attr_string(_val)		{ .type = SIDE_ATTR_TYPE_STRING, .u = { .string_value = (uintptr_t) (_val) } }

/* Static field definition */

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

#define side_type_string(_attr) \
	{ \
		.type = SIDE_TYPE_STRING, \
		.u = { \
			.side_string = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}

#define side_type_dynamic() \
	{ \
		.type = SIDE_TYPE_DYNAMIC, \
	}

#define _side_type_integer(_type, _signedness, _byte_order, _integer_size_bits, _len_bits, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_integer = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.integer_size_bits = _integer_size_bits, \
				.len_bits = _len_bits, \
				.signedness = _signedness, \
				.byte_order = _byte_order, \
			}, \
		}, \
	}

#define _side_type_float(_type, _byte_order, _float_size_bits, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_float = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.byte_order = _byte_order, \
				.float_size_bits = _float_size_bits, \
			}, \
		}, \
	}

#define _side_field(_name, _type) \
	{ \
		.field_name = _name, \
		.side_type = _type, \
	}

/* Host endian */
#define side_type_u8(_attr)				_side_type_integer(SIDE_TYPE_U8, false, SIDE_TYPE_BYTE_ORDER_HOST, 8, 8, SIDE_PARAM(_attr))
#define side_type_u16(_attr)				_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_HOST, 16, 16, SIDE_PARAM(_attr))
#define side_type_u32(_attr)				_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_HOST, 32, 32, SIDE_PARAM(_attr))
#define side_type_u64(_attr)				_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_HOST, 64, 64, SIDE_PARAM(_attr))
#define side_type_s8(_attr)				_side_type_integer(SIDE_TYPE_S8, true, SIDE_TYPE_BYTE_ORDER_HOST, 8, 8, SIDE_PARAM(_attr))
#define side_type_s16(_attr)				_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_HOST, 16, 16, SIDE_PARAM(_attr))
#define side_type_s32(_attr)				_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_HOST, 32, 32, SIDE_PARAM(_attr))
#define side_type_s64(_attr)				_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_HOST, 64, 64, SIDE_PARAM(_attr))
#define side_type_pointer(_attr)			_side_type_integer(SIDE_TYPE_POINTER_HOST, false, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_BITS_PER_LONG, \
									SIDE_BITS_PER_LONG, SIDE_PARAM(_attr))
#define side_type_float_binary16(_attr)			_side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, 16, SIDE_PARAM(_attr))
#define side_type_float_binary32(_attr)			_side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, 32, SIDE_PARAM(_attr))
#define side_type_float_binary64(_attr)			_side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, 64, SIDE_PARAM(_attr))
#define side_type_float_binary128(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, 128, SIDE_PARAM(_attr))

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
#define side_field_dynamic(_name)			_side_field(_name, side_type_dynamic())

/* Little endian */
#define side_type_u16_le(_attr)				_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_LE, 16, 16, SIDE_PARAM(_attr))
#define side_type_u32_le(_attr)				_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_LE, 32, 32, SIDE_PARAM(_attr))
#define side_type_u64_le(_attr)				_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_LE, 64, 64, SIDE_PARAM(_attr))
#define side_type_s16_le(_attr)				_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_LE, 16, 16, SIDE_PARAM(_attr))
#define side_type_s32_le(_attr)				_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_LE, 32, 32, SIDE_PARAM(_attr))
#define side_type_s64_le(_attr)				_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_LE, 64, 64, SIDE_PARAM(_attr))
#define side_type_pointer_le(_attr)			_side_type_integer(SIDE_TYPE_POINTER_HOST, false, SIDE_TYPE_BYTE_ORDER_LE, SIDE_BITS_PER_LONG, \
									SIDE_BITS_PER_LONG, SIDE_PARAM(_attr))
#define side_type_float_binary16_le(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_LE, 16, SIDE_PARAM(_attr))
#define side_type_float_binary32_le(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_LE, 32, SIDE_PARAM(_attr))
#define side_type_float_binary64_le(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_LE, 64, SIDE_PARAM(_attr))
#define side_type_float_binary128_le(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_LE, 128, SIDE_PARAM(_attr))

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

/* Big endian */
#define side_type_u16_be(_attr)				_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_BE, 16, 16, SIDE_PARAM(_attr))
#define side_type_u32_be(_attr)				_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_BE, 32, 32, SIDE_PARAM(_attr))
#define side_type_u64_be(_attr)				_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_BE, 64, 64, SIDE_PARAM(_attr))
#define side_type_s16_be(_attr)				_side_type_integer(SIDE_TYPE_S16, false, SIDE_TYPE_BYTE_ORDER_BE, 16, 16, SIDE_PARAM(_attr))
#define side_type_s32_be(_attr)				_side_type_integer(SIDE_TYPE_S32, false, SIDE_TYPE_BYTE_ORDER_BE, 32, 32, SIDE_PARAM(_attr))
#define side_type_s64_be(_attr)				_side_type_integer(SIDE_TYPE_S64, false, SIDE_TYPE_BYTE_ORDER_BE, 64, 64, SIDE_PARAM(_attr))
#define side_type_pointer_be(_attr)			_side_type_integer(SIDE_TYPE_POINTER_HOST, false, SIDE_TYPE_BYTE_ORDER_BE, SIDE_BITS_PER_LONG, \
									SIDE_BITS_PER_LONG, SIDE_PARAM(_attr))
#define side_type_float_binary16_be(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_BE, 16, SIDE_PARAM(_attr))
#define side_type_float_binary32_be(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_BE, 32, SIDE_PARAM(_attr))
#define side_type_float_binary64_be(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_BE, 64, SIDE_PARAM(_attr))
#define side_type_float_binary128_be(_attr)		_side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_BE, 128, SIDE_PARAM(_attr))

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
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
		.nr_attr  = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		.fields = _fields, \
		.attr = _attr, \
	}

#define side_define_struct(_identifier, _fields, _attr) \
	const struct side_type_struct _identifier = _side_type_struct_define(SIDE_PARAM(_fields), SIDE_PARAM(_attr))

#define side_struct_literal(_fields, _attr) \
	SIDE_COMPOUND_LITERAL(const struct side_type_struct, \
		_side_type_struct_define(SIDE_PARAM(_fields), SIDE_PARAM(_attr)))

/* Scatter-gather fields */

#define _side_type_sg_integer(_type, _signedness, _byte_order, _offset, _integer_size_bits, _offset_bits, _len_bits, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_sg = { \
				.offset = _offset, \
				.u = { \
					.side_integer = { \
						.type = { \
							.attr = _attr, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
							.integer_size_bits = _integer_size_bits, \
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

#define side_type_sg_unsigned_integer(_integer_offset, _integer_size_bits, _offset_bits, _len_bits, _attr) \
	_side_type_sg_integer(SIDE_TYPE_SG_UNSIGNED_INT, false,  SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size_bits, _offset_bits, _len_bits, SIDE_PARAM(_attr))
#define side_type_sg_signed_integer(_integer_offset, _integer_size_bits, _offset_bits, _len_bits, _attr) \
	_side_type_sg_integer(SIDE_TYPE_SG_SIGNED_INT, true, SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size_bits, _offset_bits, _len_bits, SIDE_PARAM(_attr))

#define side_field_sg_unsigned_integer(_name, _integer_offset, _integer_size_bits, _offset_bits, _len_bits, _attr) \
	_side_field(_name, side_type_sg_unsigned_integer(_integer_offset, _integer_size_bits, _offset_bits, _len_bits, SIDE_PARAM(_attr)))
#define side_field_sg_signed_integer(_name, _integer_offset, _integer_size_bits, _offset_bits, _len_bits, _attr) \
	_side_field(_name, side_type_sg_signed_integer(_integer_offset, _integer_size_bits, _offset_bits, _len_bits, SIDE_PARAM(_attr)))

#define side_type_struct_sg(_struct_sg, _offset) \
	{ \
		.type = SIDE_TYPE_STRUCT_SG, \
		.u = { \
			.side_sg = { \
				.offset = _offset, \
				.u = { \
					.side_struct = _struct_sg, \
				}, \
			}, \
		}, \
	}
#define side_field_struct_sg(_name, _struct_sg, _offset) \
	_side_field(_name, side_type_struct_sg(SIDE_PARAM(_struct_sg), _offset))

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

#define side_elem(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type, __VA_ARGS__)

#define side_field_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_event_field, __VA_ARGS__)

/* Static field arguments */

#define side_arg_null(_val)		{ .type = SIDE_TYPE_NULL }
#define side_arg_bool(_val)		{ .type = SIDE_TYPE_BOOL, .u = { .side_static = { .bool_value = !!(_val) } } }
#define side_arg_byte(_val)		{ .type = SIDE_TYPE_BYTE, .u = { .side_static = { .byte_value = (_val) } } }
#define side_arg_string(_val)		{ .type = SIDE_TYPE_STRING, .u = { .side_static = { .string_value = (uintptr_t) (_val) } } }

#define side_arg_u8(_val)		{ .type = SIDE_TYPE_U8, .u = { .side_static = {  .integer_value = { .side_u8 = (_val) } } } }
#define side_arg_u16(_val)		{ .type = SIDE_TYPE_U16, .u = { .side_static = { .integer_value = { .side_u16 = (_val) } } } }
#define side_arg_u32(_val)		{ .type = SIDE_TYPE_U32, .u = { .side_static = { .integer_value = { .side_u32 = (_val) } } } }
#define side_arg_u64(_val)		{ .type = SIDE_TYPE_U64, .u = { .side_static = { .integer_value = { .side_u64 = (_val) } } } }
#define side_arg_s8(_val)		{ .type = SIDE_TYPE_S8, .u = { .side_static = { .integer_value = { .side_s8 = (_val) } } } }
#define side_arg_s16(_val)		{ .type = SIDE_TYPE_S16, .u = { .side_static = { .integer_value = { .side_s16 = (_val) } } } }
#define side_arg_s32(_val)		{ .type = SIDE_TYPE_S32, .u = { .side_static = { .integer_value = { .side_s32 = (_val) } } } }
#define side_arg_s64(_val)		{ .type = SIDE_TYPE_S64, .u = { .side_static = { .integer_value = { .side_s64 = (_val) } } } }
#define side_arg_pointer(_val)		{ .type = SIDE_TYPE_POINTER_HOST, .u = { .side_static = { .integer_value = { SIDE_PTR_HOST = (uintptr_t) (_val) } } } }
#define side_arg_enum_bitmap8(_val)	{ .type = SIDE_TYPE_ENUM_BITMAP8, .u = { .side_static = { .integer_value = { .side_u8 = (_val) } } } }
#define side_arg_enum_bitmap16(_val)	{ .type = SIDE_TYPE_ENUM_BITMAP16, .u = { .side_static = { .integer_value = { .side_u16 = (_val) } } } }
#define side_arg_enum_bitmap32(_val)	{ .type = SIDE_TYPE_ENUM_BITMAP32, .u = { .side_static = { .integer_value = { .side_u32 = (_val) } } } }
#define side_arg_enum_bitmap64(_val)	{ .type = SIDE_TYPE_ENUM_BITMAP64, .u = { .side_static = { .integer_value = { .side_u64 = (_val) } } } }
#define side_arg_enum_bitmap_array(_side_type)	{ .type = SIDE_TYPE_ENUM_BITMAP_ARRAY, .u = { .side_static = { .side_array = (_side_type) } } }
#define side_arg_enum_bitmap_vla(_side_type)	{ .type = SIDE_TYPE_ENUM_BITMAP_VLA, .u = { .side_static = { .side_vla = (_side_type) } } }
#define side_arg_float_binary16(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY16, .u = { .side_static = { .float_value = { .side_float_binary16 = (_val) } } } }
#define side_arg_float_binary32(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY32, .u = { .side_static = { .float_value = { .side_float_binary32 = (_val) } } } }
#define side_arg_float_binary64(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY64, .u = { .side_static = { .float_value = { .side_float_binary64 = (_val) } } } }
#define side_arg_float_binary128(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY128, .u = { .side_static = { .float_value = { .side_float_binary128 = (_val) } } } }

#define side_arg_struct(_side_type)	{ .type = SIDE_TYPE_STRUCT, .u = { .side_static = { .side_struct = (_side_type) } } }
#define side_arg_struct_sg(_ptr)	{ .type = SIDE_TYPE_STRUCT_SG, .u = { .side_static = { .side_struct_sg_ptr = (_ptr) } } }
#define side_arg_unsigned_integer_sg(_ptr)	{ .type = SIDE_TYPE_SG_UNSIGNED_INT, .u = { .side_static = { .side_integer_sg_ptr = (_ptr) } } }
#define side_arg_signed_integer_sg(_ptr)	{ .type = SIDE_TYPE_SG_SIGNED_INT, .u = { .side_static = { .side_integer_sg_ptr = (_ptr) } } }
#define side_arg_array(_side_type)	{ .type = SIDE_TYPE_ARRAY, .u = { .side_static = { .side_array = (_side_type) } } }
#define side_arg_vla(_side_type)	{ .type = SIDE_TYPE_VLA, .u = { .side_static = { .side_vla = (_side_type) } } }
#define side_arg_vla_visitor(_ctx)	{ .type = SIDE_TYPE_VLA_VISITOR, .u = { .side_static = { .side_vla_app_visitor_ctx = (_ctx) } } }

#define side_arg_array_u8(_ptr)		{ .type = SIDE_TYPE_ARRAY_U8, .u = { .side_static = { .side_array_fixint = (_ptr) } } }
#define side_arg_array_u16(_ptr)	{ .type = SIDE_TYPE_ARRAY_U16, .u = { .side_static = { .side_array_fixint = (_ptr) } } }
#define side_arg_array_u32(_ptr)	{ .type = SIDE_TYPE_ARRAY_U32, .u = { .side_static = { .side_array_fixint = (_ptr) } } }
#define side_arg_array_u64(_ptr)	{ .type = SIDE_TYPE_ARRAY_U64, .u = { .side_static = { .side_array_fixint = (_ptr) } } }
#define side_arg_array_s8(_ptr)		{ .type = SIDE_TYPE_ARRAY_S8, .u = { .side_static = { .side_array_fixint = (_ptr) } } }
#define side_arg_array_s16(_ptr)	{ .type = SIDE_TYPE_ARRAY_S16, .u = { .side_static = { .side_array_fixint  = (_ptr) } } }
#define side_arg_array_s32(_ptr)	{ .type = SIDE_TYPE_ARRAY_S32, .u = { .side_static = { .side_array_fixint = (_ptr) } } }
#define side_arg_array_s64(_ptr)	{ .type = SIDE_TYPE_ARRAY_S64, .u = { .side_static = { .side_array_fixint = (_ptr) } } }
#define side_arg_array_byte(_ptr)	{ .type = SIDE_TYPE_ARRAY_BYTE, .u = { .side_static = { .side_array_fixint = (_ptr) } } }
#define side_arg_array_pointer(_ptr)	{ .type = SIDE_TYPE_ARRAY_POINTER_HOST, .u = { .side_static = { .side_array_fixint = (_ptr) } } }

#define side_arg_vla_u8(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U8, .u = { .side_static = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_u16(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U16, .u = { .side_static = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } } }
#define side_arg_vla_u32(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U32, .u = { .side_static = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } } }
#define side_arg_vla_u64(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U64, .u = { .side_static = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } } }
#define side_arg_vla_s8(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S8, .u = { .side_static = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } } }
#define side_arg_vla_s16(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S16, .u = { .side_static = { .side_vla_fixint  = { .p = (_ptr), .length = (_length) } } } }
#define side_arg_vla_s32(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S32, .u = { .side_static = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } } }
#define side_arg_vla_s64(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S64, .u = { .side_static = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } } }
#define side_arg_vla_byte(_ptr, _length) { .type = SIDE_TYPE_VLA_BYTE, .u = { .side_static = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } } }
#define side_arg_vla_pointer(_ptr, _length) { .type = SIDE_TYPE_VLA_POINTER_HOST, .u = { .side_static = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } } }

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
					}, \
					.value = !!(_val), \
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
#define side_arg_dynamic_string(_val, _attr) \
	{ \
		.type = SIDE_TYPE_DYNAMIC_STRING, \
		.u = { \
			.side_dynamic = { \
				.side_string = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
					}, \
					.value = (uintptr_t) (_val), \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_integer(_field, _val, _type, _signedness, _byte_order, _integer_size_bits, _len_bits, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_dynamic = { \
				.side_integer = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
						.integer_size_bits = _integer_size_bits, \
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
	_side_arg_dynamic_integer(.side_u8, _val, SIDE_TYPE_DYNAMIC_U8, false, SIDE_TYPE_BYTE_ORDER_HOST, 8, 8, SIDE_PARAM(_attr))
#define side_arg_dynamic_s8(_val, _attr) \
	_side_arg_dynamic_integer(.side_s8, _val, SIDE_TYPE_DYNAMIC_S8, true, SIDE_TYPE_BYTE_ORDER_HOST, 8, 8, SIDE_PARAM(_attr))

#define _side_arg_dynamic_u16(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_u16, _val, SIDE_TYPE_DYNAMIC_U16, false, _byte_order, 16, 16, SIDE_PARAM(_attr))
#define _side_arg_dynamic_u32(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_u32, _val, SIDE_TYPE_DYNAMIC_U32, false, _byte_order, 32, 32, SIDE_PARAM(_attr))
#define _side_arg_dynamic_u64(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_u64, _val, SIDE_TYPE_DYNAMIC_U64, false, _byte_order, 64, 64, SIDE_PARAM(_attr))

#define _side_arg_dynamic_s16(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_s16, _val, SIDE_TYPE_DYNAMIC_S16, true, _byte_order, 16, 16, SIDE_PARAM(_attr))
#define _side_arg_dynamic_s32(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_s32, _val, SIDE_TYPE_DYNAMIC_S32, true, _byte_order, 32, 32, SIDE_PARAM(_attr))
#define _side_arg_dynamic_s64(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(.side_s64, _val, SIDE_TYPE_DYNAMIC_S64, true, _byte_order, 64, 64, SIDE_PARAM(_attr))

#define _side_arg_dynamic_pointer(_val, _byte_order, _attr) \
	_side_arg_dynamic_integer(SIDE_PTR_HOST, (uintptr_t) (_val), SIDE_TYPE_DYNAMIC_POINTER_HOST, false, _byte_order, \
			SIDE_BITS_PER_LONG, SIDE_BITS_PER_LONG, SIDE_PARAM(_attr))

#define _side_arg_dynamic_float(_field, _val, _type, _byte_order, _float_size_bits, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_dynamic = { \
				.side_float = { \
					.type = { \
						.attr = _attr, \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
						.byte_order = _byte_order, \
						.float_size_bits = _float_size_bits, \
					}, \
					.value = { \
						_field = (_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_float_binary16(_val, _byte_order, _attr) \
	_side_arg_dynamic_float(.side_float_binary16, _val, SIDE_TYPE_DYNAMIC_FLOAT_BINARY16, _byte_order, 16, SIDE_PARAM(_attr))
#define _side_arg_dynamic_float_binary32(_val, _byte_order, _attr) \
	_side_arg_dynamic_float(.side_float_binary32, _val, SIDE_TYPE_DYNAMIC_FLOAT_BINARY32, _byte_order, 32, SIDE_PARAM(_attr))
#define _side_arg_dynamic_float_binary64(_val, _byte_order, _attr) \
	_side_arg_dynamic_float(.side_float_binary64, _val, SIDE_TYPE_DYNAMIC_FLOAT_BINARY64, _byte_order, 64, SIDE_PARAM(_attr))
#define _side_arg_dynamic_float_binary128(_val, _byte_order, _attr) \
	_side_arg_dynamic_float(.side_float_binary128, _val, SIDE_TYPE_DYNAMIC_FLOAT_BINARY128, _byte_order, 128, SIDE_PARAM(_attr))

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
	const struct side_arg_dynamic_event_field _identifier##_fields[] = { _struct_fields }; \
	const struct side_arg_dynamic_event_struct _identifier = { \
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

#define side_arg_list(...)	__VA_ARGS__

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
		.label = _label, \
	}

#define side_enum_mapping_value(_label, _value) \
	{ \
		.range_begin = _value, \
		.range_end = _value, \
		.label = _label, \
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
		.label = _label, \
	}

#define side_enum_bitmap_mapping_value(_label, _value) \
	{ \
		.range_begin = _value, \
		.range_end = _value, \
		.label = _label, \
	}

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
		const struct side_arg_dynamic_event_field side_fields[] = { _var_fields }; \
		const struct side_arg_dynamic_event_struct var_struct = { \
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
	_side_define_event(__attribute__((visibility("hidden"))), _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_PARAM(_attr), 0)

#define side_hidden_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(__attribute__((visibility("hidden"))), _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_PARAM(_attr), SIDE_EVENT_FLAG_VARIADIC)

#define side_export_event(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(__attribute__((visibility("default"))), _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_PARAM(_attr), 0)

#define side_export_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(__attribute__((visibility("default"))), _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_PARAM(_attr), SIDE_EVENT_FLAG_VARIADIC)

#define side_declare_event(_identifier) \
	extern uintptr_t side_event_enable_##_identifier; \
	extern struct side_event_description _identifier

extern const struct side_callback side_empty_callback;

void side_call(const struct side_event_description *desc,
	const struct side_arg_vec *side_arg_vec);
void side_call_variadic(const struct side_event_description *desc,
	const struct side_arg_vec *side_arg_vec,
	const struct side_arg_dynamic_event_struct *var_struct);

int side_tracer_callback_register(struct side_event_description *desc,
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			void *priv),
		void *priv);
int side_tracer_callback_variadic_register(struct side_event_description *desc,
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_event_struct *var_struct,
			void *priv),
		void *priv);
int side_tracer_callback_unregister(struct side_event_description *desc,
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			void *priv),
		void *priv);
int side_tracer_callback_variadic_unregister(struct side_event_description *desc,
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_event_struct *var_struct,
			void *priv),
		void *priv);

struct side_events_register_handle *side_events_register(struct side_event_description **events, uint32_t nr_events);
void side_events_unregister(struct side_events_register_handle *handle);

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

void side_init(void);
void side_exit(void);

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

#endif /* _SIDE_TRACE_H */
