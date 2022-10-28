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

/* SIDE stands for "Static Instrumentation Dynamically Enabled" */

//TODO: as those structures will be ABI, we need to either consider them
//fixed forever, or think of a scheme that would allow their binary
//representation to be extended if need be.

struct side_arg_vec;
struct side_arg_vec_description;
struct side_arg_dynamic_vec;
struct side_arg_dynamic_vec_vla;
struct side_type_description;
struct side_event_field;
struct side_tracer_visitor_ctx;
struct side_tracer_dynamic_struct_visitor_ctx;
struct side_tracer_dynamic_vla_visitor_ctx;
struct side_event_description;
struct side_arg_dynamic_event_struct;
struct side_events_register_handle;

enum side_type {
	/* Basic types */
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
	SIDE_TYPE_FLOAT_BINARY16,
	SIDE_TYPE_FLOAT_BINARY32,
	SIDE_TYPE_FLOAT_BINARY64,
	SIDE_TYPE_FLOAT_BINARY128,
	SIDE_TYPE_STRING,

	/* Compound types */
	SIDE_TYPE_STRUCT,
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

	SIDE_TYPE_VLA_U8,
	SIDE_TYPE_VLA_U16,
	SIDE_TYPE_VLA_U32,
	SIDE_TYPE_VLA_U64,
	SIDE_TYPE_VLA_S8,
	SIDE_TYPE_VLA_S16,
	SIDE_TYPE_VLA_S32,
	SIDE_TYPE_VLA_S64,
	SIDE_TYPE_VLA_BYTE,

	/* Enumeration types */
	SIDE_TYPE_ENUM,
	SIDE_TYPE_ENUM_BITMAP,

	/* Dynamic type */
	SIDE_TYPE_DYNAMIC,
};

enum side_dynamic_type {
	/* Basic types */
	SIDE_DYNAMIC_TYPE_NULL,
	SIDE_DYNAMIC_TYPE_BOOL,
	SIDE_DYNAMIC_TYPE_U8,
	SIDE_DYNAMIC_TYPE_U16,
	SIDE_DYNAMIC_TYPE_U32,
	SIDE_DYNAMIC_TYPE_U64,
	SIDE_DYNAMIC_TYPE_S8,
	SIDE_DYNAMIC_TYPE_S16,
	SIDE_DYNAMIC_TYPE_S32,
	SIDE_DYNAMIC_TYPE_S64,
	SIDE_DYNAMIC_TYPE_BYTE,
	SIDE_DYNAMIC_TYPE_FLOAT_BINARY16,
	SIDE_DYNAMIC_TYPE_FLOAT_BINARY32,
	SIDE_DYNAMIC_TYPE_FLOAT_BINARY64,
	SIDE_DYNAMIC_TYPE_FLOAT_BINARY128,
	SIDE_DYNAMIC_TYPE_STRING,

	/* Compound types */
	SIDE_DYNAMIC_TYPE_STRUCT,
	SIDE_DYNAMIC_TYPE_STRUCT_VISITOR,
	SIDE_DYNAMIC_TYPE_VLA,
	SIDE_DYNAMIC_TYPE_VLA_VISITOR,
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

typedef enum side_visitor_status (*side_visitor)(
		const struct side_tracer_visitor_ctx *tracer_ctx,
		void *app_ctx);
typedef enum side_visitor_status (*side_dynamic_struct_visitor)(
		const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
		void *app_ctx);
typedef enum side_visitor_status (*side_dynamic_vla_visitor)(
		const struct side_tracer_dynamic_vla_visitor_ctx *tracer_ctx,
		void *app_ctx);

struct side_attr_value {
	uint32_t type;	/* enum side_attr_type */
	union {
		uint8_t side_bool;
		uint8_t side_u8;
		uint16_t side_u16;
		uint32_t side_u32;
		uint64_t side_u64;
		int8_t side_s8;
		int16_t side_s16;
		int32_t side_s32;
		int64_t side_s64;
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
		const char *string;
	} u;
};

/* User attributes. */
struct side_attr {
	const char *key;
	const struct side_attr_value value;
};

struct side_enum_mapping {
	int64_t range_begin;
	int64_t range_end;
	const char *label;
};

struct side_enum_mappings {
	const struct side_enum_mapping *mappings;
	const struct side_attr *attr;
	uint32_t nr_mappings;
	uint32_t nr_attr;
};

struct side_enum_bitmap_mapping {
	uint64_t range_begin;
	uint64_t range_end;
	const char *label;
};

struct side_enum_bitmap_mappings {
	const struct side_enum_bitmap_mapping *mappings;
	const struct side_attr *attr;
	uint32_t nr_mappings;
	uint32_t nr_attr;
};

struct side_type_struct {
	uint32_t nr_fields;
	uint32_t nr_attr;
	const struct side_event_field *fields;
	const struct side_attr *attr;
};

struct side_type_description {
	uint32_t type;	/* enum side_type */
	union {
		/* Basic types */
		struct {
			const struct side_attr *attr;
			uint32_t nr_attr;
		} side_basic;

