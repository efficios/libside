// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.

#ifndef _SIDE_API_H
#define _SIDE_API_H

#include <side/instrumentation-c-api.h>

/*
 * This layer defines the expansion of the libside DSL to the targeted
 * language.  In this case, the C/C++ API.
 *
 * This intermediate layer allows to redefine the DSL to something else,
 * for example when enabling the static checker.
 *
 * Most of the macros are defined with the form:
 *
 * 	#define X _X
 *
 * However, it is necessary in some cases to define `X' with the form:
 *
 * 	#define X(...) _X(__VA_ARGS__)
 *
 * because of name collisions with other definitions such as structures and
 * unions. For example, `side_arg_variant' is a macro when used as a function
 * and is struct type name when used as an object.
 */

/* Misc. */
#define side_length _side_length

/* Attributes. */
#define SIDE_DEFAULT_ATTR SIDE_PARAM_SELECT_ARG1

#define side_attr(_name, ...) _side_attr(_name, SIDE_PARAM(__VA_ARGS__))
#define side_attr_list _side_attr_list
#define side_attr_bool _side_attr_bool
#define side_attr_u8 _side_attr_u8
#define side_attr_u16 _side_attr_u16
#define side_attr_u32 _side_attr_u32
#define side_attr_u64 _side_attr_u64
#define side_attr_u128 _side_attr_u128
#define side_attr_s8 _side_attr_s8
#define side_attr_s16 _side_attr_s16
#define side_attr_s32 _side_attr_s32
#define side_attr_s64 _side_attr_s64
#define side_attr_s128 _side_attr_s128
#define side_attr_float_binary16 _side_attr_float_binary16
#define side_attr_float_binary32 _side_attr_float_binary32
#define side_attr_float_binary64 _side_attr_float_binary64
#define side_attr_float_binary128 _side_attr_float_binary128
#define side_attr_string _side_attr_string
#define side_attr_string16 _side_attr_string16
#define side_attr_string32 _side_attr_string32

/* Fields. */
#define side_field_list _side_field_list
#define side_arg_list _side_arg_list

#define side_field_null _side_field_null
#define side_arg_null _side_arg_null

#define side_field_bool _side_field_bool
#define side_arg_bool _side_arg_bool

#define side_field_byte _side_field_byte
#define side_arg_byte _side_arg_byte

#define side_field_string _side_field_string
#define side_arg_string _side_arg_string

#define side_field_string16 _side_field_string16
#define side_arg_string16 _side_arg_string16

#define side_field_string32 _side_field_string32
#define side_arg_string32 _side_arg_string32

#define side_field_string16_le _side_field_string16_le
#define side_arg_string16_le _side_arg_string16_le

#define side_field_string32_le _side_field_string32_le
#define side_arg_string32_le _side_arg_string32_le

#define side_field_string16_be _side_field_string16_be
#define side_arg_string16_be _side_arg_string16_be

#define side_field_string32_be _side_field_string32_be
#define side_arg_string32_be _side_arg_string32_be

#define side_field_pointer _side_field_pointer
#define side_arg_pointer _side_arg_pointer

#define side_field_pointer_le _side_field_pointer_le
#define side_arg_pointer_le _side_arg_pointer_le

#define side_field_pointer_be _side_field_pointer_be
#define side_arg_pointer_be _side_arg_pointer_be

#define side_field_float_binary16 _side_field_float_binary16
#define side_arg_float_binary16 _side_arg_float_binary16

#define side_field_float_binary32 _side_field_float_binary32
#define side_arg_float_binary32 _side_arg_float_binary32

#define side_field_float_binary64 _side_field_float_binary64
#define side_arg_float_binary64 _side_arg_float_binary64

#define side_field_float_binary128 _side_field_float_binary128
#define side_arg_float_binary128 _side_arg_float_binary128

#define side_field_float_binary16_le _side_field_float_binary16_le
#define side_arg_float_binary16_le _side_arg_float_binary16_le

#define side_field_float_binary32_le _side_field_float_binary32_le
#define side_arg_float_binary32_le _side_arg_float_binary32_le

#define side_field_float_binary64_le _side_field_float_binary64_le
#define side_arg_float_binary64_le _side_arg_float_binary64_le

