// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

#ifndef SIDE_CONSOLE_TRACER_COMMON_H
#define SIDE_CONSOLE_TRACER_COMMON_H

#include <stddef.h>
#include <stdint.h>

#include <side/trace.h>

/*
 * An integer value of a side event, split as the side ABI describes
 * integers of more than 64 bits.
 */
union int_value {
	uint64_t u[NR_SIDE_INTEGER128_SPLIT];
	int64_t s[NR_SIDE_INTEGER128_SPLIT];
};

/*
 * The value of an integer, of its own byte order and bit offset,
 * loaded into the byte order of this process.
 */
union int_value tracer_load_integer_value(const struct side_type_integer *type_integer,
		const union side_integer_value *value,
		uint16_t offset_bits, uint16_t *_len_bits);

/*
 * Convert a string of the given unit size and byte order into UTF-8.
 * The output is the input itself when it already is UTF-8, and has to
 * be freed otherwise.
 */
void tracer_convert_string_to_utf8(const void *p, uint8_t unit_size,
		enum side_type_label_byte_order byte_order,
		size_t *strlen_with_null, char **output_str);

#endif /* SIDE_CONSOLE_TRACER_COMMON_H */