		/* Compound types */
		struct {
			const struct side_type_description *elem_type;
			const struct side_attr *attr;
			uint32_t length;
			uint32_t nr_attr;
		} side_array;
		struct {
			const struct side_type_description *elem_type;
			const struct side_attr *attr;
			uint32_t nr_attr;
		} side_vla;
		struct {
			const struct side_type_description *elem_type;
			side_visitor visitor;
			const struct side_attr *attr;
			uint32_t nr_attr;
		} side_vla_visitor;
		const struct side_type_struct *side_struct;

		/* Enumeration types */
		struct {
			const struct side_enum_mappings *mappings;
			const struct side_type_description *elem_type;
		} side_enum;
		struct {
			const struct side_enum_bitmap_mappings *mappings;
			const struct side_type_description *elem_type;
		} side_enum_bitmap;
	} u;
};

struct side_event_field {
	const char *field_name;
	struct side_type_description side_type;
};

enum side_event_flags {
	SIDE_EVENT_FLAG_VARIADIC = (1 << 0),
};

struct side_callback {
	union {
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
			void *priv);
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
			const struct side_arg_dynamic_event_struct *var_struct,
			void *priv);
	} u;
	void *priv;
};

struct side_event_description {
	uint32_t version;
	uint32_t *enabled;
	uint32_t loglevel;	/* enum side_loglevel */
	uint32_t nr_fields;
	uint32_t nr_attr;
	uint32_t _unused;
	uint64_t flags;
	const char *provider_name;
	const char *event_name;
	const struct side_event_field *fields;
	const struct side_attr *attr;
	const struct side_callback *callbacks;
};

struct side_arg_dynamic_vec {
	uint32_t dynamic_type;	/* enum side_dynamic_type */
	union {
		/* Basic types */
		struct {
			const struct side_attr *attr;
			uint32_t nr_attr;
			union {
				uint8_t side_bool;
				uint8_t side_u8;
				uint16_t side_u16;
				uint32_t side_u32;
				uint64_t side_u64;
				int8_t side_s8;
				int16_t side_s16;
				int32_t side_s32;
				int64_t side_s64;
				uint8_t side_byte;
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
				const char *string;
			} u;
		} side_basic;

		/* Compound types */
		const struct side_arg_dynamic_event_struct *side_dynamic_struct;
		struct {
			void *app_ctx;
			side_dynamic_struct_visitor visitor;
			const struct side_attr *attr;
			uint32_t nr_attr;
		} side_dynamic_struct_visitor;
		const struct side_arg_dynamic_vec_vla *side_dynamic_vla;
		struct {
			void *app_ctx;
			side_dynamic_vla_visitor visitor;
			const struct side_attr *attr;
			uint32_t nr_attr;
		} side_dynamic_vla_visitor;
	} u;
};

struct side_arg_dynamic_vec_vla {
	const struct side_arg_dynamic_vec *sav;
	const struct side_attr *attr;
	uint32_t len;
	uint32_t nr_attr;
};

struct side_arg_dynamic_event_field {
	const char *field_name;
	const struct side_arg_dynamic_vec elem;
};

struct side_arg_dynamic_event_struct {
	const struct side_arg_dynamic_event_field *fields;
	const struct side_attr *attr;
	uint32_t len;
	uint32_t nr_attr;
};

struct side_arg_vec {
	enum side_type type;
	union {
		/* Basic types */
		uint8_t side_bool;
		uint8_t side_u8;
		uint16_t side_u16;
		uint32_t side_u32;
		uint64_t side_u64;
		int8_t side_s8;
		int16_t side_s16;
		int32_t side_s32;
		int64_t side_s64;
		uint8_t side_byte;
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
		const char *string;