#define side_field_float_binary128_le _side_field_float_binary128_le
#define side_arg_float_binary128_le _side_arg_float_binary128_le

#define side_field_float_binary16_be _side_field_float_binary16_be
#define side_arg_float_binary16_be _side_arg_float_binary16_be

#define side_field_float_binary32_be _side_field_float_binary32_be
#define side_arg_float_binary32_be _side_arg_float_binary32_be

#define side_field_float_binary64_be _side_field_float_binary64_be
#define side_arg_float_binary64_be _side_arg_float_binary64_be

#define side_field_float_binary128_be _side_field_float_binary128_be
#define side_arg_float_binary128_be _side_arg_float_binary128_be

#define side_field_char _side_field_char
#define side_arg_char _side_arg_char

#define side_field_uchar _side_field_uchar
#define side_arg_uchar _side_arg_uchar

#define side_field_schar _side_field_schar
#define side_arg_schar _side_arg_schar

#define side_field_short _side_field_short
#define side_arg_short _side_arg_short

#define side_field_ushort _side_field_ushort
#define side_arg_ushort _side_arg_ushort

#define side_field_int _side_field_int
#define side_arg_int _side_arg_int

#define side_field_uint _side_field_uint
#define side_arg_uint _side_arg_uint

#define side_field_long _side_field_long
#define side_arg_long _side_arg_long

#define side_field_ulong _side_field_ulong
#define side_arg_ulong _side_arg_ulong

#define side_field_long_long _side_field_long_long
#define side_arg_long_long _side_arg_long_long

#define side_field_ulong_long _side_field_ulong_long
#define side_arg_ulong_long _side_arg_ulong_long

#define side_field_float _side_field_float
#define side_arg_float _side_arg_float

#define side_field_double _side_field_double
#define side_arg_double _side_arg_double

#define side_field_long_double _side_field_long_double
#define side_arg_long_double _side_arg_long_double

#define side_field_u8 _side_field_u8
#define side_arg_u8 _side_arg_u8

#define side_field_u16 _side_field_u16
#define side_arg_u16 _side_arg_u16

#define side_field_u32 _side_field_u32
#define side_arg_u32 _side_arg_u32

#define side_field_u64 _side_field_u64
#define side_arg_u64 _side_arg_u64

#define side_field_u128 _side_field_u128
#define side_arg_u128 _side_arg_u128

#define side_field_s8 _side_field_s8
#define side_arg_s8 _side_arg_s8

#define side_field_s16 _side_field_s16
#define side_arg_s16 _side_arg_s16

#define side_field_s32 _side_field_s32
#define side_arg_s32 _side_arg_s32

#define side_field_s64 _side_field_s64
#define side_arg_s64 _side_arg_s64

#define side_field_s128 _side_field_s128
#define side_arg_s128 _side_arg_s128

#define side_field_u16_le _side_field_u16_le
#define side_arg_u16_le _side_arg_u16_le

#define side_field_u32_le _side_field_u32_le
#define side_arg_u32_le _side_arg_u32_le

#define side_field_u64_le _side_field_u64_le
#define side_arg_u64_le _side_arg_u64_le

#define side_field_u128_le _side_field_u128_le
#define side_arg_u128_le _side_arg_u128_le

#define side_field_s8_le _side_field_s8_le
#define side_arg_s8_le _side_arg_s8_le

#define side_field_s16_le _side_field_s16_le
#define side_arg_s16_le _side_arg_s16_le

#define side_field_s32_le _side_field_s32_le
#define side_arg_s32_le _side_arg_s32_le

#define side_field_s64_le _side_field_s64_le
#define side_arg_s64_le _side_arg_s64_le

#define side_field_s128_le _side_field_s128_le
#define side_arg_s128_le _side_arg_s128_le

#define side_field_u16_be _side_field_u16_be
#define side_arg_u16_be _side_arg_u16_be

#define side_field_u32_be _side_field_u32_be
#define side_arg_u32_be _side_arg_u32_be

#define side_field_u64_be _side_field_u64_be
#define side_arg_u64_be _side_arg_u64_be

#define side_field_u128_be _side_field_u128_be
#define side_arg_u128_be _side_arg_u128_be

#define side_field_s8_be _side_field_s8_be
#define side_arg_s8_be _side_arg_s8_be

#define side_field_s16_be _side_field_s16_be
#define side_arg_s16_be _side_arg_s16_be

