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

#define side_attr(_key, _value)	\
	{ \
		.key = { \
			.p = SIDE_PTR_INIT(_key), \
			.unit_size = sizeof(uint8_t), \
			.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
		}, \
		.value = SIDE_PARAM(_value), \
	}

#define side_attr_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_attr, __VA_ARGS__)

#define side_attr_null(_val)		{ .type = SIDE_ATTR_TYPE_NULL }
#define side_attr_bool(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_BOOL), .u = { .bool_value = !!(_val) } }
#define side_attr_u8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U8), .u = { .integer_value = { .side_u8 = (_val) } } }
#define side_attr_u16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U16), .u = { .integer_value = { .side_u16 = (_val) } } }
#define side_attr_u32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U32), .u = { .integer_value = { .side_u32 = (_val) } } }
#define side_attr_u64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_U64), .u = { .integer_value = { .side_u64 = (_val) } } }
#define side_attr_s8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S8), .u = { .integer_value = { .side_s8 = (_val) } } }
#define side_attr_s16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S16), .u = { .integer_value = { .side_s16 = (_val) } } }
#define side_attr_s32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S32), .u = { .integer_value = { .side_s32 = (_val) } } }
#define side_attr_s64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_S64), .u = { .integer_value = { .side_s64 = (_val) } } }
#define side_attr_float_binary16(_val)	{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY16), .u = { .float_value = { .side_float_binary16 = (_val) } } }
#define side_attr_float_binary32(_val)	{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY32), .u = { .float_value = { .side_float_binary32 = (_val) } } }
#define side_attr_float_binary64(_val)	{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY64), .u = { .float_value = { .side_float_binary64 = (_val) } } }
#define side_attr_float_binary128(_val)	{ .type = SIDE_ENUM_INIT(SIDE_ATTR_TYPE_FLOAT_BINARY128), .u = { .float_value = { .side_float_binary128 = (_val) } } }

#define _side_attr_string(_val, _byte_order, _unit_size) \
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

#define side_attr_string(_val)		_side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t))
#define side_attr_string16(_val)	_side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t))
#define side_attr_string32(_val)	_side_attr_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t))

/* Stack-copy enumeration type definitions */

