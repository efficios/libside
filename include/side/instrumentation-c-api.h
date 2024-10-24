// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_INSTRUMENTATION_C_API_H
#define SIDE_INSTRUMENTATION_C_API_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

#include <side/abi/type-value.h>
#include <side/abi/attribute.h>
#include <side/abi/type-description.h>

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

/* Event and type attributes */

#define _side_attr(_key, _value)	\
	{ \
		.key = { \
			.p = SIDE_PTR_INIT(_key), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
		}, \
		.value = SIDE_PARAM(_value), \
	}

#define _side_attr_list(...)						\
	SIDE_COMPOUND_LITERAL(const struct side_attr, __VA_ARGS__)

/*
 * `side_dynamic_attr_list()' is actually not defined.  Instead, macro
 * dispatching is done to get the allocated array and its size independently.
 *
 * This is because C++ does not support well array compound literal that are not
 * static.  See the comment of `SIDE_DYNAMIC_COMPOUND_LITERAL'.
 *
 * Dispatching is done by concatenating a prefix that will expand to a defined
 * macro.
 */

/*
 * Dispatch `_side_allocate_dynamic_' of `side_dynamic_attr_list()'.  Returns the
 * dynamic allocation for the list of attributes.
 *
 * In C, it expands to a regular compound literal.  In C++, the compound literal
 * is copied onto a `alloca(3)' buffer, thus bypassing the the lifetime of the
 * expression.
 *
 * Because `SIDE_DYNAMIC_COMPOUND_LITERAL' uses `SIDE_CAT' internally, this
 * dispatching must be done using `SIDE_CAT2'.
 */
#define _side_allocate_dynamic_side_dynamic_attr_list(...)		\
	SIDE_DYNAMIC_COMPOUND_LITERAL(const struct side_attr, __VA_ARGS__)

/*
 * Dispatch `_side_allocate_static_' of `side_dynamic_attr_list()'.  Returns the
 * static allocation of the list of attributes.
 *
 * This variant is fine to be used in C++ if the only goal is to determine the
 * allocation size, since the lifetime of the allocation is bound to the
 * expression.
 *
 * This dispatching can be either done with `SIDE_CAT' or `SIDE_CAT2'.
 */
#define _side_allocate_static_side_dynamic_attr_list(...)			\
	SIDE_COMPOUND_LITERAL(const struct side_attr, __VA_ARGS__)

#define _side_attr_null(_val)		{ .type = SIDE_ATTR_TYPE_NULL }
#define _side_attr_bool(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_BOOL), .u = { .bool_value = !!(_val) } }
#define _side_attr_u8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U8), .u = { .integer_value = { .side_u8 = (_val) } } }
#define _side_attr_u16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U16), .u = { .integer_value = { .side_u16 = (_val) } } }
#define _side_attr_u32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U32), .u = { .integer_value = { .side_u32 = (_val) } } }
#define _side_attr_u64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U64), .u = { .integer_value = { .side_u64 = (_val) } } }
#define _side_attr_u128(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U128), .u = { .integer_value = { .side_u128 = (_val) } } }
#define _side_attr_s8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S8), .u = { .integer_value = { .side_s8 = (_val) } } }
#define _side_attr_s16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S16), .u = { .integer_value = { .side_s16 = (_val) } } }
#define _side_attr_s32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S32), .u = { .integer_value = { .side_s32 = (_val) } } }
#define _side_attr_s64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S64), .u = { .integer_value = { .side_s64 = (_val) } } }
#define _side_attr_s128(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S128), .u = { .integer_value = { .side_s128 = (_val) } } }
#define _side_attr_float_binary16(_val)	{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY16), .u = { .float_value = { .side_float_binary16 = (_val) } } }
#define _side_attr_float_binary32(_val)	{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY32), .u = { .float_value = { .side_float_binary32 = (_val) } } }
#define _side_attr_float_binary64(_val)	{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY64), .u = { .float_value = { .side_float_binary64 = (_val) } } }
#define _side_attr_float_binary128(_val)	{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY128), .u = { .float_value = { .side_float_binary128 = (_val) } } }

#define __side_attr_string(_val, _byte_order, _unit_size) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_STRING), \
		.u = { \
			.string_value = { \
				.p = SIDE_PTR_INIT(_val), \
				.unit_size = _unit_size, \
				.byte_order = SIDE_ENUM_INIT(_byte_order), \
			}, \
		}, \
	}

#define _side_attr_string(_val)		__side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t))
#define _side_attr_string16(_val)	__side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t))
#define _side_attr_string32(_val)	__side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t))

/* Stack-copy enumeration type definitions */

#define side_define_enum(_identifier, _mappings, _attr...) \
	const struct side_enum_mappings _identifier = { \
		.mappings = SIDE_PTR_INIT(_mappings), \
		.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
		.nr_mappings = SIDE_ARRAY_SIZE(SIDE_PARAM(_mappings)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
	}

#define side_enum_mapping_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_enum_mapping, __VA_ARGS__)

#define side_enum_mapping_range(_label, _begin, _end) \
	{ \
		.range_begin = _begin, \
		.range_end = _end, \
		.label = { \
			.p = SIDE_PTR_INIT(_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
		}, \
	}

#define side_enum_mapping_value(_label, _value) \
	{ \
		.range_begin = _value, \
		.range_end = _value, \
		.label = { \
			.p = SIDE_PTR_INIT(_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
		}, \
	}

#define _side_define_enum_bitmap(_identifier, _mappings, _attr...) \
	const struct side_enum_bitmap_mappings _identifier = { \
		.mappings = SIDE_PTR_INIT(_mappings), \
		.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
		.nr_mappings = SIDE_ARRAY_SIZE(SIDE_PARAM(_mappings)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
	}

#define side_enum_bitmap_mapping_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_enum_bitmap_mapping, __VA_ARGS__)

#define side_enum_bitmap_mapping_range(_label, _begin, _end) \
	{ \
		.range_begin = _begin, \
		.range_end = _end, \
		.label = { \
			.p = SIDE_PTR_INIT(_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
		}, \
	}

#define side_enum_bitmap_mapping_value(_label, _value) \
	{ \
		.range_begin = _value, \
		.range_end = _value, \
		.label = { \
			.p = SIDE_PTR_INIT(_label), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
		}, \
	}

/* Stack-copy field and type definitions */

#define _side_type_null(_attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_NULL), \
		.u = { \
			.side_null = { \
				.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
			}, \
		}, \
	}

#define _side_type_bool(_attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_BOOL), \
		.u = { \
			.side_bool = { \
				.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
				.bool_size = sizeof(uint8_t), \
				.len_bits = 0, \
				.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
			}, \
		}, \
	}

#define _side_type_byte(_attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_BYTE), \
		.u = { \
			.side_byte = { \
				.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
			}, \
		}, \
	}

#define __side_type_string(_type, _byte_order, _unit_size, _attr) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.u = { \
			.side_string = { \
				.attr = SIDE_PTR_INIT(_attr), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.unit_size = _unit_size, \
				.byte_order = SIDE_ENUM_INIT(_byte_order), \
			}, \
		}, \
	}

#define _side_type_dynamic() \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC), \
		.u = { }				   \
	}

