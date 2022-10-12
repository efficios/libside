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

	SIDE_TYPE_ARRAY_U8,
	SIDE_TYPE_ARRAY_U16,
	SIDE_TYPE_ARRAY_U32,
	SIDE_TYPE_ARRAY_U64,
	SIDE_TYPE_ARRAY_S8,
	SIDE_TYPE_ARRAY_S16,
	SIDE_TYPE_ARRAY_S32,
	SIDE_TYPE_ARRAY_S64,

	//TODO:
	//specialized vla for fixed-size integers (optimization)
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

		void *side_array_fixint;
	} u;
};

struct side_arg_vec_description {
	const struct side_arg_vec *sav;
	uint32_t len;
};

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

#define side_elem(...) \
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

#define side_arg_array_u8(ptr)	{ .type = SIDE_TYPE_ARRAY_U8, .u = { .side_array_fixint = (ptr) } }
#define side_arg_array_u16(ptr)	{ .type = SIDE_TYPE_ARRAY_U16, .u = { .side_array_fixint = (ptr) } }
#define side_arg_array_u32(ptr)	{ .type = SIDE_TYPE_ARRAY_U32, .u = { .side_array_fixint = (ptr) } }
#define side_arg_array_u64(ptr)	{ .type = SIDE_TYPE_ARRAY_U64, .u = { .side_array_fixint = (ptr) } }
#define side_arg_array_s8(ptr)	{ .type = SIDE_TYPE_ARRAY_S8, .u = { .side_array_fixint = (ptr) } }
#define side_arg_array_s16(ptr)	{ .type = SIDE_TYPE_ARRAY_S16, .u = { .side_array_fixint  = (ptr) } }
#define side_arg_array_s32(ptr)	{ .type = SIDE_TYPE_ARRAY_S32, .u = { .side_array_fixint = (ptr) } }
#define side_arg_array_s64(ptr)	{ .type = SIDE_TYPE_ARRAY_S64, .u = { .side_array_fixint = (ptr) } }

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

#endif /* _SIDE_TRACE_H */
