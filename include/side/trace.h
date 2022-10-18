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
#include <side/macros.h>

/* SIDE stands for "Static Instrumentation Dynamically Enabled" */

struct side_arg_vec;
struct side_arg_vec_description;
struct side_arg_dynamic_vec;
struct side_arg_dynamic_vec_vla;
struct side_type_description;
struct side_event_field;
struct side_tracer_visitor_ctx;
struct side_tracer_dynamic_struct_visitor_ctx;
struct side_tracer_dynamic_vla_visitor_ctx;

enum side_type {
	SIDE_TYPE_BOOL,

	SIDE_TYPE_U8,
	SIDE_TYPE_U16,
	SIDE_TYPE_U32,
	SIDE_TYPE_U64,
	SIDE_TYPE_S8,
	SIDE_TYPE_S16,
	SIDE_TYPE_S32,
	SIDE_TYPE_S64,

	SIDE_TYPE_STRING,

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

	SIDE_TYPE_VLA_U8,
	SIDE_TYPE_VLA_U16,
	SIDE_TYPE_VLA_U32,
	SIDE_TYPE_VLA_U64,
	SIDE_TYPE_VLA_S8,
	SIDE_TYPE_VLA_S16,
	SIDE_TYPE_VLA_S32,
	SIDE_TYPE_VLA_S64,

	SIDE_TYPE_DYNAMIC,
};

enum side_dynamic_type {
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

	SIDE_DYNAMIC_TYPE_STRING,

	SIDE_DYNAMIC_TYPE_STRUCT,
	SIDE_DYNAMIC_TYPE_STRUCT_VISITOR,

	SIDE_DYNAMIC_TYPE_VLA,
	SIDE_DYNAMIC_TYPE_VLA_VISITOR,
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

typedef enum side_visitor_status (*side_visitor)(
		const struct side_tracer_visitor_ctx *tracer_ctx,
		void *app_ctx);
typedef enum side_visitor_status (*side_dynamic_struct_visitor)(
		const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
		void *app_ctx);
typedef enum side_visitor_status (*side_dynamic_vla_visitor)(
		const struct side_tracer_dynamic_vla_visitor_ctx *tracer_ctx,
		void *app_ctx);

/* User attributes. */
struct side_attr {
	const char *key;
	const char *value;
};

struct side_type_description {
	uint32_t type;	/* enum side_type */
	uint32_t nr_attr;
	const struct side_attr *attr;
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
			side_visitor visitor;
		} side_vla_visitor;
	} u;
};

struct side_event_field {
	const char *field_name;
	struct side_type_description side_type;
};

enum side_event_flags {
	SIDE_EVENT_FLAG_VARIADIC = (1 << 0),
};

struct side_event_description {
	uint32_t version;
	uint32_t enabled;
	uint32_t loglevel;	/* enum side_loglevel */
	uint32_t nr_fields;
	uint32_t nr_attr;
	uint32_t _unused;
	uint64_t flags;
	const char *provider_name;
	const char *event_name;
	const struct side_event_field *fields;
	const struct side_attr *attr;
};

struct side_arg_dynamic_vec_vla {
	const struct side_arg_dynamic_vec *sav;
	uint32_t len;
};

struct side_arg_dynamic_vec {
	uint32_t dynamic_type;	/* enum side_dynamic_type */
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

		const char *string;

		const struct side_arg_dynamic_event_struct *side_dynamic_struct;
		struct {
			void *app_ctx;
			side_dynamic_struct_visitor visitor;
		} side_dynamic_struct_visitor;

		const struct side_arg_dynamic_vec_vla *side_dynamic_vla;
		struct {
			void *app_ctx;
			side_dynamic_vla_visitor visitor;
		} side_dynamic_vla_visitor;
	} u;
};

struct side_arg_dynamic_event_field {
	const char *field_name;
	const struct side_arg_dynamic_vec elem;
	//TODO: we should add something like a list of user attributes (namespaced strings)
};

struct side_arg_dynamic_event_struct {
	const struct side_arg_dynamic_event_field *fields;
	uint32_t len;
};

struct side_arg_vec {
	enum side_type type;
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

		const char *string;
		const struct side_arg_vec_description *side_struct;
		const struct side_arg_vec_description *side_array;
		const struct side_arg_vec_description *side_vla;
		void *side_vla_app_visitor_ctx;