#define _side_type_integer(_type, _signedness, _byte_order, _integer_size, _len_bits, _attr) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.u = { \
			.side_integer = { \
				.attr = SIDE_PTR_INIT(_attr), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.integer_size = _integer_size, \
				.len_bits = _len_bits, \
				.signedness = _signedness, \
				.byte_order = SIDE_ENUM_INIT(_byte_order), \
			}, \
		}, \
	}

#define __side_type_float(_type, _byte_order, _float_size, _attr) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.u = { \
			.side_float = { \
				.attr = SIDE_PTR_INIT(_attr), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
				.float_size = _float_size, \
				.byte_order = SIDE_ENUM_INIT(_byte_order), \
			}, \
		}, \
	}

#define _side_field(_name, _type) \
	{ \
		.field_name = SIDE_PTR_INIT(_name), \
		.side_type = _type, \
	}

#define _side_option_range(_range_begin, _range_end, _type) \
	{ \
		.range_begin = _range_begin, \
		.range_end = _range_end, \
		.side_type = _type, \
	}

#define _side_option(_value, _type) \
	_side_option_range(_value, _value, SIDE_PARAM(_type))

/* Host endian */
#define _side_type_u8(_attr...)				_side_type_integer(SIDE_TYPE_U8, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u16(_attr...)				_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u32(_attr...)				_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u64(_attr...)				_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u128(_attr...)			_side_type_integer(SIDE_TYPE_U128, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(unsigned __int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s8(_attr...)				_side_type_integer(SIDE_TYPE_S8, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s16(_attr...)				_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s32(_attr...)				_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s64(_attr...)				_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s128(_attr...)			_side_type_integer(SIDE_TYPE_S128, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(__int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_pointer(_attr...)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uintptr_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary16(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float16), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary32(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float32), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary64(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float64), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary128(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float128), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string(_attr...)			__side_type_string(SIDE_TYPE_STRING_UTF8, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string16(_attr...) 			__side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string32(_attr...)		 	__side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_null(_name, _attr...)		_side_field(_name, _side_type_null(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_bool(_name, _attr...)		_side_field(_name, _side_type_bool(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u8(_name, _attr...)			_side_field(_name, _side_type_u8(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u16(_name, _attr...)			_side_field(_name, _side_type_u16(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u32(_name, _attr...)			_side_field(_name, _side_type_u32(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u64(_name, _attr...)			_side_field(_name, _side_type_u64(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u128(_name, _attr...)		_side_field(_name, _side_type_u128(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s8(_name, _attr...)			_side_field(_name, _side_type_s8(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s16(_name, _attr...)			_side_field(_name, _side_type_s16(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s32(_name, _attr...)			_side_field(_name, _side_type_s32(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s64(_name, _attr...)			_side_field(_name, _side_type_s64(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s128(_name, _attr...)		_side_field(_name, _side_type_s128(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_byte(_name, _attr...)		_side_field(_name, _side_type_byte(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_pointer(_name, _attr...)		_side_field(_name, _side_type_pointer(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary16(_name, _attr...)	_side_field(_name, _side_type_float_binary16(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary32(_name, _attr...)	_side_field(_name, _side_type_float_binary32(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary64(_name, _attr...)	_side_field(_name, _side_type_float_binary64(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary128(_name, _attr...)	_side_field(_name, _side_type_float_binary128(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string(_name, _attr...)		_side_field(_name, _side_type_string(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string16(_name, _attr...)		_side_field(_name, _side_type_string16(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string32(_name, _attr...)		_side_field(_name, _side_type_string32(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_dynamic(_name)			_side_field(_name, _side_type_dynamic())

/* C native types. */

/*
 * The SIDE ABI specifies fixed sizes integers and floating points.  However, as
 * a convenience for C/C++, the SIDE C API supports C native types (e.g. char)
 * which are translated to their equivalent.
 *
 * Note that the translation of C * native types is toolchain dependent and
 * therefore could produce different * results.
 *
 * The main use case is for auto-generating SIDE events for public API of shared
 * libraries.
 */

#ifdef __CHAR_UNSIGNED__
#  define _side_field_char(_name, _attr...) _side_field_uchar(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_char(_args...) _side_arg_uchar(_args)
#  define _side_type_char _side_type_uchar
#else
#  define _side_field_char(_name, _attr...) _side_field_schar(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_char(_args...) _side_arg_schar(_args)
#  define _side_type_char _side_type_schar
#endif

#define _side_field_schar(_name, _attr...) _side_field_s8(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_arg_schar(_args...) _side_arg_s8(_args)
#define _side_type_schar _side_type_s8

#define _side_field_uchar(_name, _attr...) _side_field_u8(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_arg_uchar(_args...) _side_arg_u8(_args)
#define _side_type_uchar _side_type_u8

#if __SIZEOF_SHORT__ <= 2
#  define _side_field_short(_name, _attr...) _side_field_s16(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_short(_args...) _side_arg_s16(_args)
#  define _side_type_short _side_type_s16
#  define _side_field_ushort(_name, _attr...) _side_field_u16(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ushort(_args...) _side_arg_u16(_args)
#  define _side_type_ushort _side_type_u16
#elif __SIZEOF_SHORT__ <= 4
#  define _side_field_short(_name, _attr...) _side_field_s32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_short(_args...) _side_arg_s32(_args)
#  define _side_type_short _side_type_s32
#  define _side_field_ushort(_name, _attr...) _side_field_u32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ushort(_args...) _side_arg_u32(_args)
#  define _side_type_ushort _side_type_u32
#elif __SIZEOF_SHORT__ <= 8
#  define _side_field_short(_name, _attr...) _side_field_s64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_short(_args...) _side_arg_s64(_args)
#  define _side_type_short _side_type_s64
#  define _side_field_ushort(_name, _attr...) _side_field_u64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ushort(_args...) _side_arg_u64(_args)
#  define _side_type_ushort _side_type_u64
#else
#  define _side_field_short(...)					\
	side_static_assert(0, "Type `signed short int' is not supported", type__signed_short_int__is_not_supported)
#  define _side_arg_short(...)					\
	side_static_assert(0, "Type `signed short int' is not supported", type__signed_short_int__is_not_supported)
#  define _side_type_short(...)					\
	side_static_assert(0, "Type `signed short int' is not supported", type__signed_short_int__is_not_supported)
#  define _side_field_ushort(...)					\
	side_static_assert(0, "Type `unsigned short int' is not supported", type__unsigned_short_int__is_not_supported)
#  define _side_arg_ushort(...)						\
	side_static_assert(0, "Type `unsigned short int' is not supported", type__unsigned_short_int__is_not_supported)
#  define _side_type_ushort(...)					\
	side_static_assert(0, "Type `unsigned short int' is not supported", type__unsigned_short_int__is_not_supported)
#endif

#if __SIZEOF_INT__ <= 2
#  define _side_field_int(_name, _attr...) _side_field_s16(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_int(_args...) _side_arg_s16(_args)
#  define _side_type_int _side_type_s16
#  define _side_field_uint(_name, _attr...) _side_field_u16(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_uint(_args...) _side_arg_u16(_args)
#  define _side_type_uint _side_type_u16
#elif __SIZEOF_INT__ <= 4
#  define _side_field_int(_name, _attr...) _side_field_s32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_int(_args...) _side_arg_s32(_args)
#  define _side_type_int _side_type_s32
#  define _side_field_uint(_name, _attr...) _side_field_u32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_uint(_args...) _side_arg_u32(_args)
#  define _side_type_uint _side_type_u32
#elif __SIZEOF_INT__ <= 8
#  define _side_field_int(_name, _attr...) _side_field_s64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_int(_args...) _side_arg_s64(_args)
#  define _side_type_int _side_type_s64
#  define _side_field_uint(_name, _attr...) _side_field_u64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_uint(_args...) _side_arg_u64(_args)
#  define _side_type_uint _side_type_u64
#else
#  define _side_field_int(...)						\
	side_static_assert(0, "Type `signed int' is not supported", type__signed_int__is_not_supported)