#define side_field_s32_be _side_field_s32_be
#define side_arg_s32_be _side_arg_s32_be

#define side_field_s64_be _side_field_s64_be
#define side_arg_s64_be _side_arg_s64_be

#define side_field_s128_be _side_field_s128_be
#define side_arg_s128_be _side_arg_s128_be

/* Gather. */
#define side_field_gather_byte _side_field_gather_byte
#define side_arg_gather_byte _side_arg_gather_byte

#define side_field_gather_bool _side_field_gather_bool
#define side_field_gather_bool_le _side_field_gather_bool_le
#define side_field_gather_bool_be _side_field_gather_bool_be
#define side_arg_gather_bool _side_arg_gather_bool

#define side_field_gather_unsigned_integer _side_field_gather_unsigned_integer
#define side_field_gather_unsigned_integer_le _side_field_gather_unsigned_integer_le
#define side_field_gather_unsigned_integer_be _side_field_gather_unsigned_integer_be
#define side_field_gather_signed_integer _side_field_gather_signed_integer
#define side_field_gather_signed_integer_le _side_field_gather_signed_integer_le
#define side_field_gather_signed_integer_be _side_field_gather_signed_integer_be
#define side_arg_gather_integer _side_arg_gather_integer

#define side_field_gather_pointer _side_field_gather_pointer
#define side_field_gather_pointer_le _side_field_gather_pointer_le
#define side_field_gather_pointer_be _side_field_gather_pointer_be
#define side_arg_gather_pointer _side_arg_gather_pointer

#define side_field_gather_float _side_field_gather_float
#define side_field_gather_float_le _side_field_gather_float_le
#define side_field_gather_float_be _side_field_gather_float_be
#define side_arg_gather_float _side_arg_gather_float

#define side_field_gather_string _side_field_gather_string
#define side_field_gather_string16 _side_field_gather_string16
#define side_field_gather_string16_le _side_field_gather_string16_le
#define side_field_gather_string16_be _side_field_gather_string16_be
#define side_field_gather_string32 _side_field_gather_string32
#define side_field_gather_string32_le _side_field_gather_string32_le
#define side_field_gather_string32_be _side_field_gather_string32_be
#define side_arg_gather_string _side_arg_gather_string

#define side_field_gather_enum _side_field_gather_enum
#define side_arg_gather_enum _side_arg_gather_enum

#define side_field_gather_struct _side_field_gather_struct
#define side_arg_gather_struct _side_arg_gather_struct

#define side_field_gather_array _side_field_gather_array
#define side_arg_gather_array _side_arg_gather_array

#define side_field_gather_vla _side_field_gather_vla
#define side_arg_gather_vla _side_arg_gather_vla

/* Dynamic. */
#define side_unwrap_dynamic_field(_type, _name, _val)	\
	_side_arg_dynamic_field(_name, side_unwrap_dynamic(_type, _val))

#define side_unwrap_dynamic(_type, _val)	\
	_side_arg_dynamic_##_type(_val)

#define side_field_dynamic _side_field_dynamic