		/* Compound types */
		const struct side_arg_vec_description *side_struct;
		const struct side_arg_vec_description *side_array;
		const struct side_arg_vec_description *side_vla;
		void *side_vla_app_visitor_ctx;
		void *side_array_fixint;
		struct {
			void *p;
			uint32_t length;
		} side_vla_fixint;

		/* Dynamic type */
		struct side_arg_dynamic_vec dynamic;
	} u;
};

struct side_arg_vec_description {
	const struct side_arg_vec *sav;
	uint32_t len;
};

/* The visitor pattern is a double-dispatch visitor. */
struct side_tracer_visitor_ctx {
	enum side_visitor_status (*write_elem)(
			const struct side_tracer_visitor_ctx *tracer_ctx,
			const struct side_arg_vec *elem);
	void *priv;		/* Private tracer context. */
};

struct side_tracer_dynamic_struct_visitor_ctx {
	enum side_visitor_status (*write_field)(
			const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
			const struct side_arg_dynamic_event_field *dynamic_field);
	void *priv;		/* Private tracer context. */
};

struct side_tracer_dynamic_vla_visitor_ctx {
	enum side_visitor_status (*write_elem)(
			const struct side_tracer_dynamic_vla_visitor_ctx *tracer_ctx,
			const struct side_arg_dynamic_vec *elem);
	void *priv;		/* Private tracer context. */
};

/* Event and type attributes */

#define side_attr(_key, _value)	\
	{ \
		.key = _key, \
		.value = SIDE_PARAM(_value), \
	}

#define side_attr_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_attr, __VA_ARGS__)

#define side_attr_null(_val)		{ .type = SIDE_ATTR_TYPE_NULL }
#define side_attr_bool(_val)		{ .type = SIDE_ATTR_TYPE_BOOL, .u = { .side_bool = !!(_val) } }
#define side_attr_u8(_val)		{ .type = SIDE_ATTR_TYPE_U8, .u = { .side_u8 = (_val) } }
#define side_attr_u16(_val)		{ .type = SIDE_ATTR_TYPE_U16, .u = { .side_u16 = (_val) } }
#define side_attr_u32(_val)		{ .type = SIDE_ATTR_TYPE_U32, .u = { .side_u32 = (_val) } }
#define side_attr_u64(_val)		{ .type = SIDE_ATTR_TYPE_U64, .u = { .side_u64 = (_val) } }
#define side_attr_s8(_val)		{ .type = SIDE_ATTR_TYPE_S8, .u = { .side_s8 = (_val) } }
#define side_attr_s16(_val)		{ .type = SIDE_ATTR_TYPE_S16, .u = { .side_s16 = (_val) } }
#define side_attr_s32(_val)		{ .type = SIDE_ATTR_TYPE_S32, .u = { .side_s32 = (_val) } }
#define side_attr_s64(_val)		{ .type = SIDE_ATTR_TYPE_S64, .u = { .side_s64 = (_val) } }
#define side_attr_float_binary16(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY16, .u = { .side_float_binary16 = (_val) } }
#define side_attr_float_binary32(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY32, .u = { .side_float_binary32 = (_val) } }
#define side_attr_float_binary64(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY64, .u = { .side_float_binary64 = (_val) } }
#define side_attr_float_binary128(_val)	{ .type = SIDE_ATTR_TYPE_FLOAT_BINARY128, .u = { .side_float_binary128 = (_val) } }
#define side_attr_string(_val)		{ .type = SIDE_ATTR_TYPE_STRING, .u = { .string = (_val) } }

/* Static field definition */

#define _side_type_basic(_type, _attr) \
	{ \
		.type = _type, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}