#  define _side_arg_int(...)						\
	side_static_assert(0, "Type `signed int' is not supported", type__signed_int__is_not_supported)
#  define _side_type_int(...)						\
	side_static_assert(0, "Type `signed int' is not supported", type__signed_int__is_not_supported)
#  define _side_field_uint(...)						\
	side_static_assert(0, "Type `unsigned int' is not supported", type__unsigned_int__is_not_supported)
#  define _side_arg_uint(...)						\
	side_static_assert(0, "Type `unsigned int' is not supported", type__unsigned_int__is_not_supported)
#  define _side_type_uint(...)						\
	side_static_assert(0, "Type `unsigned int' is not supported", type__unsigned_int__is_not_supported)
#endif

#if __SIZEOF_LONG__ <= 4
#  define _side_field_long(_name, _attr...) _side_field_s32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_long(_args...) _side_arg_s32(_args)
#  define _side_type_long _side_type_s32
#  define _side_field_ulong(_name, _attr...) _side_field_u32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ulong(_args...) _side_arg_u32(_args)
#  define _side_type_ulong _side_type_u32
#elif __SIZEOF_LONG__ <= 8
#  define _side_field_long(_name, _attr...) _side_field_s64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_long(_args...) _side_arg_s64(_args)
#  define _side_type_long _side_type_s64
#  define _side_field_ulong(_name, _attr...) _side_field_u64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ulong(_args...) _side_arg_u64(_args)
#  define _side_type_ulong _side_type_u64
#else
#  define _side_field_long(...)						\
	side_static_assert(0, "Type `signed long int' is not supported", type__signed_long_int__is_not_supported)
#  define _side_arg_long(...)					\
	side_static_assert(0, "Type `signed long int' is not supported", type__signed_long_int__is_not_supported)
#  define _side_type_long(...)					\
	side_static_assert(0, "Type `signed long int' is not supported", type__signed_long_int__is_not_supported)
#  define _side_field_ulong(...)					\
	side_static_assert(0, "Type `unsigned long int' is not supported", type__unsigned_long_int__is_not_supported)
#  define _side_arg_ulong(...)						\
	side_static_assert(0, "Type `unsigned long int' is not supported", type__unsigned_long_int__is_not_supported)
#  define _side_type_ulong(...)						\
	side_static_assert(0, "Type `unsigned long int' is not supported", type__unsigned_long_int__is_not_supported)
#endif

#if __SIZEOF_LONG_LONG__ <= 8
#  define _side_field_long_long(_name, _attr...) _side_field_s64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_long_long(_args...) _side_arg_s64(_args)
#  define _side_type_long_long _side_type_s64
#  define _side_field_ulong_long(_name, _attr...) _side_field_u64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_ulong_long(_args...) _side_arg_u64(_args)
#  define _side_type_ulong_long _side_type_u64
#else
#  define _side_field_long_long(...)					\
	side_static_assert(0, "Type `signed long long int' is not supported", type__signed_long_long_int__is_not_supported)
#  define _side_arg_long_long(...)					\
	side_static_assert(0, "Type `signed long long int' is not supported", type__signed_long_long_int__is_not_supported)
#  define _side_type_long_long(...)					\
	side_static_assert(0, "Type `signed long long int' is not supported", type__signed_long_long_int__is_not_supported)
#  define _side_field_ulong_long(...)					\
	side_static_assert(0, "Type `unsigned long long int' is not supported", type__unsigned_long_long_int__is_not_supported)
#  define _side_arg_ulong_long(...)					\
	side_static_assert(0, "Type `unsigned long long int' is not supported", type__unsigned_long_long_int__is_not_supported)
#  define _side_arg_ulong_long(...)					\
	side_static_assert(0, "Type `unsigned long long int' is not supported", type__unsigned_long_long_int__is_not_supported)
#endif

#if __SIZEOF_FLOAT__ <= 4 && __HAVE_FLOAT32
#  define _side_field_float(_name, _attr...) _side_field_float_binary32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_float(_args...) _side_arg_float_binary32(_args)
#  define _side_type_float _side_type_float_binary32
#elif __SIZEOF_FLOAT__ <= 8 && __HAVE_FLOAT64
#  define _side_field_float(_name, _attr...) _side_field_float_binary64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_float(_args...) _side_arg_float_binary64(_args)
#  define _side_type_float _side_type_float_binary64
#elif __SIZEOF_FLOAT__ <= 16 && __HAVE_FLOAT128
#  define _side_field_float(_name, _attr...) _side_field_float_binary128(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_float(_args...) _side_arg_float_binary128(_args)
#  define _side_type_float _side_type_float_binary128
#else
#  define _side_field_float(...)					\
	side_static_assert(0, "Type `float' is not supported", type__float__is_not_supported)
#  define _side_arg_float(...)					\
	side_static_assert(0, "Type `float' is not supported", type__float__is_not_supported)
#  define _side_type_float(...)					\
	side_static_assert(0, "Type `float' is not supported", type__float__is_not_supported)
#endif

#if __SIZEOF_DOUBLE__ <= 4 && __HAVE_FLOAT32
#  define _side_field_double(_name, _attr...) _side_field_float_binary32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_double(_args...) _side_arg_float_binary32(_args)
#  define _side_type_double _side_type_float_binary32
#elif __SIZEOF_DOUBLE__ <= 8 && __HAVE_FLOAT64
#  define _side_field_double(_name, _attr...) _side_field_float_binary64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_double(_args...) _side_arg_float_binary64(_args)
#  define _side_type_double _side_type_float_binary64
#elif __SIZEOF_DOUBLE__ <= 16 && __HAVE_FLOAT128
#  define _side_field_double(_name, _attr...) _side_field_double_binary128(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#  define _side_arg_double(_args...) _side_arg_float_binary128(_args)
#  define _side_type_double _side_type_float_binary128
#else
#  define _side_field_double(...)					\
	side_static_assert(0, "Type `double' is not supported", type__double__is_not_supported)
#  define _side_arg_double(...)						\
	side_static_assert(0, "Type `double' is not supported", type__double__is_not_supported)
#  define _side_type_double(...)					\
	side_static_assert(0, "Type `double' is not supported", type__double__is_not_supported)
#endif