#define side_define_enum(_identifier, _mappings, _attr...) \
	const struct side_enum_mappings _identifier = { \
		.mappings = SIDE_PTR_INIT(_mappings), \
		.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
		.nr_mappings = SIDE_ARRAY_SIZE(SIDE_PARAM(_mappings)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
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

#define side_define_enum_bitmap(_identifier, _mappings, _attr...) \
	const struct side_enum_bitmap_mappings _identifier = { \
		.mappings = SIDE_PTR_INIT(_mappings), \
		.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
		.nr_mappings = SIDE_ARRAY_SIZE(SIDE_PARAM(_mappings)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
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

#define side_type_null(_attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_NULL), \
		.u = { \
			.side_null = { \
				.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
			}, \
		}, \
	}

#define side_type_bool(_attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_BOOL), \
		.u = { \
			.side_bool = { \
				.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				.bool_size = sizeof(uint8_t), \
				.len_bits = 0, \
				.byte_order = SIDE_ENUM_INIT(SIDE_TYPE_BYTE_ORDER_HOST), \
			}, \
		}, \
	}

#define side_type_byte(_attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_BYTE), \
		.u = { \
			.side_byte = { \
				.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
			}, \
		}, \
	}

#define _side_type_string(_type, _byte_order, _unit_size, _attr) \
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

#define side_type_dynamic() \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC), \
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

#define _side_type_float(_type, _byte_order, _float_size, _attr) \
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

#define side_option_range(_range_begin, _range_end, _type) \
	{ \
		.range_begin = _range_begin, \
		.range_end = _range_end, \
		.side_type = _type, \
	}

#define side_option(_value, _type) \
	side_option_range(_value, _value, SIDE_PARAM(_type))

/* Host endian */
#define side_type_u8(_attr...)				_side_type_integer(SIDE_TYPE_U8, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_u16(_attr...)				_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_u32(_attr...)				_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_u64(_attr...)				_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint64_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s8(_attr...)				_side_type_integer(SIDE_TYPE_S8, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s16(_attr...)				_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int16_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s32(_attr...)				_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int32_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s64(_attr...)				_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int64_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_pointer(_attr...)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uintptr_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary16(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float16), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary32(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float32), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary64(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float64), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary128(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, sizeof(_Float128), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_string(_attr...)			_side_type_string(SIDE_TYPE_STRING_UTF8, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_string16(_attr...) 			_side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_string32(_attr...)		 	_side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_field_null(_name, _attr...)		_side_field(_name, side_type_null(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_bool(_name, _attr...)		_side_field(_name, side_type_bool(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_u8(_name, _attr...)			_side_field(_name, side_type_u8(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_u16(_name, _attr...)			_side_field(_name, side_type_u16(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_u32(_name, _attr...)			_side_field(_name, side_type_u32(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_u64(_name, _attr...)			_side_field(_name, side_type_u64(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s8(_name, _attr...)			_side_field(_name, side_type_s8(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s16(_name, _attr...)			_side_field(_name, side_type_s16(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s32(_name, _attr...)			_side_field(_name, side_type_s32(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s64(_name, _attr...)			_side_field(_name, side_type_s64(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_byte(_name, _attr...)		_side_field(_name, side_type_byte(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_pointer(_name, _attr...)		_side_field(_name, side_type_pointer(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary16(_name, _attr...)	_side_field(_name, side_type_float_binary16(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary32(_name, _attr...)	_side_field(_name, side_type_float_binary32(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary64(_name, _attr...)	_side_field(_name, side_type_float_binary64(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary128(_name, _attr...)	_side_field(_name, side_type_float_binary128(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_string(_name, _attr...)		_side_field(_name, side_type_string(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_string16(_name, _attr...)		_side_field(_name, side_type_string16(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_string32(_name, _attr...)		_side_field(_name, side_type_string32(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_dynamic(_name)			_side_field(_name, side_type_dynamic())

/* Little endian */
#define side_type_u16_le(_attr...)			_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_u32_le(_attr...)			_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_u64_le(_attr...)			_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint64_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s16_le(_attr...)			_side_type_integer(SIDE_TYPE_S16, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int16_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s32_le(_attr...)			_side_type_integer(SIDE_TYPE_S32, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int32_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s64_le(_attr...)			_side_type_integer(SIDE_TYPE_S64, true, SIDE_TYPE_BYTE_ORDER_LE, sizeof(int64_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_pointer_le(_attr...)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uintptr_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary16_le(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float16), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary32_le(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float32), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary64_le(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float64), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary128_le(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_LE, sizeof(_Float128), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_string16_le(_attr...) 		_side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_string32_le(_attr...)		 	_side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_field_u16_le(_name, _attr...)		_side_field(_name, side_type_u16_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_u32_le(_name, _attr...)		_side_field(_name, side_type_u32_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_u64_le(_name, _attr...)		_side_field(_name, side_type_u64_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s16_le(_name, _attr...)		_side_field(_name, side_type_s16_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s32_le(_name, _attr...)		_side_field(_name, side_type_s32_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s64_le(_name, _attr...)		_side_field(_name, side_type_s64_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_pointer_le(_name, _attr...)		_side_field(_name, side_type_pointer_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary16_le(_name, _attr...)	_side_field(_name, side_type_float_binary16_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary32_le(_name, _attr...)	_side_field(_name, side_type_float_binary32_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary64_le(_name, _attr...)	_side_field(_name, side_type_float_binary64_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary128_le(_name, _attr...)	_side_field(_name, side_type_float_binary128_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_string16_le(_name, _attr...)		_side_field(_name, side_type_string16_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_string32_le(_name, _attr...)		_side_field(_name, side_type_string32_le(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

/* Big endian */
#define side_type_u16_be(_attr...)			_side_type_integer(SIDE_TYPE_U16, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_u32_be(_attr...)			_side_type_integer(SIDE_TYPE_U32, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_u64_be(_attr...)			_side_type_integer(SIDE_TYPE_U64, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint64_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s16_be(_attr...)			_side_type_integer(SIDE_TYPE_S16, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int16_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s32_be(_attr...)			_side_type_integer(SIDE_TYPE_S32, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int32_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_s64_be(_attr...)			_side_type_integer(SIDE_TYPE_S64, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(int64_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_pointer_be(_attr...)			_side_type_integer(SIDE_TYPE_POINTER, false, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uintptr_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary16_be(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY16, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float16), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary32_be(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY32, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float32), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary64_be(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY64, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float64), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_float_binary128_be(_attr...)		_side_type_float(SIDE_TYPE_FLOAT_BINARY128, SIDE_TYPE_BYTE_ORDER_BE, sizeof(_Float128), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_string16_be(_attr...) 		_side_type_string(SIDE_TYPE_STRING_UTF16, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_string32_be(_attr...)		 	_side_type_string(SIDE_TYPE_STRING_UTF32, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_field_u16_be(_name, _attr...)		_side_field(_name, side_type_u16_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_u32_be(_name, _attr...)		_side_field(_name, side_type_u32_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_u64_be(_name, _attr...)		_side_field(_name, side_type_u64_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s16_be(_name, _attr...)		_side_field(_name, side_type_s16_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s32_be(_name, _attr...)		_side_field(_name, side_type_s32_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_s64_be(_name, _attr...)		_side_field(_name, side_type_s64_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_pointer_be(_name, _attr...)		_side_field(_name, side_type_pointer_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary16_be(_name, _attr...)	_side_field(_name, side_type_float_binary16_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary32_be(_name, _attr...)	_side_field(_name, side_type_float_binary32_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary64_be(_name, _attr...)	_side_field(_name, side_type_float_binary64_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_float_binary128_be(_name, _attr...)	_side_field(_name, side_type_float_binary128_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_string16_be(_name, _attr...)		_side_field(_name, side_type_string16_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_string32_be(_name, _attr...)		_side_field(_name, side_type_string32_be(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_enum(_mappings, _elem_type) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_ENUM), \
		.u = { \
			.side_enum = { \
				.mappings = SIDE_PTR_INIT(_mappings), \
				.elem_type = SIDE_PTR_INIT(_elem_type), \
			}, \
		}, \
	}