#define side_type_bool(_attr)				_side_type_basic(SIDE_TYPE_BOOL, SIDE_PARAM(_attr))
#define side_type_u8(_attr)				_side_type_basic(SIDE_TYPE_U8, SIDE_PARAM(_attr))
#define side_type_u16(_attr)				_side_type_basic(SIDE_TYPE_U16, SIDE_PARAM(_attr))
#define side_type_u32(_attr)				_side_type_basic(SIDE_TYPE_U32, SIDE_PARAM(_attr))
#define side_type_u64(_attr)				_side_type_basic(SIDE_TYPE_U64, SIDE_PARAM(_attr))
#define side_type_s8(_attr)				_side_type_basic(SIDE_TYPE_S8, SIDE_PARAM(_attr))
#define side_type_s16(_attr)				_side_type_basic(SIDE_TYPE_S16, SIDE_PARAM(_attr))
#define side_type_s32(_attr)				_side_type_basic(SIDE_TYPE_S32, SIDE_PARAM(_attr))
#define side_type_s64(_attr)				_side_type_basic(SIDE_TYPE_S64, SIDE_PARAM(_attr))
#define side_type_byte(_attr)				_side_type_basic(SIDE_TYPE_BYTE, SIDE_PARAM(_attr))
#define side_type_float_binary16(_attr)			_side_type_basic(SIDE_TYPE_FLOAT_BINARY16, SIDE_PARAM(_attr))
#define side_type_float_binary32(_attr)			_side_type_basic(SIDE_TYPE_FLOAT_BINARY32, SIDE_PARAM(_attr))
#define side_type_float_binary64(_attr)			_side_type_basic(SIDE_TYPE_FLOAT_BINARY64, SIDE_PARAM(_attr))
#define side_type_float_binary128(_attr)		_side_type_basic(SIDE_TYPE_FLOAT_BINARY128, SIDE_PARAM(_attr))
#define side_type_string(_attr)				_side_type_basic(SIDE_TYPE_STRING, SIDE_PARAM(_attr))
#define side_type_dynamic(_attr)			_side_type_basic(SIDE_TYPE_DYNAMIC, SIDE_PARAM(_attr))

#define _side_field(_name, _type) \
	{ \
		.field_name = _name, \
		.side_type = _type, \
	}

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
#define side_field_float_binary16(_name, _attr)		_side_field(_name, side_type_float_binary16(SIDE_PARAM(_attr)))
#define side_field_float_binary32(_name, _attr)		_side_field(_name, side_type_float_binary32(SIDE_PARAM(_attr)))
#define side_field_float_binary64(_name, _attr)		_side_field(_name, side_type_float_binary64(SIDE_PARAM(_attr)))
#define side_field_float_binary128(_name, _attr)	_side_field(_name, side_type_float_binary128(SIDE_PARAM(_attr)))
#define side_field_string(_name, _attr)			_side_field(_name, side_type_string(SIDE_PARAM(_attr)))
#define side_field_dynamic(_name, _attr)		_side_field(_name, side_type_dynamic(SIDE_PARAM(_attr)))

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
	SIDE_COMPOUND_LITERAL(const struct side_type_description, __VA_ARGS__)

#define side_field_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_event_field, __VA_ARGS__)

/* Static field arguments */

#define side_arg_bool(_val)		{ .type = SIDE_TYPE_BOOL, .u = { .side_bool = !!(_val) } }
#define side_arg_u8(_val)		{ .type = SIDE_TYPE_U8, .u = { .side_u8 = (_val) } }
#define side_arg_u16(_val)		{ .type = SIDE_TYPE_U16, .u = { .side_u16 = (_val) } }
#define side_arg_u32(_val)		{ .type = SIDE_TYPE_U32, .u = { .side_u32 = (_val) } }
#define side_arg_u64(_val)		{ .type = SIDE_TYPE_U64, .u = { .side_u64 = (_val) } }
#define side_arg_s8(_val)		{ .type = SIDE_TYPE_S8, .u = { .side_s8 = (_val) } }
#define side_arg_s16(_val)		{ .type = SIDE_TYPE_S16, .u = { .side_s16 = (_val) } }
#define side_arg_s32(_val)		{ .type = SIDE_TYPE_S32, .u = { .side_s32 = (_val) } }
#define side_arg_s64(_val)		{ .type = SIDE_TYPE_S64, .u = { .side_s64 = (_val) } }
#define side_arg_byte(_val)		{ .type = SIDE_TYPE_BYTE, .u = { .side_byte = (_val) } }
#define side_arg_enum_bitmap8(_val)	{ .type = SIDE_TYPE_ENUM_BITMAP8, .u = { .side_u8 = (_val) } }
#define side_arg_enum_bitmap16(_val)	{ .type = SIDE_TYPE_ENUM_BITMAP16, .u = { .side_u16 = (_val) } }
#define side_arg_enum_bitmap32(_val)	{ .type = SIDE_TYPE_ENUM_BITMAP32, .u = { .side_u32 = (_val) } }
#define side_arg_enum_bitmap64(_val)	{ .type = SIDE_TYPE_ENUM_BITMAP64, .u = { .side_u64 = (_val) } }
#define side_arg_enum_bitmap_array(_side_type)	{ .type = SIDE_TYPE_ENUM_BITMAP_ARRAY, .u = { .side_array = (_side_type) } }
#define side_arg_enum_bitmap_vla(_side_type)	{ .type = SIDE_TYPE_ENUM_BITMAP_VLA, .u = { .side_vla = (_side_type) } }
#define side_arg_float_binary16(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY16, .u = { .side_float_binary16 = (_val) } }
#define side_arg_float_binary32(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY32, .u = { .side_float_binary32 = (_val) } }
#define side_arg_float_binary64(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY64, .u = { .side_float_binary64 = (_val) } }
#define side_arg_float_binary128(_val)	{ .type = SIDE_TYPE_FLOAT_BINARY128, .u = { .side_float_binary128 = (_val) } }