#ifdef __SIZEOF_LONG_DOUBLE__
#  if __SIZEOF_LONG_DOUBLE__ <= 4 && __HAVE_FLOAT32
#    define _side_field_long_double(_name, _attr...) _side_field_float_binary32(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#    define _side_arg_long_double(_args...) _side_arg_float_binary32(_args)
#    define _side_type_long_double _side_type_float_binary32
#  elif __SIZEOF_LONG_DOUBLE__ <= 8 && __HAVE_FLOAT64
#    define _side_field_long_double(_name, _attr...) _side_field_float_binary64(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#    define _side_arg_long_double(_args...) _side_arg_float_binary64(_args)
#    define _side_type_long_double _side_type_float_binary64
#  elif __SIZEOF_LONG_DOUBLE__ <= 16 && __HAVE_FLOAT128
#    define _side_field_long_double(_name, _attr...) _side_field_float_binary128(_name, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#    define _side_arg_long_double(_args...) _side_arg_float_binary128(_args)
#    define _side_type_long_double _side_type_float_binary128
#  else
#    define _side_field_long_double(...)					\
	side_static_assert(0, "Type `long double' is not supported", type__long_double__is_not_supported)
#    define _side_arg_long_double(...)					\
	side_static_assert(0, "Type `long double' is not supported", type__long_double__is_not_supported)
#    define _side_type_long_double(...)					\
	side_static_assert(0, "Type `long double' is not supported", type__long_double__is_not_supported)
#  endif
#endif	/* __SIZEOF_LONG_DOUBLE__ */

/* Little endian */
#define _side_type_u16_le(_attr...)			_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u32_le(_attr...)			_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u64_le(_attr...)			_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u128_le(_attr...)			_side_type_integer(SIDE_TYPE_U128, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(unsigned __int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s16_le(_attr...)			_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s32_le(_attr...)			_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s64_le(_attr...)			_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s128_le(_attr...)			_side_type_integer(SIDE_TYPE_S128, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(__int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_pointer_le(_attr...)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uintptr_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary16_le(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float16), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary32_le(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float32), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary64_le(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float64), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary128_le(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float128), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string16_le(_attr...) 		__side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string32_le(_attr...)		 	__side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_u16_le(_name, _attr...)		_side_field(_name, _side_type_u16_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u32_le(_name, _attr...)		_side_field(_name, _side_type_u32_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u64_le(_name, _attr...)		_side_field(_name, _side_type_u64_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u128_le(_name, _attr...)		_side_field(_name, _side_type_u128_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s16_le(_name, _attr...)		_side_field(_name, _side_type_s16_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s32_le(_name, _attr...)		_side_field(_name, _side_type_s32_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s64_le(_name, _attr...)		_side_field(_name, _side_type_s64_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s128_le(_name, _attr...)		_side_field(_name, _side_type_s128_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_pointer_le(_name, _attr...)		_side_field(_name, _side_type_pointer_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary16_le(_name, _attr...)	_side_field(_name, _side_type_float_binary16_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary32_le(_name, _attr...)	_side_field(_name, _side_type_float_binary32_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary64_le(_name, _attr...)	_side_field(_name, _side_type_float_binary64_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary128_le(_name, _attr...)	_side_field(_name, _side_type_float_binary128_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string16_le(_name, _attr...)		_side_field(_name, _side_type_string16_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string32_le(_name, _attr...)		_side_field(_name, _side_type_string32_le(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

/* Big endian */
#define _side_type_u16_be(_attr...)			_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u32_be(_attr...)			_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u64_be(_attr...)			_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_u128_be(_attr...)			_side_type_integer(SIDE_TYPE_U128, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(unsigned __int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s16_be(_attr...)			_side_type_integer(SIDE_TYPE_S16, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s32_be(_attr...)			_side_type_integer(SIDE_TYPE_S32, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s64_be(_attr...)			_side_type_integer(SIDE_TYPE_S64, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_s128_be(_attr...)			_side_type_integer(SIDE_TYPE_S128, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(__int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_pointer_be(_attr...)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uintptr_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary16_be(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float16), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary32_be(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float32), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary64_be(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float64), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_float_binary128_be(_attr...)		__side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float128), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string16_be(_attr...) 		__side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_string32_be(_attr...)		 	__side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_u16_be(_name, _attr...)		_side_field(_name, _side_type_u16_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u32_be(_name, _attr...)		_side_field(_name, _side_type_u32_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u64_be(_name, _attr...)		_side_field(_name, _side_type_u64_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_u128_be(_name, _attr...)		_side_field(_name, _side_type_u128_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s16_be(_name, _attr...)		_side_field(_name, _side_type_s16_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s32_be(_name, _attr...)		_side_field(_name, _side_type_s32_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s64_be(_name, _attr...)		_side_field(_name, _side_type_s64_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_s128_be(_name, _attr...)		_side_field(_name, _side_type_s128_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_pointer_be(_name, _attr...)		_side_field(_name, _side_type_pointer_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary16_be(_name, _attr...)	_side_field(_name, _side_type_float_binary16_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary32_be(_name, _attr...)	_side_field(_name, _side_type_float_binary32_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary64_be(_name, _attr...)	_side_field(_name, _side_type_float_binary64_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_float_binary128_be(_name, _attr...)	_side_field(_name, _side_type_float_binary128_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string16_be(_name, _attr...)		_side_field(_name, _side_type_string16_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_string32_be(_name, _attr...)		_side_field(_name, _side_type_string32_be(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_enum(_mappings, _elem_type) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_ENUM), \
		.u = { \
			.side_enum = { \
				.mappings = SIDE_PTR_INIT(_mappings), \
				.elem_type = SIDE_PTR_INIT(_elem_type), \
			}, \
		}, \
	}
#define _side_field_enum(_name, _mappings, _elem_type) \
	_side_field(_name, _side_type_enum(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define _side_type_enum_bitmap(_mappings, _elem_type) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_ENUM_BITMAP), \
		.u = { \
			.side_enum_bitmap = { \
				.mappings = SIDE_PTR_INIT(_mappings), \
				.elem_type = SIDE_PTR_INIT(_elem_type), \
			}, \
		}, \
	}
#define _side_field_enum_bitmap(_name, _mappings, _elem_type) \
	_side_field(_name, _side_type_enum_bitmap(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define _side_type_struct(_struct) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_STRUCT), \
		.u = { \
			.side_struct = SIDE_PTR_INIT(&_struct), \
		}, \
	}

#define _side_field_struct(_name, _struct) \
	_side_field(_name, _side_type_struct(SIDE_PARAM(_struct)))

#define _side_type_struct_define(_fields, _attr) \
	{ \
		.fields = SIDE_PTR_INIT(_fields), \
		.attr = SIDE_PTR_INIT(_attr), \
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
		.nr_attr  = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
	}

#define _side_define_struct(_identifier, _fields, _attr...) \
	const struct side_type_struct _identifier = _side_type_struct_define(SIDE_PARAM(_fields), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_type_variant(_variant) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VARIANT), \
		.u = { \
			.side_variant = SIDE_PTR_INIT(_variant), \
		}, \
	}

#define _side_field_variant(_name, _variant) \
	_side_field(_name, _side_type_variant(SIDE_PARAM(&_variant)))

#define _side_type_variant_define(_selector, _options, _attr)	     \
	{							     \
		.options = SIDE_PTR_INIT(_options),		     \
		.attr = SIDE_PTR_INIT(_attr),			     \
		.nr_options = SIDE_ARRAY_SIZE(SIDE_PARAM(_options)), \
		.nr_attr  = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)),	     \
		.selector = _selector,				     \
	}