#define side_field_enum(_name, _mappings, _elem_type) \
	_side_field(_name, side_type_enum(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define side_type_enum_bitmap(_mappings, _elem_type) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_ENUM_BITMAP), \
		.u = { \
			.side_enum_bitmap = { \
				.mappings = SIDE_PTR_INIT(_mappings), \
				.elem_type = SIDE_PTR_INIT(_elem_type), \
			}, \
		}, \
	}
#define side_field_enum_bitmap(_name, _mappings, _elem_type) \
	_side_field(_name, side_type_enum_bitmap(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define side_type_struct(_struct) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_STRUCT), \
		.u = { \
			.side_struct = SIDE_PTR_INIT(_struct), \
		}, \
	}
#define side_field_struct(_name, _struct) \
	_side_field(_name, side_type_struct(SIDE_PARAM(_struct)))

#define _side_type_struct_define(_fields, _attr) \
	{ \
		.fields = SIDE_PTR_INIT(_fields), \
		.attr = SIDE_PTR_INIT(_attr), \
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
		.nr_attr  = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
	}

#define side_define_struct(_identifier, _fields, _attr...) \
	const struct side_type_struct _identifier = _side_type_struct_define(SIDE_PARAM(_fields), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_struct_literal(_fields, _attr...) \
	SIDE_COMPOUND_LITERAL(const struct side_type_struct, \
		_side_type_struct_define(SIDE_PARAM(_fields), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_variant(_variant) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VARIANT), \
		.u = { \
			.side_variant = SIDE_PTR_INIT(_variant), \
		}, \
	}
#define side_field_variant(_name, _variant) \
	_side_field(_name, side_type_variant(SIDE_PARAM(_variant)))

#define _side_type_variant_define(_selector, _options, _attr) \
	{ \
		.selector = _selector, \
		.options = SIDE_PTR_INIT(_options), \
		.attr = SIDE_PTR_INIT(_attr), \
		.nr_options = SIDE_ARRAY_SIZE(SIDE_PARAM(_options)), \
		.nr_attr  = SIDE_ARRAY_SIZE(SIDE_PARAM(_attr)), \
	}

#define side_define_variant(_identifier, _selector, _options, _attr...) \
	const struct side_type_variant _identifier = \
		_side_type_variant_define(SIDE_PARAM(_selector), SIDE_PARAM(_options), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_variant_literal(_selector, _options, _attr...) \
	SIDE_COMPOUND_LITERAL(const struct side_type_variant, \
		_side_type_variant_define(SIDE_PARAM(_selector), SIDE_PARAM(_options), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_array(_elem_type, _length, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_ARRAY), \
		.u = { \
			.side_array = { \
				.elem_type = SIDE_PTR_INIT(_elem_type), \
				.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				.length = _length, \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
			}, \
		}, \
	}
#define side_field_array(_name, _elem_type, _length, _attr...) \
	_side_field(_name, side_type_array(SIDE_PARAM(_elem_type), _length, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_vla(_elem_type, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VLA), \
		.u = { \
			.side_vla = { \
				.elem_type = SIDE_PTR_INIT(_elem_type), \
				.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
			}, \
		}, \
	}
#define side_field_vla(_name, _elem_type, _attr...) \
	_side_field(_name, side_type_vla(SIDE_PARAM(_elem_type), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_vla_visitor(_elem_type, _visitor, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VLA_VISITOR), \
		.u = { \
			.side_vla_visitor = { \
				.elem_type = SIDE_PTR_INIT(_elem_type), \
				.visitor = SIDE_PTR_INIT(_visitor), \
				.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
			}, \
		}, \
	}
#define side_field_vla_visitor(_name, _elem_type, _visitor, _attr...) \
	_side_field(_name, side_type_vla_visitor(SIDE_PARAM(_elem_type), _visitor, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

/* Gather field and type definitions */

#define side_type_gather_byte(_offset, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BYTE), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_byte = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_field_gather_byte(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_byte(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define _side_type_gather_bool(_byte_order, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BOOL), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_bool = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.bool_size = _bool_size, \
							.len_bits = _len_bits, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
						.offset_bits = _offset_bits, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_HOST, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_LE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_bool(SIDE_TYPE_BYTE_ORDER_BE, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_field_gather_bool(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_bool(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))))
#define side_field_gather_bool_le(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_bool_le(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))))
#define side_field_gather_bool_be(_name, _offset, _bool_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_bool_be(_offset, _bool_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))))

#define _side_type_gather_integer(_type, _signedness, _byte_order, _offset, \
		_integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_integer = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.integer_size = _integer_size, \
							.len_bits = _len_bits, \
							.signedness = _signedness, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
						.offset_bits = _offset_bits, \
					}, \
				}, \
			}, \
		}, \
	}

#define side_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_HOST, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_LE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, false,  SIDE_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_INTEGER, true, SIDE_TYPE_BYTE_ORDER_BE, \
			_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_field_gather_unsigned_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_unsigned_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_gather_signed_integer(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_signed_integer(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_field_gather_unsigned_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_unsigned_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_gather_signed_integer_le(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_signed_integer_le(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_field_gather_unsigned_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_unsigned_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_gather_signed_integer_be(_name, _integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_signed_integer_be(_integer_offset, _integer_size, _offset_bits, _len_bits, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_gather_pointer(_offset, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_HOST, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_field_gather_pointer(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_pointer(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_gather_pointer_le(_offset, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_LE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_field_gather_pointer_le(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_pointer_le(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_gather_pointer_be(_offset, _access_mode, _attr...) \
	_side_type_gather_integer(SIDE_TYPE_GATHER_POINTER, false, SIDE_TYPE_BYTE_ORDER_BE, \
			_offset, sizeof(uintptr_t), 0, 0, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_field_gather_pointer_be(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_pointer_be(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define _side_type_gather_float(_byte_order, _offset, _float_size, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_FLOAT), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_float = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.float_size = _float_size, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}

#define side_type_gather_float(_offset, _float_size, _access_mode, _attr...) \
	_side_type_gather_float(SIDE_TYPE_FLOAT_WORD_ORDER_HOST, _offset, _float_size, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_float_le(_offset, _float_size, _access_mode, _attr...) \
	_side_type_gather_float(SIDE_TYPE_BYTE_ORDER_LE, _offset, _float_size, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_float_be(_offset, _float_size, _access_mode, _attr...) \
	_side_type_gather_float(SIDE_TYPE_BYTE_ORDER_BE, _offset, _float_size, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_field_gather_float(_name, _offset, _float_size, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_float(_offset, _float_size, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_gather_float_le(_name, _offset, _float_size, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_float_le(_offset, _float_size, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_gather_float_be(_name, _offset, _float_size, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_float_be(_offset, _float_size, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define _side_type_gather_string(_offset, _byte_order, _unit_size, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRING), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_string = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.unit_size = _unit_size, \
							.byte_order = SIDE_ENUM_INIT(_byte_order), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_type_gather_string(_offset, _access_mode, _attr...) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_field_gather_string(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_string(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_gather_string16(_offset, _access_mode, _attr...) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_string16_le(_offset, _access_mode, _attr...) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_string16_be(_offset, _access_mode, _attr...) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_field_gather_string16(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_string16(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_gather_string16_le(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_string16_le(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_gather_string16_be(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_string16_be(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_gather_string32(_offset, _access_mode, _attr...) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_string32_le(_offset, _access_mode, _attr...) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_type_gather_string32_be(_offset, _access_mode, _attr...) \
	_side_type_gather_string(_offset, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_field_gather_string32(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_string32(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_gather_string32_le(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_string32_le(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))
#define side_field_gather_string32_be(_name, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_string32_be(_offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_gather_enum(_mappings, _elem_type) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_ENUM), \
		.u = { \
			.side_enum = { \
				.mappings = SIDE_PTR_INIT(_mappings), \
				.elem_type = SIDE_PTR_INIT(_elem_type), \
			}, \
		}, \
	}
#define side_field_gather_enum(_name, _mappings, _elem_type) \
	_side_field(_name, side_type_gather_enum(SIDE_PARAM(_mappings), SIDE_PARAM(_elem_type)))

#define side_type_gather_struct(_struct_gather, _offset, _size, _access_mode) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRUCT), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_struct = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = SIDE_PTR_INIT(_struct_gather), \
						.size = _size, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_field_gather_struct(_name, _struct_gather, _offset, _size, _access_mode) \
	_side_field(_name, side_type_gather_struct(SIDE_PARAM(_struct_gather), _offset, _size, _access_mode))

#define side_type_gather_array(_elem_type_gather, _length, _offset, _access_mode, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_ARRAY), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_array = { \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.elem_type = SIDE_PTR_INIT(_elem_type_gather), \
							.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.length = _length, \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_field_gather_array(_name, _elem_type, _length, _offset, _access_mode, _attr...) \
	_side_field(_name, side_type_gather_array(SIDE_PARAM(_elem_type), _length, _offset, _access_mode, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_type_gather_vla(_elem_type_gather, _offset, _access_mode, _length_type_gather, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_VLA), \
		.u = { \
			.side_gather = { \
				.u = { \
					.side_vla = { \
						.length_type = SIDE_PTR_INIT(_length_type_gather), \
						.offset = _offset, \
						.access_mode = _access_mode, \
						.type = { \
							.elem_type = SIDE_PTR_INIT(_elem_type_gather), \
							.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
							.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
						}, \
					}, \
				}, \
			}, \
		}, \
	}
#define side_field_gather_vla(_name, _elem_type_gather, _offset, _access_mode, _length_type_gather, _attr...) \
	_side_field(_name, side_type_gather_vla(SIDE_PARAM(_elem_type_gather), _offset, _access_mode, SIDE_PARAM(_length_type_gather), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())))

#define side_elem(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type, __VA_ARGS__)

#define side_length(...) \
	SIDE_COMPOUND_LITERAL(const struct side_type, __VA_ARGS__)

#define side_field_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_event_field, __VA_ARGS__)

#define side_option_list(...) \
	SIDE_COMPOUND_LITERAL(const struct side_variant_option, __VA_ARGS__)

/* Stack-copy field arguments */

#define side_arg_null(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_NULL) }
#define side_arg_bool(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_BOOL), .u = { .side_static = { .bool_value = { .side_bool8 = !!(_val) } } } }
#define side_arg_byte(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_BYTE), .u = { .side_static = { .byte_value = (_val) } } }
#define side_arg_string(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRING_UTF8), .u = { .side_static = { .string_value = SIDE_PTR_INIT(_val) } } }
#define side_arg_string16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRING_UTF16), .u = { .side_static = { .string_value = SIDE_PTR_INIT(_val) } } }
#define side_arg_string32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRING_UTF32), .u = { .side_static = { .string_value = SIDE_PTR_INIT(_val) } } }

#define side_arg_u8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U8), .u = { .side_static = {  .integer_value = { .side_u8 = (_val) } } } }
#define side_arg_u16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U16), .u = { .side_static = { .integer_value = { .side_u16 = (_val) } } } }
#define side_arg_u32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U32), .u = { .side_static = { .integer_value = { .side_u32 = (_val) } } } }
#define side_arg_u64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_U64), .u = { .side_static = { .integer_value = { .side_u64 = (_val) } } } }
#define side_arg_s8(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S8), .u = { .side_static = { .integer_value = { .side_s8 = (_val) } } } }
#define side_arg_s16(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S16), .u = { .side_static = { .integer_value = { .side_s16 = (_val) } } } }
#define side_arg_s32(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S32), .u = { .side_static = { .integer_value = { .side_s32 = (_val) } } } }
#define side_arg_s64(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_S64), .u = { .side_static = { .integer_value = { .side_s64 = (_val) } } } }
#define side_arg_pointer(_val)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_POINTER), .u = { .side_static = { .integer_value = { .side_uptr = (uintptr_t) (_val) } } } }
#define side_arg_float_binary16(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY16), .u = { .side_static = { .float_value = { .side_float_binary16 = (_val) } } } }
#define side_arg_float_binary32(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY32), .u = { .side_static = { .float_value = { .side_float_binary32 = (_val) } } } }
#define side_arg_float_binary64(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY64), .u = { .side_static = { .float_value = { .side_float_binary64 = (_val) } } } }
#define side_arg_float_binary128(_val)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_FLOAT_BINARY128), .u = { .side_static = { .float_value = { .side_float_binary128 = (_val) } } } }

#define side_arg_struct(_side_type)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_STRUCT), .u = { .side_static = { .side_struct = SIDE_PTR_INIT(_side_type) } } }

#define side_arg_define_variant(_identifier, _selector_val, _option) \
	const struct side_arg_variant _identifier = { \
		.selector = _selector_val, \
		.option = _option, \
	}
#define side_arg_variant(_side_variant) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_VARIANT), \
		.u = { \
			.side_static = { \
				.side_variant = SIDE_PTR_INIT(_side_variant), \
			}, \
		}, \
	}

#define side_arg_array(_side_type)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_ARRAY), .u = { .side_static = { .side_array = SIDE_PTR_INIT(_side_type) } } }
#define side_arg_vla(_side_type)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_VLA), .u = { .side_static = { .side_vla = SIDE_PTR_INIT(_side_type) } } }
#define side_arg_vla_visitor(_ctx)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_VLA_VISITOR), .u = { .side_static = { .side_vla_app_visitor_ctx = (_ctx) } } }

/* Gather field arguments */

#define side_arg_gather_bool(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BOOL), .u = { .side_static = { .side_bool_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define side_arg_gather_byte(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_BYTE), .u = { .side_static = { .side_byte_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define side_arg_gather_pointer(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_POINTER), .u = { .side_static = { .side_integer_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define side_arg_gather_integer(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_INTEGER), .u = { .side_static = { .side_integer_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define side_arg_gather_float(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_FLOAT), .u = { .side_static = { .side_float_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define side_arg_gather_string(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRING), .u = { .side_static = { .side_string_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define side_arg_gather_struct(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_STRUCT), .u = { .side_static = { .side_struct_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define side_arg_gather_array(_ptr)		{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_ARRAY), .u = { .side_static = { .side_array_gather_ptr = SIDE_PTR_INIT(_ptr) } } }
#define side_arg_gather_vla(_ptr, _length_ptr)	{ .type = SIDE_ENUM_INIT(SIDE_TYPE_GATHER_VLA), .u = { .side_static = { .side_vla_gather = { .ptr = SIDE_PTR_INIT(_ptr), .length_ptr = SIDE_PTR_INIT(_length_ptr) } } } }

/* Dynamic field arguments */

#define side_arg_dynamic_null(_attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_NULL), \
		.u = { \
			.side_dynamic = { \
				.side_null = { \
					.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
					.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_bool(_val, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_BOOL), \
		.u = { \
			.side_dynamic = { \
				.side_bool = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
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

#define side_arg_dynamic_byte(_val, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_BYTE), \
		.u = { \
			.side_dynamic = { \
				.side_byte = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
					}, \
					.value = (_val), \
				}, \
			}, \
		}, \
	}

#define _side_arg_dynamic_string(_val, _byte_order, _unit_size, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_STRING), \
		.u = { \
			.side_dynamic = { \
				.side_string = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
						.unit_size = _unit_size, \
						.byte_order = SIDE_ENUM_INIT(_byte_order), \
					}, \
					.value = (uintptr_t) (_val), \
				}, \
			}, \
		}, \
	}
#define side_arg_dynamic_string(_val, _attr...) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_string16(_val, _attr...) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint16_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_string16_le(_val, _attr...) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint16_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_string16_be(_val, _attr...) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint16_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_string32(_val, _attr...) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint32_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_string32_le(_val, _attr...) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_LE, sizeof(uint32_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_string32_be(_val, _attr...) \
	_side_arg_dynamic_string(_val, SIDE_TYPE_BYTE_ORDER_BE, sizeof(uint32_t), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define _side_arg_dynamic_integer(_field, _val, _type, _signedness, _byte_order, _integer_size, _len_bits, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.u = { \
			.side_dynamic = { \
				.side_integer = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
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

#define side_arg_dynamic_u8(_val, _attr...) \
	_side_arg_dynamic_integer(.side_u8, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(uint8_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s8(_val, _attr...) \
	_side_arg_dynamic_integer(.side_s8, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, SIDE_TYPE_BYTE_ORDER_HOST, sizeof(int8_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define _side_arg_dynamic_u16(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u16, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint16_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define _side_arg_dynamic_u32(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u32, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint32_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define _side_arg_dynamic_u64(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_u64, _val, SIDE_TYPE_DYNAMIC_INTEGER, false, _byte_order, sizeof(uint64_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define _side_arg_dynamic_s16(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s16, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int16_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define _side_arg_dynamic_s32(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s32, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int32_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define _side_arg_dynamic_s64(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_s64, _val, SIDE_TYPE_DYNAMIC_INTEGER, true, _byte_order, sizeof(int64_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define _side_arg_dynamic_pointer(_val, _byte_order, _attr...) \
	_side_arg_dynamic_integer(.side_uptr, (uintptr_t) (_val), SIDE_TYPE_DYNAMIC_POINTER, false, _byte_order, \
			sizeof(uintptr_t), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define _side_arg_dynamic_float(_field, _val, _type, _byte_order, _float_size, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(_type), \
		.u = { \
			.side_dynamic = { \
				.side_float = { \
					.type = { \
						.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
						.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
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

#define _side_arg_dynamic_float_binary16(_val, _byte_order, _attr...) \
	_side_arg_dynamic_float(.side_float_binary16, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float16), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define _side_arg_dynamic_float_binary32(_val, _byte_order, _attr...) \
	_side_arg_dynamic_float(.side_float_binary32, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float32), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define _side_arg_dynamic_float_binary64(_val, _byte_order, _attr...) \
	_side_arg_dynamic_float(.side_float_binary64, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float64), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define _side_arg_dynamic_float_binary128(_val, _byte_order, _attr...) \
	_side_arg_dynamic_float(.side_float_binary128, _val, SIDE_TYPE_DYNAMIC_FLOAT, _byte_order, sizeof(_Float128), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

/* Host endian */
#define side_arg_dynamic_u16(_val, _attr...) 			_side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_u32(_val, _attr...) 			_side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_u64(_val, _attr...) 			_side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s16(_val, _attr...) 			_side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s32(_val, _attr...) 			_side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s64(_val, _attr...) 			_side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_pointer(_val, _attr...) 		_side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary16(_val, _attr...)		_side_arg_dynamic_float_binary16(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary32(_val, _attr...)		_side_arg_dynamic_float_binary32(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary64(_val, _attr...)		_side_arg_dynamic_float_binary64(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary128(_val, _attr...)	_side_arg_dynamic_float_binary128(_val, SIDE_TYPE_FLOAT_WORD_ORDER_HOST, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

/* Little endian */
#define side_arg_dynamic_u16_le(_val, _attr...) 		_side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_u32_le(_val, _attr...) 		_side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_u64_le(_val, _attr...) 		_side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s16_le(_val, _attr...) 		_side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s32_le(_val, _attr...) 		_side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s64_le(_val, _attr...) 		_side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_pointer_le(_val, _attr...) 		_side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary16_le(_val, _attr...)	_side_arg_dynamic_float_binary16(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary32_le(_val, _attr...)	_side_arg_dynamic_float_binary32(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary64_le(_val, _attr...)	_side_arg_dynamic_float_binary64(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary128_le(_val, _attr...)	_side_arg_dynamic_float_binary128(_val, SIDE_TYPE_BYTE_ORDER_LE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

/* Big endian */
#define side_arg_dynamic_u16_be(_val, _attr...) 		_side_arg_dynamic_u16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_u32_be(_val, _attr...) 		_side_arg_dynamic_u32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_u64_be(_val, _attr...) 		_side_arg_dynamic_u64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s16_be(_val, _attr...) 		_side_arg_dynamic_s16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s32_be(_val, _attr...) 		_side_arg_dynamic_s32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_s64_be(_val, _attr...) 		_side_arg_dynamic_s64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_pointer_be(_val, _attr...) 		_side_arg_dynamic_pointer(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary16_be(_val, _attr...)	_side_arg_dynamic_float_binary16(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary32_be(_val, _attr...)	_side_arg_dynamic_float_binary32(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary64_be(_val, _attr...)	_side_arg_dynamic_float_binary64(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))
#define side_arg_dynamic_float_binary128_be(_val, _attr...)	_side_arg_dynamic_float_binary128(_val, SIDE_TYPE_BYTE_ORDER_BE, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_arg_dynamic_vla(_vla) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_VLA), \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_vla = SIDE_PTR_INIT(_vla), \
			}, \
		}, \
	}

#define side_arg_dynamic_vla_visitor(_dynamic_vla_visitor, _ctx, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_VLA_VISITOR), \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_vla_visitor = { \
					.app_ctx = SIDE_PTR_INIT(_ctx), \
					.visitor = SIDE_PTR_INIT(_dynamic_vla_visitor), \
					.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
					.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_struct(_struct) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_STRUCT), \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_struct = SIDE_PTR_INIT(_struct), \
			}, \
		}, \
	}

#define side_arg_dynamic_struct_visitor(_dynamic_struct_visitor, _ctx, _attr...) \
	{ \
		.type = SIDE_ENUM_INIT(SIDE_TYPE_DYNAMIC_STRUCT_VISITOR), \
		.u = { \
			.side_dynamic = { \
				.side_dynamic_struct_visitor = { \
					.app_ctx = SIDE_PTR_INIT(_ctx), \
					.visitor = SIDE_PTR_INIT(_dynamic_struct_visitor), \
					.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
					.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
				}, \
			}, \
		}, \
	}

#define side_arg_dynamic_define_vec(_identifier, _sav, _attr...) \
	const struct side_arg _identifier##_vec[] = { _sav }; \
	const struct side_arg_dynamic_vla _identifier = { \
		.sav = SIDE_PTR_INIT(_identifier##_vec), \
		.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
	}

#define side_arg_dynamic_define_struct(_identifier, _struct_fields, _attr...) \
	const struct side_arg_dynamic_field _identifier##_fields[] = { _struct_fields }; \
	const struct side_arg_dynamic_struct _identifier = { \
		.fields = SIDE_PTR_INIT(_identifier##_fields), \
		.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
		.len = SIDE_ARRAY_SIZE(_identifier##_fields), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
	}

#define side_arg_define_vec(_identifier, _sav) \
	const struct side_arg _identifier##_vec[] = { _sav }; \
	const struct side_arg_vec _identifier = { \
		.sav = SIDE_PTR_INIT(_identifier##_vec), \
		.len = SIDE_ARRAY_SIZE(_identifier##_vec), \
	}

#define side_arg_dynamic_field(_name, _elem) \
	{ \
		.field_name = SIDE_PTR_INIT(_name), \
		.elem = _elem, \
	}

/*
 * Event instrumentation description registration, runtime enabled state
 * check, and instrumentation invocation.
 */

#define side_arg_list(...)	__VA_ARGS__

#define side_event_cond(_identifier) \
	if (side_unlikely(__atomic_load_n(&side_event_state__##_identifier.enabled, \
					__ATOMIC_RELAXED)))

#define side_event_call(_identifier, _sav) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = SIDE_PTR_INIT(side_sav), \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		side_call(&(side_event_state__##_identifier).parent, &side_arg_vec); \
	}

#define side_event(_identifier, _sav) \
	side_event_cond(_identifier) \
		side_event_call(_identifier, SIDE_PARAM(_sav))

#define side_event_call_variadic(_identifier, _sav, _var_fields, _attr...) \
	{ \
		const struct side_arg side_sav[] = { _sav }; \
		const struct side_arg_vec side_arg_vec = { \
			.sav = SIDE_PTR_INIT(side_sav), \
			.len = SIDE_ARRAY_SIZE(side_sav), \
		}; \
		const struct side_arg_dynamic_field side_fields[] = { _var_fields }; \
		const struct side_arg_dynamic_struct var_struct = { \
			.fields = SIDE_PTR_INIT(side_fields), \
			.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
			.len = SIDE_ARRAY_SIZE(side_fields), \
			.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
		}; \
		side_call_variadic(&(side_event_state__##_identifier.parent), &side_arg_vec, &var_struct); \
	}

#define side_event_variadic(_identifier, _sav, _var, _attr...) \
	side_event_cond(_identifier) \
		side_event_call_variadic(_identifier, SIDE_PARAM(_sav), SIDE_PARAM(_var), SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define _side_define_event(_linkage, _identifier, _provider, _event, _loglevel, _fields, _flags, _attr...) \
	_linkage struct side_event_description __attribute__((section("side_event_description"))) \
			_identifier; \
	_linkage struct side_event_state_0 __attribute__((section("side_event_state"))) \
			side_event_state__##_identifier = { \
		.parent = { \
			.version = SIDE_EVENT_STATE_ABI_VERSION, \
		}, \
		.nr_callbacks = 0, \
		.enabled = 0, \
		.callbacks = (const struct side_callback *) &side_empty_callback[0], \
		.desc = &(_identifier), \
	}; \
	_linkage struct side_event_description __attribute__((section("side_event_description"))) \
			_identifier = { \
		.struct_size = offsetof(struct side_event_description, end), \
		.version = SIDE_EVENT_DESCRIPTION_ABI_VERSION, \
		.state = SIDE_PTR_INIT(&(side_event_state__##_identifier.parent)), \
		.provider_name = SIDE_PTR_INIT(_provider), \
		.event_name = SIDE_PTR_INIT(_event), \
		.fields = SIDE_PTR_INIT(_fields), \
		.attr = SIDE_PTR_INIT(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
		.flags = (_flags), \
		.nr_side_type_label = _NR_SIDE_TYPE_LABEL, \
		.nr_side_attr_type = _NR_SIDE_ATTR_TYPE, \
		.loglevel = SIDE_ENUM_INIT(_loglevel), \
		.nr_fields = SIDE_ARRAY_SIZE(SIDE_PARAM(_fields)), \
		.nr_attr = SIDE_ARRAY_SIZE(SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list())), \
	}; \
	static const struct side_event_description *side_event_ptr__##_identifier \
		__attribute__((section("side_event_description_ptr"), used)) = &(_identifier);

#define side_static_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(static, _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			0, ##_attr)

#define side_static_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(static, _identifier, _provider, _event, _loglevel, SIDE_PARAM(_fields), \
			SIDE_EVENT_FLAG_VARIADIC, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_hidden_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_hidden_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("hidden"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_export_event(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), 0, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_export_event_variadic(_identifier, _provider, _event, _loglevel, _fields, _attr...) \
	_side_define_event(__attribute__((visibility("default"))), _identifier, _provider, _event, \
			_loglevel, SIDE_PARAM(_fields), SIDE_EVENT_FLAG_VARIADIC, SIDE_PARAM_SELECT_ARG1(_, ##_attr, side_attr_list()))

#define side_declare_event(_identifier) \
	extern struct side_event_state_0 side_event_state_##_identifier; \
	extern struct side_event_description _identifier

#endif /* SIDE_INSTRUMENTATION_C_API_H */