#define side_arg_string(_val)		{ .type = SIDE_TYPE_STRING, .u = { .string = (_val) } }
#define side_arg_struct(_side_type)	{ .type = SIDE_TYPE_STRUCT, .u = { .side_struct = (_side_type) } }
#define side_arg_array(_side_type)	{ .type = SIDE_TYPE_ARRAY, .u = { .side_array = (_side_type) } }
#define side_arg_vla(_side_type)	{ .type = SIDE_TYPE_VLA, .u = { .side_vla = (_side_type) } }
#define side_arg_vla_visitor(_ctx)	{ .type = SIDE_TYPE_VLA_VISITOR, .u = { .side_vla_app_visitor_ctx = (_ctx) } }

#define side_arg_array_u8(_ptr)		{ .type = SIDE_TYPE_ARRAY_U8, .u = { .side_array_fixint = (_ptr) } }
#define side_arg_array_u16(_ptr)	{ .type = SIDE_TYPE_ARRAY_U16, .u = { .side_array_fixint = (_ptr) } }
#define side_arg_array_u32(_ptr)	{ .type = SIDE_TYPE_ARRAY_U32, .u = { .side_array_fixint = (_ptr) } }
#define side_arg_array_u64(_ptr)	{ .type = SIDE_TYPE_ARRAY_U64, .u = { .side_array_fixint = (_ptr) } }
#define side_arg_array_s8(_ptr)		{ .type = SIDE_TYPE_ARRAY_S8, .u = { .side_array_fixint = (_ptr) } }
#define side_arg_array_s16(_ptr)	{ .type = SIDE_TYPE_ARRAY_S16, .u = { .side_array_fixint  = (_ptr) } }
#define side_arg_array_s32(_ptr)	{ .type = SIDE_TYPE_ARRAY_S32, .u = { .side_array_fixint = (_ptr) } }
#define side_arg_array_s64(_ptr)	{ .type = SIDE_TYPE_ARRAY_S64, .u = { .side_array_fixint = (_ptr) } }
#define side_arg_array_byte(_ptr)	{ .type = SIDE_TYPE_ARRAY_BYTE, .u = { .side_array_fixint = (_ptr) } }

#define side_arg_vla_u8(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U8, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } }
#define side_arg_vla_u16(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U16, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_u32(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U32, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_u64(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U64, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_s8(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S8, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_s16(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S16, .u = { .side_vla_fixint  = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_s32(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S32, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_s64(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S64, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_byte(_ptr, _length) { .type = SIDE_TYPE_VLA_BYTE, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }

#define side_arg_dynamic(_dynamic_arg_type) \
	{ \
		.type = SIDE_TYPE_DYNAMIC, \
		.u = { \
			.dynamic = _dynamic_arg_type, \
		}, \
	}

/* Dynamic field arguments */

#define side_arg_dynamic_null(_attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_NULL, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}