#define _side_define_variant(_identifier, _selector, _options, _attr...) \
	const struct side_type_variant _identifier = \
		_side_type_variant_define(SIDE_PARAM(_selector), SIDE_PARAM(_options), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

enum {
	SIDE_OPTIONAL_DISABLED = 0,
	SIDE_OPTIONAL_ENABLED = 1,
};

#define _side_type_optional(_optional)					\
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_OPTIONAL),		\
		.u = {							\
			.side_optional = SIDE_PTR_INIT(_optional),	\
		}							\
	}

#define _side_type_optional_define(_elem_type, _attr...)		\
	{								\
		.elem_type = SIDE_PTR_INIT(_elem_type),			\
		.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())),	\
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
	}

#define _side_define_optional(_identifier, _elem_type, _attr...)	\
	const struct side_type_optional _identifier = _side_type_optional_define(SIDE_PARAM(_elem_type), \
										SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_optional(_name, _identifier)		\
	_side_field(_name, _side_type_optional(SIDE_PARAM(&(_identifier))))

#define _side_field_optional_literal(_name, _elem_type, _attr...)		\
	_side_field(_name, _side_type_optional(SIDE_COMPOUND_LITERAL(struct side_type_optional, _side_type_optional_define(SIDE_PARAM(_elem_type), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))))

#define _side_type_array(_array)				\
	{							\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_ARRAY),	\
		.u = {						\
			.side_array = SIDE_PTR_INIT(&_array)	\
		}						\
	}

#define _side_field_array(_name, _array) \
	_side_field(_name, _side_type_array(_array))

#define _side_define_array(_identifier, _elem_type, _length, _attr...)	\
	const struct side_type_array _identifier = {			\
		.elem_type = SIDE_PTR_INIT(SIDE_PARAM(_elem_type)),	\
		.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
		.length = _length,					\
		.nr_attr  = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
	}

#define _side_type_vla(_vla)					\
	{							\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VLA),		\
		.u = {						\
			.side_vla = SIDE_PTR_INIT(&_vla),	\
		},						\
	}

#define _side_field_vla(_name, _vla) \
	_side_field(_name, _side_type_vla(_vla))

#define _side_define_vla(_identifier, _elem_type, _length_type, _attr...) \
	const struct side_type_vla _identifier = {			\
		.elem_type = SIDE_PTR_INIT(SIDE_PARAM(_elem_type)),	\
		.length_type = SIDE_PTR_INIT(SIDE_PARAM(_length_type)),	\
		.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
		.nr_attr  = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
	}

#define _side_type_vla_visitor_define(_elem_type, _length_type, _visitor, _attr...) \
	{ \
		.elem_type = SIDE_PTR_INIT(_elem_type), \
		.length_type = SIDE_PTR_INIT(_length_type), \
		.visitor = SIDE_PTR_INIT(_visitor), \
		.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
	}

#define _side_type_vla_visitor(_vla_visitor) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VLA_VISITOR), \
		.u = { \
			.side_vla_visitor = SIDE_PTR_INIT(&_vla_visitor), \
		}, \
	}
#define _side_field_vla_visitor(_name, _vla_visitor) \
	_side_field(_name, _side_type_vla_visitor(SIDE_PARAM(_vla_visitor)))

/* Gather field and type definitions */

