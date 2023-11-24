// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_ATTRIBUTE_ABI_H
#define SIDE_ATTRIBUTE_ABI_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

#include <side/type-value-abi.h>

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

	_NR_SIDE_ATTR_TYPE,	/* Last entry. */
};

struct side_attr_value {
	side_enum_t(enum side_attr_type, uint32_t) type;
	union {
		uint8_t bool_value;
		struct side_type_raw_string string_value;
		union side_integer_value integer_value;
		union side_float_value float_value;
		side_padding(32);
	} SIDE_PACKED u;
};
side_check_size(struct side_attr_value, 36);

struct side_attr {
	const struct side_type_raw_string key;
	const struct side_attr_value value;
} SIDE_PACKED;
side_check_size(struct side_attr, 54);

#endif /* SIDE_ATTRIBUTE_ABI_H */