#define side_arg_dynamic_bool(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_BOOL, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_bool = !!(_val), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_u8(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_U8, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_u8 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_u16(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_U16, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_u16 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_u32(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_U32, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_u32 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_u64(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_U64, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_u64 = (_val), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_s8(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_S8, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_s8 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_s16(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_S16, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_s16 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_s32(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_S32, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_s32 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_s64(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_S64, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_s64 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_byte(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_BYTE, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_byte = (_val), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_float_binary16(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_FLOAT_BINARY16, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_float_binary16 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_float_binary32(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_FLOAT_BINARY32, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_float_binary32 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_float_binary64(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_FLOAT_BINARY64, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_float_binary64 = (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_float_binary128(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_FLOAT_BINARY128, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.side_float_binary128 = (_val), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_string(_val, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_STRING, \
		.u = { \
			.side_basic = { \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.u = { \
					.string = (_val), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_vla(_vla) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_VLA, \
		.u = { \
			.side_dynamic_vla = (_vla), \
		}, \
	}

#define side_arg_dynamic_vla_visitor(_dynamic_vla_visitor, _ctx, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_VLA_VISITOR, \
		.u = { \
			.side_dynamic_vla_visitor = { \
				.app_ctx = _ctx, \
				.visitor = _dynamic_vla_visitor, \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}

#define side_arg_dynamic_struct(_struct) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_STRUCT, \
		.u = { \
			.side_dynamic_struct = (_struct), \
		}, \
	}

#define side_arg_dynamic_struct_visitor(_dynamic_struct_visitor, _ctx, _attr) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_STRUCT_VISITOR, \
		.u = { \
			.side_dynamic_struct_visitor = { \
				.app_ctx = _ctx, \
				.visitor = _dynamic_struct_visitor, \
				.attr = _attr, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
			}, \
		}, \
	}

#define side_arg_dynamic_define_vec(_identifier, _sav, _attr) \
	const struct side_arg_dynamic_vec _identifier##_vec[] = { _sav }; \
	const struct side_arg_dynamic_vec_vla _identifier = { \
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
	const struct side_arg_vec _identifier##_vec[] = { _sav }; \
	const struct side_arg_vec_description _identifier = { \
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
		const struct side_arg_vec side_sav[] = { _sav }; \
		const struct side_arg_vec_description sav_desc = { \
			.sav = side_sav, \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		side_call(&(_identifier), &sav_desc); \
	}

#define side_event(_identifier, _sav) \
	side_event_cond(_identifier) \
		side_event_call(_identifier, SIDE_PARAM(_sav))

#define side_event_call_variadic(_identifier, _sav, _var_fields, _attr) \
	{ \
		const struct side_arg_vec side_sav[] = { _sav }; \
		const struct side_arg_vec_description sav_desc = { \
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
		side_call_variadic(&(_identifier), &sav_desc, &var_struct); \
	}

#define side_event_variadic(_identifier, _sav, _var, _attr) \
	side_event_cond(_identifier) \
		side_event_call_variadic(_identifier, SIDE_PARAM(_sav), SIDE_PARAM(_var), SIDE_PARAM(_attr))

#define _side_define_event(_linkage, _identifier, _provider, _event, _loglevel, _fields, _attr, _flags) \
	_linkage uint32_t side_event_enable__##_identifier __attribute__((section("side_event_enable"))); \
	_linkage struct side_event_description __attribute__((section("side_event_description"))) \
			_identifier = { \
		.version = 0, \
		.enabled = &(side_event_enable__##_identifier), \
		.loglevel = _loglevel, \
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		._unused = 0, \
		.flags = (_flags), \
		.provider_name = _provider, \
		.event_name = _event, \
		.fields = _fields, \
		.attr = _attr, \
		.callbacks = &side_empty_callback, \
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
	extern uint32_t side_event_enable_##_identifier; \
	extern struct side_event_description _identifier

extern const struct side_callback side_empty_callback;

void side_call(const struct side_event_description *desc,
	const struct side_arg_vec_description *sav_desc);
void side_call_variadic(const struct side_event_description *desc,
	const struct side_arg_vec_description *sav_desc,
	const struct side_arg_dynamic_event_struct *var_struct);

int side_tracer_callback_register(struct side_event_description *desc,
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
			void *priv),
		void *priv);
int side_tracer_callback_variadic_register(struct side_event_description *desc,
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
			const struct side_arg_dynamic_event_struct *var_struct,
			void *priv),
		void *priv);
int side_tracer_callback_unregister(struct side_event_description *desc,
		void (*call)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
			void *priv),
		void *priv);
int side_tracer_callback_variadic_unregister(struct side_event_description *desc,
		void (*call_variadic)(const struct side_event_description *desc,
			const struct side_arg_vec_description *sav_desc,
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

#endif /* _SIDE_TRACE_H */