#define _side_type_gather_byte(_offset, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BYTE), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_byte = { \
						.offset = _offset, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define _side_field_gather_byte(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_byte(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define __side_type_gather_bool(_byte_order, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BOOL), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_bool = { \
						.offset = _offset, \
						.offset_bits = _offset_bits, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.bool_size = _bool_size, \
							.len_bits = _len_bits, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define _side_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	__side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_HOST, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	__side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_LE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	__side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_BE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_bool(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_bool_le(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_bool_be(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_integer(_type, _signedness, _byte_order, _offset, \
		_integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_integer = { \
						.offset = _offset, \
						.offset_bits = _offset_bits, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.integer_size = _integer_size, \
							.len_bits = _len_bits, \
							.signedness = _signedness, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_unsigned_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_signed_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_field_gather_unsigned_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_signed_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_field_gather_unsigned_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_signed_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_pointer(_offset, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_HOST, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_pointer(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_pointer(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_pointer_le(_offset, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_LE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_pointer_le(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_pointer_le(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_pointer_be(_offset, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_BE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_pointer_be(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_pointer_be(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define __side_type_gather_float(_byte_order, _offset, _float_size, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_FLOAT), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_float = { \
						.offset = _offset, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.float_size = _float_size, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_type_gather_float(_offset, _float_size, _access_mode, _attr...) \
	__side_type_gather_float(SIDE_TYPE_FLOAT_WORD_ORDER_HOST, _offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_float_le(_offset, _float_size, _access_mode, _attr...) \
	__side_type_gather_float(SIDE_TYPE_BYTE_ORDER_LE, _offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_float_be(_offset, _float_size, _access_mode, _attr...) \
	__side_type_gather_float(SIDE_TYPE_BYTE_ORDER_BE, _offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_float(_name, _offset, _float_size, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_float(_offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_float_le(_name, _offset, _float_size, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_float_le(_offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_float_be(_name, _offset, _float_size, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_float_be(_offset, _float_size, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define __side_type_gather_string(_offset, _byte_order, _unit_size, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRING), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_string = { \
						.offset = _offset, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.unit_size = _unit_size, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define _side_type_gather_string(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_field_gather_string(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_string16(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_string16_le(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_string16_be(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_string16(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string16(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_string16_le(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string16_le(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_string16_be(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string16_be(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_string32(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_string32_le(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))
#define _side_type_gather_string32_be(_offset, _access_mode, _attr...) \
	__side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_field_gather_string32(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string32(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_string32_le(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string32_le(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))
#define _side_field_gather_string32_be(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_string32_be(_offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_enum(_mappings, _elem_type) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_ENUM), \
		.u = { \
			.side_enum = { \
				.mappings = SIDE_PTR_INIT(&_mappings), \
				.elem_type = SIDE_PTR_INIT(_elem_type), \
			}, \
		}, \
	}
#define _side_field_gather_enum(_name, _mappings, _elem_type) \
	_side_field(_name, _side_type_gather_enum(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define _side_type_gather_struct(_struct_gather, _offset, _size, _access_mode) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRUCT), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_struct = { \
						.type = SIDE_PTR_INIT(&_struct_gather), \
						.offset = _offset, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.size = _size, \
					}, \
				}, \
			}, \
		}, \
	}
#define _side_field_gather_struct(_name, _struct_gather, _offset, _size, _access_mode) \
	_side_field(_name, _side_type_gather_struct(SIDE_PARAM(_struct_gather), _offset, _size, _access_mode))

#define _side_type_gather_array(_elem_type_gather, _length, _offset, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_ARRAY), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_array = { \
						.offset = _offset, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.elem_type = SIDE_PTR_INIT(_elem_type_gather), \
							.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.length = _length, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define _side_field_gather_array(_name, _elem_type, _length, _offset, _access_mode, _attr...) \
	_side_field(_name, _side_type_gather_array(SIDE_PARAM(_elem_type), _length, _offset, _access_mode, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_type_gather_vla(_elem_type_gather, _offset, _access_mode, _length_type_gather, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_VLA), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_vla = { \
						.offset = _offset, \
						.access_mode = SIDE_ENUM_INIT(_access_mode), \
						.type = { \
							.elem_type = SIDE_PTR_INIT(_elem_type_gather), \
							.length_type = SIDE_PTR_INIT(_length_type_gather), \
							.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define _side_field_gather_vla(_name, _elem_type_gather, _offset, _access_mode, _length_type_gather, _attr...) \
	_side_field(_name, _side_type_gather_vla(SIDE_PARAM(_elem_type_gather), _offset, _access_mode, SIDE_PARAM(_length_type_gather), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())))

#define _side_elem(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type, __VA_ARGS__)

#define _side_length(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type, __VA_ARGS__)

#define _side_field_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_event_field, __VA_ARGS__)

#define _side_option_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_variant_option, __VA_ARGS__)

/* Stack-copy field arguments */

#define _side_arg_null(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_NULL), .flags = 0, .u = { .side_static = {  } } }
#define _side_arg_bool(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_BOOL), .flags = 0, .u = { .side_static = { .bool_value = { .side_bool8 = !!(_val) } } } }
#define _side_arg_byte(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_BYTE), .flags = 0, .u = { .side_static = { .byte_value = (_val) } } }
#define _side_arg_string(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRING_UTF8), .flags = 0, .u = { .side_static = { .string_value = SIDE_PTR_INIT(_val) } } }
#define _side_arg_string16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRING_UTF16), .flags = 0, .u = { .side_static = { .string_value = SIDE_PTR_INIT(_val) } } }
#define _side_arg_string32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRING_UTF32), .flags = 0, .u = { .side_static = { .string_value = SIDE_PTR_INIT(_val) } } }

#define _side_arg_u8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U8), .flags = 0, .u = { .side_static = {  .integer_value = { .side_u8 = (_val) } } } }
#define _side_arg_u16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U16), .flags = 0, .u = { .side_static = { .integer_value = { .side_u16 = (_val) } } } }
#define _side_arg_u32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U32), .flags = 0, .u = { .side_static = { .integer_value = { .side_u32 = (_val) } } } }
#define _side_arg_u64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U64), .flags = 0, .u = { .side_static = { .integer_value = { .side_u64 = (_val) } } } }
#define _side_arg_u128(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U128), .flags = 0, .u = { .side_static = { .integer_value = { .side_u128 = (_val) } } } }
#define _side_arg_s8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S8), .flags = 0, .u = { .side_static = { .integer_value = { .side_s8 = (_val) } } } }
#define _side_arg_s16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S16), .flags = 0, .u = { .side_static = { .integer_value = { .side_s16 = (_val) } } } }
#define _side_arg_s32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S32), .flags = 0, .u = { .side_static = { .integer_value = { .side_s32 = (_val) } } } }
#define _side_arg_s64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S64), .flags = 0, .u = { .side_static = { .integer_value = { .side_s64 = (_val) } } } }
#define _side_arg_s128(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S128), .flags = 0, .u = { .side_static = { .integer_value = { .side_s128 = (_val) } } } }
#define _side_arg_pointer(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_POINTER), .flags = 0, .u = { .side_static = { .integer_value = { .side_uptr = (uintptr_t) (_val) } } } }
#define _side_arg_float_binary16(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY16), .flags = 0, .u = { .side_static = { .float_value = { .side_float_binary16 = (_val) } } } }
#define _side_arg_float_binary32(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY32), .flags = 0, .u = { .side_static = { .float_value = { .side_float_binary32 = (_val) } } } }
#define _side_arg_float_binary64(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY64), .flags = 0, .u = { .side_static = { .float_value = { .side_float_binary64 = (_val) } } } }
#define _side_arg_float_binary128(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY128), .flags = 0, .u = { .side_static = { .float_value = { .side_float_binary128 = (_val) } } } }

#define _side_arg_struct(_side_type)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRUCT), .flags = 0, .u = { .side_static = { .side_struct = SIDE_PTR_INIT(&_side_type) } } }

#define _side_arg_define_variant(_identifier, _selector_val, _option) \
	const struct side_arg_variant _identifier = { \
		.selector = _selector_val, \
		.option = _option, \
	}

#define _side_arg_variant(_side_variant) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VARIANT), \
		.flags = 0, \
		.u = { \
			.side_static = { \
				.side_variant = SIDE_PTR_INIT(&_side_variant), \
			}, \
		}, \
	}

#define _side_arg_define_optional(_identifier, _value, _selector)	\
	const struct side_arg_optional _identifier = {			\
		.side_static = _value,					\
		.selector = _selector,					\
	}

#define _side_arg_optional(_identifier)					\
	{								\
		.type = SIDE_ENUM_INIT(SIDE_TYPE_OPTIONAL),		\
		.flags = 0,						\
		.u = {							\
			.side_static = {				\
				.side_optional = SIDE_PTR_INIT(&(_identifier)), \
			},						\
		},							\
	}

#define _side_arg_array(_side_type)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_ARRAY), .flags = 0, .u = { .side_static = { .side_array = SIDE_PTR_INIT(&_side_type) } } }

#define _side_arg_vla(_side_type)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_VLA), .flags = 0, .u = { .side_static = { .side_vla = SIDE_PTR_INIT(&_side_type) } } }
#define _side_arg_vla_visitor(_side_vla_visitor) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VLA_VISITOR), \
		.flags = 0, \
		.u = { \
			.side_static = { \
				.side_vla_visitor = SIDE_PTR_INIT(&_side_vla_visitor), \
			 } \
		 } \
	}

#define _side_arg_define_vla_visitor(_identifier, _ctx) \
	struct side_arg_vla_visitor _identifier = { \
		.app_ctx = SIDE_PTR_INIT(_ctx), \
		.cached_arg = SIDE_PTR_INIT(NULL), \
	}

/* Gather field arguments */

#define _side_arg_gather_bool(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BOOL), .flags = 0, .u = { .side_static = { .side_bool_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_byte(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BYTE), .flags = 0, .u = { .side_static = { .side_byte_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_pointer(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_POINTER), .flags = 0, .u = { .side_static = { .side_integer_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_integer(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_INTEGER), .flags = 0, .u = { .side_static = { .side_integer_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_float(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_FLOAT), .flags = 0, .u = { .side_static = { .side_float_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_string(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRING), .flags = 0, .u = { .side_static = { .side_string_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_struct(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRUCT), .flags = 0, .u = { .side_static = { .side_struct_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_array(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_ARRAY), .flags = 0, .u = { .side_static = { .side_array_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define _side_arg_gather_vla(_ptr, _length_ptr)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_VLA), .flags = 0, .u = { .side_static = { .side_vla_gather = { .ptr = SIDE_PTR_INIT(_ptr), .length_ptr = SIDE_PTR_INIT(_length_ptr) } } } }

/* Dynamic field arguments */

#define _side_arg_dynamic_null(_attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_NULL), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_null = { \
					.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
					.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_bool(_val, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_BOOL), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_bool = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
						.bool_size = sizeof(uint8_t), \
						.len_bits = 0, \
						.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
					}, \
					.value = { \
						.side_bool8 = !!(_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_byte(_val, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_BYTE), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_byte = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
					}, \
					.value = (_val), \
				}, \
			}, \
		}, \
	}

#define __side_arg_dynamic_string(_val, _byte_order, _unit_size, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_STRING), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_string = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
						.unit_size = _unit_size, \
						.byte_order = SIDE_ENUM_INIT(_byte_order), \
					}, \
					.value = (uintptr_t) (_val), \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_string(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string16(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string16_le(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string16_be(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string32(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string32_le(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_string32_be(_val, _attr...) \
	__side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define _side_arg_dynamic_integer(_field, _val, _type, _signedness, _byte_order, _integer_size, _len_bits, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_integer = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
						.integer_size = _integer_size, \
						.len_bits = _len_bits, \
						.signedness = _signedness, \
						.byte_order = SIDE_ENUM_INIT(_byte_order), \
					}, \
					.value = { \
						_field = (_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_u8(_val, _attr...) \
	_side_arg_dynamic_integer(.side_u8, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s8(_val, _attr...) \
	_side_arg_dynamic_integer(.side_s8, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define __side_arg_dynamic_u16(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u16, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_u32(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u32, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_u64(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u64, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_u128(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u128, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(unsigned __int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define __side_arg_dynamic_s16(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s16, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int16_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_s32(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s32, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int32_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_s64(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s64, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int64_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_s128(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s128, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(__int128), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define __side_arg_dynamic_pointer(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_uptr, (uintptr_t) (_val), SIDE_TYPE_DYNAMIC_POINTER, false, _byte_order, \
			sizeof(uintptr_t), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define __side_arg_dynamic_float(_field, _val, _type, _byte_order, _float_size, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_float = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
						.float_size = _float_size, \
						.byte_order = SIDE_ENUM_INIT(_byte_order), \
					}, \
					.value = { \
						_field = (_val), \
					}, \
				}, \
			}, \
		}, \
	}

#define __side_arg_dynamic_float_binary16(_val, _byte_order, _attr...) \
	__side_arg_dynamic_float(.side_float_binary16, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float16), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_float_binary32(_val, _byte_order, _attr...) \
	__side_arg_dynamic_float(.side_float_binary32, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float32), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_float_binary64(_val, _byte_order, _attr...) \
	__side_arg_dynamic_float(.side_float_binary64, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float64), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define __side_arg_dynamic_float_binary128(_val, _byte_order, _attr...) \
	__side_arg_dynamic_float(.side_float_binary128, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float128), SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

/* Host endian */
#define _side_arg_dynamic_u16(_val, _attr...) 			__side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u32(_val, _attr...) 			__side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u64(_val, _attr...) 			__side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u128(_val, _attr...) 			__side_arg_dynamic_u128(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s16(_val, _attr...) 			__side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s32(_val, _attr...) 			__side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s64(_val, _attr...) 			__side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s128(_val, _attr...) 			__side_arg_dynamic_s128(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_pointer(_val, _attr...) 		__side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary16(_val, _attr...)		__side_arg_dynamic_float_binary16(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary32(_val, _attr...)		__side_arg_dynamic_float_binary32(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary64(_val, _attr...)		__side_arg_dynamic_float_binary64(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary128(_val, _attr...)	__side_arg_dynamic_float_binary128(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

/* Little endian */
#define _side_arg_dynamic_u16_le(_val, _attr...) 		__side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u32_le(_val, _attr...) 		__side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u64_le(_val, _attr...) 		__side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u128_le(_val, _attr...) 		__side_arg_dynamic_u128(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s16_le(_val, _attr...) 		__side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s32_le(_val, _attr...) 		__side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s64_le(_val, _attr...) 		__side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s128_le(_val, _attr...) 		__side_arg_dynamic_s128(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_pointer_le(_val, _attr...) 		__side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary16_le(_val, _attr...)	__side_arg_dynamic_float_binary16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary32_le(_val, _attr...)	__side_arg_dynamic_float_binary32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary64_le(_val, _attr...)	__side_arg_dynamic_float_binary64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary128_le(_val, _attr...)	__side_arg_dynamic_float_binary128(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

/* Big endian */
#define _side_arg_dynamic_u16_be(_val, _attr...) 		__side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u32_be(_val, _attr...) 		__side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u64_be(_val, _attr...) 		__side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_u128_be(_val, _attr...) 		_side_arg_dynamic_u128(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s16_be(_val, _attr...) 		__side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s32_be(_val, _attr...) 		__side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s64_be(_val, _attr...) 		__side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_s128_be(_val, _attr...) 		__side_arg_dynamic_s128(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_pointer_be(_val, _attr...) 		__side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary16_be(_val, _attr...)	__side_arg_dynamic_float_binary16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary32_be(_val, _attr...)	__side_arg_dynamic_float_binary32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary64_be(_val, _attr...)	__side_arg_dynamic_float_binary64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))
#define _side_arg_dynamic_float_binary128_be(_val, _attr...)	__side_arg_dynamic_float_binary128(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define _side_arg_dynamic_vla(_vla) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_VLA), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_vla = SIDE_PTR_INIT(_vla), \
			}, \
		}, \
	}

#define _side_arg_dynamic_vla_visitor(_dynamic_vla_visitor) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_VLA_VISITOR), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_vla_visitor = SIDE_PTR_INIT(&_dynamic_vla_visitor), \
			}, \
		}, \
	}

#define _side_arg_dynamic_struct(_struct) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_STRUCT), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_struct = SIDE_PTR_INIT(_struct), \
			}, \
		}, \
	}

#define _side_arg_dynamic_struct_visitor(_dynamic_struct_visitor) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_STRUCT_VISITOR), \
		.flags = 0, \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_struct_visitor = SIDE_PTR_INIT(_dynamic_struct_visitor), \
			}, \
		}, \
	}

#define _side_arg_dynamic_define_vec(_identifier, _sav, _attr...) \
	const struct side_arg _identifier##_vec[] = { _sav }; \
	const struct side_arg_dynamic_vla _identifier = { \
		.sav = SIDE_PTR_INIT(_identifier##_vec), \
		.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
	}

#define _side_arg_dynamic_define_struct(_identifier, _struct_fields, _attr...) \
	const struct side_arg_dynamic_field _identifier##_fields[] = { _struct_fields }; \
	const struct side_arg_dynamic_struct _identifier = { \
		.fields = SIDE_PTR_INIT(_identifier##_fields), \
		.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
		.len = SIDE_ARRAY_SIZE(_identifier##_fields), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
	}

#define _side_arg_dynamic_define_struct_visitor(_identifier, _dynamic_struct_visitor, _ctx, _attr...) \
	struct side_arg_dynamic_struct_visitor _identifier = { \
		.visitor = SIDE_PTR_INIT(_dynamic_struct_visitor), \
		.app_ctx = SIDE_PTR_INIT(_ctx), \
		.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
		.cached_arg = SIDE_PTR_INIT(NULL), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
	}

#define _side_arg_dynamic_define_vla_visitor(_identifier, _dynamic_vla_visitor, _ctx, _attr...) \
	struct side_arg_dynamic_vla_visitor _identifier = { \
		.visitor = SIDE_PTR_INIT(_dynamic_vla_visitor), \
		.app_ctx = SIDE_PTR_INIT(_ctx), \
		.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
		.cached_arg = SIDE_PTR_INIT(NULL), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
	}

#define _side_arg_define_vec(_identifier, _sav) \
	const struct side_arg _identifier##_vec[] = { _sav }; \
	const struct side_arg_vec _identifier = { \
		.sav = SIDE_PTR_INIT(_identifier##_vec), \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
	}

#define _side_arg_dynamic_field(_name, _elem) \
	{ \
		.field_name = SIDE_PTR_INIT(_name), \
		.elem = _elem, \
	}

/*
 * Event instrumentation description registration, runtime enabled state
 * check, and instrumentation invocation.
 */

#define _side_arg_list(...)	__VA_ARGS__

#define side_event_enabled(_identifier) \
	side_unlikely(__atomic_load_n(&side_event_state__##_identifier.enabled, \
					__ATOMIC_RELAXED))

#define _side_event(_identifier, _sav)					\
	if (side_event_enabled(_identifier))				\
		_side_event_call(side_call, _identifier, SIDE_PARAM(_sav))

#define _side_event_variadic(_identifier, _sav, _var, _attr...) \
	if (side_event_enabled(_identifier))				\
		_side_event_call_variadic(side_call_variadic, _identifier, \
					SIDE_PARAM(_sav), SIDE_PARAM(_var), \
					SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))

#define _side_event_call(_call, _identifier, _sav) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = SIDE_PTR_INIT(side_sav), \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		_call(&(side_event_state__##_identifier).parent, &side_arg_vec); \
	}

#define _side_event_call_variadic(_call, _identifier, _sav, _var_fields, _attr...) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = SIDE_PTR_INIT(side_sav), \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		const struct side_arg_dynamic_field side_fields[] = { _var_fields }; \
		const struct side_arg_dynamic_struct var_struct = { \
			.fields = SIDE_PTR_INIT(side_fields), \
			.attr = SIDE_PTR_INIT(SIDE_CAT2(_side_allocate_dynamic_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
			.len = SIDE_ARRAY_SIZE(side_fields), \
			.nr_attr = SIDE_ARRAY_SIZE(SIDE_CAT2(_side_allocate_static_, SIDE_DEFAULT_ATTR(_, ##_attr, side_dynamic_attr_list()))), \
		}; \
		_call(&(side_event_state__##_identifier.parent), &side_arg_vec, &var_struct); \
	}

#define _side_statedump_event_call(_call, _identifier, _key, _sav) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = SIDE_PTR_INIT(side_sav), \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		_call(&(side_event_state__##_identifier).parent, &side_arg_vec, _key); \
	}

#define side_statedump_event_call(_identifier, _key, _sav) \
	_side_statedump_event_call(side_statedump_call, _identifier, _key, SIDE_PARAM(_sav))

#define _side_statedump_event_call_variadic(_call, _identifier, _key, _sav, _var_fields, _attr...) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = side_ptr_init(side_sav), \
			.len = side_array_size(side_sav), \
		}; \
		const struct side_arg_dynamic_field side_fields[] = { _var_fields }; \
		const struct side_arg_dynamic_struct var_struct = { \
			.fields = side_ptr_init(side_fields), \
			.attr = side_ptr_init(_attr), \
			.len = side_array_size(side_fields), \
			.nr_attr = side_array_size(_attr), \
		}; \
		_call(&(side_event_state__##_identifier.parent), &side_arg_vec, &var_struct, _key); \
	}

#define side_statedump_event_call_variadic(_identifier, _key, _sav, _var_fields, _attr...) \
	_side_statedump_event_call_variadic(side_statedump_call_variadic, _identifier, _key, SIDE_PARAM(_sav), SIDE_PARAM(_var_fields), SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))


/*
 * The forward declaration linkage is always the same in C. In C++ however, it
 * is necessary to not use the same linkage as the declaration.
 *
 * Rationale for disabled diagnostics:
 *
 *   -Wsection:
 *      Clang complains about redeclared sections.
 */
#define _side_define_event(_forward_decl_linkage, _linkage, _identifier, _provider, _event, _loglevel, _fields, _flags, _attr...) \
	SIDE_PUSH_DIAGNOSTIC()						\
	SIDE_DIAGNOSTIC(ignored "-Wsection")				\
	_forward_decl_linkage struct side_event_description __attribute__((section("side_event_description"))) \
		_identifier;							\
	_forward_decl_linkage struct side_event_state_0 __attribute__((section("side_event_state"))) \
		side_event_state__##_identifier;				\
	_linkage struct side_event_state_0 __attribute__((section("side_event_state"))) \
		side_event_state__##_identifier = {			\
		.parent = {						\
			.version = SIDE_EVENT_STATE_ABI_VERSION,	\
		},							\
		.nr_callbacks = 0,					\
		.enabled = 0,						\
		.callbacks = (const struct side_callback *) &side_empty_callback[0], \
		.desc = &(_identifier),					\
	};								\
	_linkage struct side_event_description __attribute__((section("side_event_description"))) \
		_identifier = {						\
		.struct_size = offsetof(struct side_event_description, end), \
		.version = SIDE_EVENT_DESCRIPTION_ABI_VERSION,		\
		.state = SIDE_PTR_INIT(&(side_event_state__##_identifier.parent)), \
		.provider_name = SIDE_PTR_INIT(_provider),		\
		.event_name = SIDE_PTR_INIT(_event),			\
		.fields = SIDE_PTR_INIT(_fields),			\
		.attr = SIDE_PTR_INIT(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
		.flags = (_flags),					\
		.nr_side_type_label = _NR_SIDE_TYPE_LABEL,		\
		.nr_side_attr_type = _NR_SIDE_ATTR_TYPE,		\
		.loglevel = SIDE_ENUM_INIT(_loglevel),			\
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)),	\
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())), \
		.end = {}						\
	};								\
	static const struct side_event_description __attribute__((section("side_event_description_ptr"), used)) \
	*side_event_ptr__##_identifier = &(_identifier);		\
	SIDE_POP_DIAGNOSTIC() SIDE_ACCEPT_COMMA()

/*
 * In C++, it is not possible to forward declare a static variable.  Use
 * anonymous namespace with external linkage instead.
 */
#ifdef __cplusplus
#define _side_cxx_define_event(_namespace, _linkage, _identifier, _provider, _event, _loglevel, _fields, _flags, _attr...) \
	_namespace {							\
		_side_define_event(extern, _linkage, _identifier, _provider, _event, _loglevel, \
				SIDE_PARAM(_fields), _flags, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list())); \
	}

#define _side_static_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(namespace, , _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_static_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(namespace, , _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_EVENT_FLAG_VARIADIC, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_hidden_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(extern "C", __attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_hidden_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(extern "C", __attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_export_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(extern "C", __attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_export_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_cxx_define_event(extern "C", __attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_declare_event(_identifier)				\
	extern "C" struct side_event_description _identifier;		\
	extern "C" struct side_event_state_0 side_event_state_##_identifier
#else
#define _side_static_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(static, static, _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_static_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(static, static, _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_EVENT_FLAG_VARIADIC, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_hidden_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("hidden"))), __attribute__((visibility("hidden"))), \
			_identifier, _provider, _event,			\
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_hidden_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("hidden"))), __attribute__((visibility("hidden"))), \
			_identifier, _provider, _event,			\
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, \
			   SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_export_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("default"))), __attribute__((visibility("default"))), \
			_identifier, _provider, _event,			\
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_export_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("default"))), __attribute__((visibility("default"))), \
			_identifier, _provider, _event,			\
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, \
			   SIDE_DEFAULT_ATTR(_, ##_attr, side_attr_list()))

#define _side_declare_event(_identifier) \
	extern struct side_event_state_0 side_event_state_##_identifier; \
	extern struct side_event_description _identifier
#endif	/* __cplusplus */

#endif /* SIDE_INSTRUMENTATION_C_API_H */