		void *side_array_fixint;
		struct {
			void *p;
			uint32_t length;
		} side_vla_fixint;

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

#define side_attr(_key, _value)	\
	{ \
		.key = _key, \
		.value = _value, \
	}

#define side_attr_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_attr, __VA_ARGS__)

#define side_type_decl(_type, _attr) \
	{ \
		.type = _type, \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		.attr = _attr, \
	}

#define side_field(_name, _type, _attr) \
	{ \
		.field_name = _name, \
		.side_type = side_type_decl(_type, SIDE_PARAM(_attr)), \
	}

#define side_type_struct_decl(_fields, _attr) \
	{ \
		.type = SIDE_TYPE_STRUCT, \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		.attr = _attr, \
		.u = { \
			.side_struct = { \
				.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
				.fields = _fields, \
			}, \
		}, \
	}
#define side_field_struct(_name, _fields, _attr) \
	{ \
		.field_name = _name, \
		.side_type = side_type_struct_decl(SIDE_PARAM(_fields), SIDE_PARAM(_attr)), \
	}

#define side_type_array_decl(_elem_type, _length, _attr) \
	{ \
		.type = SIDE_TYPE_ARRAY, \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		.attr = _attr, \
		.u = { \
			.side_array = { \
				.length = _length, \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define side_field_array(_name, _elem_type, _length, _attr) \
	{ \
		.field_name = _name, \
		.side_type = side_type_array_decl(SIDE_PARAM(_elem_type), _length, SIDE_PARAM(_attr)), \
	}

#define side_type_vla_decl(_elem_type, _attr) \
	{ \
		.type = SIDE_TYPE_VLA, \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		.attr = _attr, \
		.u = { \
			.side_vla = { \
				.elem_type = _elem_type, \
			}, \
		}, \
	}
#define side_field_vla(_name, _elem_type, _attr) \
	{ \
		.field_name = _name, \
		.side_type = side_type_vla_decl(SIDE_PARAM(_elem_type), SIDE_PARAM(_attr)), \
	}

#define side_type_vla_visitor_decl(_elem_type, _visitor, _attr) \
	{ \
		.type = SIDE_TYPE_VLA_VISITOR, \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		.attr = _attr, \
		.u = { \
			.side_vla_visitor = { \
				.elem_type = SIDE_PARAM(_elem_type), \
				.visitor = _visitor, \
			}, \
		}, \
	}
#define side_field_vla_visitor(_name, _elem_type, _visitor, _attr) \
	{ \
		.field_name = _name, \
		.side_type = side_type_vla_visitor_decl(SIDE_PARAM(_elem_type), _visitor, SIDE_PARAM(_attr)), \
	}

#define side_elem(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type_description, __VA_ARGS__)

#define side_elem_type(_type, _attr) \
	side_elem(side_type_decl(_type, SIDE_PARAM(_attr)))

#define side_field_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_event_field, __VA_ARGS__)

#define side_arg_bool(val)		{ .type = SIDE_TYPE_BOOL, .u = { .side_bool = !!(val) } }
#define side_arg_u8(val)		{ .type = SIDE_TYPE_U8, .u = { .side_u8 = (val) } }
#define side_arg_u16(val)		{ .type = SIDE_TYPE_U16, .u = { .side_u16 = (val) } }
#define side_arg_u32(val)		{ .type = SIDE_TYPE_U32, .u = { .side_u32 = (val) } }
#define side_arg_u64(val)		{ .type = SIDE_TYPE_U64, .u = { .side_u64 = (val) } }
#define side_arg_s8(val)		{ .type = SIDE_TYPE_S8, .u = { .side_s8 = (val) } }
#define side_arg_s16(val)		{ .type = SIDE_TYPE_S16, .u = { .side_s16 = (val) } }
#define side_arg_s32(val)		{ .type = SIDE_TYPE_S32, .u = { .side_s32 = (val) } }
#define side_arg_s64(val)		{ .type = SIDE_TYPE_S64, .u = { .side_s64 = (val) } }
#define side_arg_string(val)		{ .type = SIDE_TYPE_STRING, .u = { .string = (val) } }
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

#define side_arg_vla_u8(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U8, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } }
#define side_arg_vla_u16(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U16, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_u32(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U32, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_u64(_ptr, _length)	{ .type = SIDE_TYPE_VLA_U64, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_s8(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S8, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_s16(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S16, .u = { .side_vla_fixint  = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_s32(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S32, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }
#define side_arg_vla_s64(_ptr, _length)	{ .type = SIDE_TYPE_VLA_S64, .u = { .side_vla_fixint = { .p = (_ptr), .length = (_length) } } }

#define side_arg_dynamic(dynamic_arg_type) \
	{ \
		.type = SIDE_TYPE_DYNAMIC, \
		.u = { \
			.dynamic = dynamic_arg_type, \
		}, \
	}

#define side_arg_dynamic_null(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_NULL }

#define side_arg_dynamic_bool(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_BOOL, .u = { .side_bool = !!(val) } }
#define side_arg_dynamic_u8(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_U8, .u = { .side_u8 = (val) } }
#define side_arg_dynamic_u16(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_U16, .u = { .side_u16 = (val) } }
#define side_arg_dynamic_u32(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_U32, .u = { .side_u32 = (val) } }
#define side_arg_dynamic_u64(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_U64, .u = { .side_u64 = (val) } }
#define side_arg_dynamic_s8(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_S8, .u = { .side_s8 = (val) } }
#define side_arg_dynamic_s16(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_S16, .u = { .side_s16 = (val) } }
#define side_arg_dynamic_s32(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_S32, .u = { .side_s32 = (val) } }
#define side_arg_dynamic_s64(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_S64, .u = { .side_s64 = (val) } }
#define side_arg_dynamic_string(val)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_STRING, .u = { .string = (val) } }

#define side_arg_dynamic_vla(_vla)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_VLA, .u = { .side_dynamic_vla = (_vla) } }
#define side_arg_dynamic_vla_visitor(_dynamic_vla_visitor, _ctx) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_VLA_VISITOR, \
		.u = { \
			.side_dynamic_vla_visitor = { \
				.app_ctx = _ctx, \
				.visitor = _dynamic_vla_visitor, \
			}, \
		}, \
	}

#define side_arg_dynamic_struct(_struct)	{ .dynamic_type = SIDE_DYNAMIC_TYPE_STRUCT, .u = { .side_dynamic_struct = (_struct) } }
#define side_arg_dynamic_struct_visitor(_dynamic_struct_visitor, _ctx) \
	{ \
		.dynamic_type = SIDE_DYNAMIC_TYPE_STRUCT_VISITOR, \
		.u = { \
			.side_dynamic_struct_visitor = { \
				.app_ctx = _ctx, \
				.visitor = _dynamic_struct_visitor, \
			}, \
		}, \
	}

#define side_arg_dynamic_define_vec(_identifier, _sav) \
	const struct side_arg_dynamic_vec _identifier##_vec[] = { _sav }; \
	const struct side_arg_dynamic_vec_vla _identifier = { \
		.sav = _identifier##_vec, \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
	}

#define side_arg_dynamic_define_struct(_identifier, _struct_fields) \
	const struct side_arg_dynamic_event_field _identifier##_fields[] = { _struct_fields }; \
	const struct side_arg_dynamic_event_struct _identifier = { \
		.fields = _identifier##_fields, \
		.len = SIDE_ARRAY_SIZE(_identifier##_fields), \
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
		side_event_call(desc, SIDE_PARAM(sav))

#define side_event_call_variadic(desc, _sav, _var_fields) \
	{ \
		const struct side_arg_vec side_sav[] = { _sav }; \
		const struct side_arg_vec_description sav_desc = { \
			.sav = side_sav, \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		const struct side_arg_dynamic_event_field side_fields[] = { _var_fields }; \
		const struct side_arg_dynamic_event_struct var_struct = { \
			.fields = side_fields, \
			.len = SIDE_ARRAY_SIZE(side_fields), \
		}; \
		tracer_call_variadic(desc, &sav_desc, &var_struct); \
	}

#define side_event_variadic(desc, sav, var) \
	side_event_cond(desc) \
		side_event_call_variadic(desc, SIDE_PARAM(sav), SIDE_PARAM(var))

#define _side_define_event(_identifier, _provider, _event, _loglevel, _fields, _attr, _flags) \
	struct side_event_description _identifier = { \
		.version = 0, \
		.enabled = 0, \
		.loglevel = _loglevel, \
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
		.flags = (_flags), \
		.provider_name = _provider, \
		.event_name = _event, \
		.fields = _fields, \
		.attr = _attr, \
	}

#define side_define_event(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(_identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_PARAM(_attr), 0)

#define side_define_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr) \
	_side_define_event(_identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_PARAM(_attr), SIDE_EVENT_FLAG_VARIADIC)

#define side_declare_event(_identifier) \
	struct side_event_description _identifier

#endif /* _SIDE_TRACE_H */