#define side_arg_dynamic_null _side_arg_dynamic_null
#define side_arg_dynamic_bool _side_arg_dynamic_bool
#define side_arg_dynamic_byte _side_arg_dynamic_byte
#define side_arg_dynamic_string _side_arg_dynamic_string
#define side_arg_dynamic_string16 _side_arg_dynamic_string16
#define side_arg_dynamic_string16_le _side_arg_dynamic_string16_le
#define side_arg_dynamic_string16_be _side_arg_dynamic_string16_be
#define side_arg_dynamic_string32 _side_arg_dynamic_string32
#define side_arg_dynamic_string32_le _side_arg_dynamic_string32_le
#define side_arg_dynamic_string32_be _side_arg_dynamic_string32_be
#define side_arg_dynamic_string _side_arg_dynamic_string
#define side_arg_dynamic_string _side_arg_dynamic_string
#define side_arg_dynamic_string _side_arg_dynamic_string
#define side_arg_dynamic_u8 _side_arg_dynamic_u8
#define side_arg_dynamic_u16 _side_arg_dynamic_u16
#define side_arg_dynamic_u32 _side_arg_dynamic_u32
#define side_arg_dynamic_u64 _side_arg_dynamic_u64
#define side_arg_dynamic_u128 _side_arg_dynamic_u128
#define side_arg_dynamic_s8 _side_arg_dynamic_s8
#define side_arg_dynamic_s16 _side_arg_dynamic_s16
#define side_arg_dynamic_s32 _side_arg_dynamic_s32
#define side_arg_dynamic_s64 _side_arg_dynamic_s64
#define side_arg_dynamic_s128 _side_arg_dynamic_s128
#define side_arg_dynamic_pointer _side_arg_dynamic_pointer
#define side_arg_dynamic_float_binary16 _side_arg_dynamic_float_binary16
#define side_arg_dynamic_float_binary32 _side_arg_dynamic_float_binary32
#define side_arg_dynamic_float_binary64 _side_arg_dynamic_float_binary64
#define side_arg_dynamic_float_binary128 _side_arg_dynamic_float_binary128
#define side_arg_dynamic_u16_le _side_arg_dynamic_u16_le
#define side_arg_dynamic_u32_le _side_arg_dynamic_u32_le
#define side_arg_dynamic_u64_le _side_arg_dynamic_u64_le
#define side_arg_dynamic_u128_le _side_arg_dynamic_u128_le
#define side_arg_dynamic_s16_le _side_arg_dynamic_s16_le
#define side_arg_dynamic_s32_le _side_arg_dynamic_s32_le
#define side_arg_dynamic_s64_le _side_arg_dynamic_s64_le
#define side_arg_dynamic_s128_le _side_arg_dynamic_s128_le
#define side_arg_dynamic_pointer_le _side_arg_dynamic_pointer_le
#define side_arg_dynamic_float_binary16_le _side_arg_dynamic_float_binary16_le
#define side_arg_dynamic_float_binary32_le _side_arg_dynamic_float_binary32_le
#define side_arg_dynamic_float_binary64_le _side_arg_dynamic_float_binary64_le
#define side_arg_dynamic_float_binary128_le _side_arg_dynamic_float_binary128_le
#define side_arg_dynamic_u16_be _side_arg_dynamic_u16_be
#define side_arg_dynamic_u32_be _side_arg_dynamic_u32_be
#define side_arg_dynamic_u64_be _side_arg_dynamic_u64_be
#define side_arg_dynamic_u128_be _side_arg_dynamic_u16
#define side_arg_dynamic_s16_be _side_arg_dynamic_s16_be
#define side_arg_dynamic_s32_be _side_arg_dynamic_s32_be
#define side_arg_dynamic_s64_be _side_arg_dynamic_s64_be
#define side_arg_dynamic_s128_be _side_arg_dynamic_s128_be
#define side_arg_dynamic_pointer_be _side_arg_dynamic_pointer_be
#define side_arg_dynamic_float_binary16_be _side_arg_dynamic_float_binary16_be
#define side_arg_dynamic_float_binary32_be _side_arg_dynamic_float_binary32_be
#define side_arg_dynamic_float_binary64_be _side_arg_dynamic_float_binary64_be
#define side_arg_dynamic_float_binary128_be _side_arg_dynamic_float_binary128_be

#define side_arg_dynamic_field(_name, _elem) _side_arg_dynamic_field(_name, SIDE_PARAM(_elem))
#define side_arg_dynamic_define_vec _side_arg_dynamic_define_vec
#define side_arg_dynamic_vla(...) _side_arg_dynamic_vla(__VA_ARGS__)

