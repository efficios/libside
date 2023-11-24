// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_TYPE_VALUE_ABI_H
#define SIDE_TYPE_VALUE_ABI_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

enum side_type_label_byte_order {
	SIDE_TYPE_BYTE_ORDER_LE = 0,
	SIDE_TYPE_BYTE_ORDER_BE = 1,
};

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

#endif /* SIDE_TYPE_VALUE_ABI_H */
