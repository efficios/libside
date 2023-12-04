// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_ABI_TYPE_VALUE_H
#define SIDE_ABI_TYPE_VALUE_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

/*
 * SIDE ABI for type values.
 *
 * The extensibility scheme for the SIDE ABI for type values is as
 * follows:
 *
 * * Existing type values are never changed nor extended. Type values
 *   can be added to the ABI by reserving a label within enum
 *   side_type_label.
 * * Each union part of the ABI has an explicit size defined by a
 *   side_padding() member. Each structure and union have a static
 *   assert validating its size.
 * * Changing the semantic of the existing type value fields is a
 *   breaking ABI change.
 *
 * Handling of unknown type values by the tracers:
 *
 * * A tracer may choose to support only a subset of the type values
 *   supported by libside. When encountering an unknown or unsupported
 *   type value, the tracer has the option to either disallow the entire
 *   event or skip over the unknown type, both at event registration and
 *   when receiving the side_call arguments.
 */

enum side_type_label_byte_order {
	SIDE_TYPE_BYTE_ORDER_LE = 0,
	SIDE_TYPE_BYTE_ORDER_BE = 1,
};

#if (SIDE_BYTE_ORDER == SIDE_LITTLE_ENDIAN)
enum side_integer128_split_index {
	SIDE_INTEGER128_SPLIT_LOW = 0,
	SIDE_INTEGER128_SPLIT_HIGH = 1,
	NR_SIDE_INTEGER128_SPLIT,
};
#else
enum side_integer128_split_index {
	SIDE_INTEGER128_SPLIT_HIGH = 0,
	SIDE_INTEGER128_SPLIT_LOW = 1,
	NR_SIDE_INTEGER128_SPLIT,
};
#endif

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
	/* Indexed with enum side_integer128_split_index */
	uint64_t side_u128_split[NR_SIDE_INTEGER128_SPLIT];
	int64_t side_s128_split[NR_SIDE_INTEGER128_SPLIT];
#ifdef __SIZEOF_INT128__
	unsigned __int128 side_u128;
	__int128 side_s128;
#endif
	side_padding(32);
} SIDE_PACKED;
side_check_size(union side_integer_value, 32);

union side_bool_value {
	uint8_t side_bool8;
	uint16_t side_bool16;
	uint32_t side_bool32;
	uint64_t side_bool64;
	side_padding(32);
} SIDE_PACKED;
side_check_size(union side_bool_value, 32);

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
	side_padding(32);
} SIDE_PACKED;
side_check_size(union side_float_value, 32);

struct side_type_raw_string {
	side_ptr_t(const void) p;	/* pointer to string */
	uint8_t unit_size;		/* 1, 2, or 4 bytes */
	side_enum_t(enum side_type_label_byte_order, uint8_t) byte_order;
} SIDE_PACKED;
side_check_size(struct side_type_raw_string, 18);

#endif /* SIDE_ABI_TYPE_VALUE_H */