#define side_arg_dynamic_define_vla_visitor(_identifier, _dynamic_vla_visitor, _ctx, _attr...)	\
	_side_arg_dynamic_define_vla_visitor(_identifier, SIDE_PARAM(_dynamic_vla_visitor), _ctx, \
					SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define side_arg_dynamic_vla_visitor(...) _side_arg_dynamic_vla_visitor(__VA_ARGS__)

#define side_arg_dynamic_define_struct(_identifier, _struct_fields, _attr...) \
	_side_arg_dynamic_define_struct(_identifier, SIDE_PARAM(_struct_fields), \
					SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define side_arg_dynamic_struct(...) _side_arg_dynamic_struct(__VA_ARGS__)


#define side_arg_dynamic_define_struct_visitor _side_arg_dynamic_define_struct_visitor
#define side_arg_dynamic_struct_visitor(...) _side_arg_dynamic_struct_visitor(__VA_ARGS__)

/* Element. */
#define side_elem _side_elem

/* Enum. */
#define side_define_enum_bitmap _side_define_enum_bitmap
#define side_field_enum _side_field_enum
#define side_field_enum_bitmap _side_field_enum_bitmap

/* Types. */
#define side_type_null(...) _side_type_null(__VA_ARGS__)
#define side_type_bool(...) _side_type_bool(__VA_ARGS__)
#define side_type_byte(...) _side_type_byte(__VA_ARGS__)
#define side_type_string(...) _side_type_string(__VA_ARGS__)
#define side_type_pointer _side_type_pointer
#define side_type_dynamic _side_type_dynamic

#define side_type_u8 _side_type_u8
#define side_type_u16 _side_type_u16
#define side_type_u32 _side_type_u32
#define side_type_u64 _side_type_u64
#define side_type_u128 _side_type_u128
#define side_type_s8 _side_type_s8
#define side_type_s16 _side_type_s16
#define side_type_s32 _side_type_s32
#define side_type_s64 _side_type_s64
#define side_type_s128 _side_type_s128
#define side_type_float_binary16 _side_type_float_binary16
#define side_type_float_binary32 _side_type_float_binary32
#define side_type_float_binary64 _side_type_float_binary64
#define side_type_float_binary128 _side_type_float_binary128
#define side_type_string16 _side_type_string16
#define side_type_string32 _side_type_string32

#define side_type_u16_le _side_type_u16_le
#define side_type_u32_le _side_type_u32_le
#define side_type_u64_le _side_type_u64_le
#define side_type_u128_le _side_type_u128_le
#define side_type_s16_le _side_type_s16_le
#define side_type_s32_le _side_type_s32_le
#define side_type_s64_le _side_type_s64_le
#define side_type_s128_le _side_type_s128_le
#define side_type_float_binary16_le _side_type_float_binary16_le
#define side_type_float_binary32_le _side_type_float_binary32_le
#define side_type_float_binary64_le _side_type_float_binary64_le
#define side_type_float_binary128_le _side_type_float_binary128_le
#define side_type_string16_le _side_type_string16_le
#define side_type_string32_le _side_type_string32_le

#define side_type_u16_be _side_type_u16_be
#define side_type_u32_be _side_type_u32_be
#define side_type_u64_be _side_type_u64_be
#define side_type_u128_be _side_type_u128_be
#define side_type_s16_be _side_type_s16_be
#define side_type_s32_be _side_type_s32_be
#define side_type_s64_be _side_type_s64_be
#define side_type_s128_be _side_type_s128_be
#define side_type_float_binary16_be _side_type_float_binary16_be
#define side_type_float_binary32_be _side_type_float_binary32_be
#define side_type_float_binary64_be _side_type_float_binary64_be
#define side_type_float_binary128_be _side_type_float_binary128_be
#define side_type_string16_be _side_type_string16_be
#define side_type_string32_be _side_type_string32_be

#define side_type_gather_byte(...) _side_type_gather_byte(__VA_ARGS__)
#define side_type_gather_bool(...) _side_type_gather_bool(__VA_ARGS__)
#define side_type_gather_bool_le _side_type_gather_bool_le
#define side_type_gather_bool_be _side_type_gather_bool_be
#define side_type_gather_unsigned_integer _side_type_gather_unsigned_integer
#define side_type_gather_unsigned_integer_le _side_type_gather_unsigned_integer_le
#define side_type_gather_unsigned_integer_be _side_type_gather_unsigned_integer_be
#define side_type_gather_signed_integer _side_type_gather_signed_integer
#define side_type_gather_signed_integer_le _side_type_gather_signed_integer_le
#define side_type_gather_signed_integer_be _side_type_gather_signed_integer_be
#define side_type_gather_pointer _side_type_gather_pointer
#define side_type_gather_pointer_le _side_type_gather_pointer_le
#define side_type_gather_pointer_be _side_type_gather_pointer_be
#define side_type_gather_float(...) _side_type_gather_float(__VA_ARGS__)
#define side_type_gather_float_le _side_type_gather_float_le
#define side_type_gather_float_be _side_type_gather_float_be
#define side_type_gather_string(...) _side_type_gather_string(__VA_ARGS__)
#define side_type_gather_string16 _side_type_gather_string16
#define side_type_gather_string16_le _side_type_gather_string16_le
#define side_type_gather_string16_be _side_type_gather_string16_be
#define side_type_gather_string32 _side_type_gather_string32
#define side_type_gather_string32_le _side_type_gather_string32_le
#define side_type_gather_string32_be _side_type_gather_string32_be
#define side_type_gather_struct(...)  _side_type_gather_struct(__VA_ARGS__)
#define side_type_gather_array(...) _side_type_gather_array(__VA_ARGS__)
#define side_type_gather_vla(...) _side_type_gather_vla(__VA_ARGS__)

/* Variant. */
#define side_define_variant _side_define_variant
#define side_arg_define_variant _side_arg_define_variant
#define side_type_variant(_identifier) _identifier
#define side_field_variant _side_field_variant
#define	side_arg_variant(...) _side_arg_variant(__VA_ARGS__)
#define side_option_list _side_option_list
#define side_option _side_option
#define side_option_range _side_option_range

/* Optional */
#define side_define_optional _side_define_optional
#define side_type_optional(_identifier) &_identifier
#define side_field_optional _side_field_optional
#define side_field_optional_literal _side_field_optional_literal
#define side_arg_optional(_identifier) _side_arg_optional(_identifier)
#define	side_arg_define_optional _side_arg_define_optional

/* Vec. */
#define side_arg_define_vec _side_arg_define_vec

/* Array. */
#define side_arg_define_array _side_arg_define_vec
#define side_define_array _side_define_array
#define side_type_array(_array) _side_type_array(_array)
#define side_field_array _side_field_array
#define side_arg_array _side_arg_array

/* VLA. */
#define side_define_vla _side_define_vla
#define side_arg_define_vla _side_arg_define_vec
#define side_type_vla(_vla) _side_type_vla(_vla)
#define side_field_vla _side_field_vla
#define side_arg_vla _side_arg_vla

/* Struct. */
#define side_type_struct(_struct) _side_type_struct(_struct)
#define side_field_struct _side_field_struct
#define side_arg_struct _side_arg_struct
#define side_define_struct _side_define_struct
#define side_arg_define_struct _side_arg_define_vec

/* Visitor. */
#define side_visit_dynamic_arg(_what, _expr...) _what(_expr)
#define side_visit_dynamic_field(_what, _name, _expr...) _side_arg_dynamic_field(_name, _what(_expr))

#define side_define_static_vla_visitor(_identifier, _elem_type, _length_type, _func, _type, _attr...) \
	static enum side_visitor_status _side_vla_visitor_func_##_identifier(const struct side_tracer_visitor_ctx *_side_tracer_ctx, \
					     void *_side_ctx)		\
	{								\
		return _func(_side_tracer_ctx, (_type *)_side_ctx);	\
	}								\
	static const struct side_type_vla_visitor _identifier =		\
		_side_type_vla_visitor_define(SIDE_PARAM(_elem_type), SIDE_PARAM(_length_type), \
					      _side_vla_visitor_func_##_identifier, \
					      SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()));

#define side_arg_define_vla_visitor _side_arg_define_vla_visitor

#define side_type_vla_visitor(...) _side_type_vla_visitor(__VA_ARGS__)

#define side_field_vla_visitor _side_field_vla_visitor
#define side_arg_vla_visitor(...) _side_arg_vla_visitor(__VA_ARGS__)


/* Event. */
#define side_event_call(_identifier, _sav)		\
	_side_event_call(side_call, _identifier, SIDE_PARAM(_sav))

#define side_event_call_variadic(_identifier, _sav, _var_fields, _attr...) \
	_side_event_call_variadic(side_call_variadic, _identifier,	\
				  SIDE_PARAM(_sav), SIDE_PARAM(_var_fields), \
				  SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define side_event _side_event
#define side_event_variadic _side_event_variadic

#define side_static_event _side_static_event
#define side_static_event_variadic _side_static_event_variadic
#define side_hidden_event _side_hidden_event
#define side_hidden_event_variadic _side_hidden_event_variadic
#define side_export_event _side_export_event
#define side_export_event_variadic _side_export_event_variadic
#define side_declare_event _side_declare_event

#endif	/* _SIDE_API_H */
