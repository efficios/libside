// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_ABI_ATTRIBUTE_H
#define SIDE_ABI_ATTRIBUTE_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

#include <side/abi/type-value.h>

/*
 * SIDE ABI for description of event and type attributes.
 * Event and type attributes are an optional array of { key, value }
 * pairs which can be associated with either an event or a type.
 *
 * The extensibility scheme for the SIDE ABI for description of event
 * and type attributes is as follows:
 *
 * * Existing attribute types are never changed nor extended. Attribute
 *   types can be added to the ABI by reserving a label within
 *   enum side_attr_type.
 * * Each union part of the ABI has an explicit size defined by a
 *   side_padding() member. Each structure and union have a static
 *   assert validating its size.
 * * Changing the semantic of the existing attribute type fields is a
 *   breaking ABI change.
 *
 * Handling of unknown attribute types by the tracers:
 *
 * * A tracer may choose to support only a subset of the types supported
 *   by libside. When encountering an unknown or unsupported attribute
 *   type, the tracer has the option to either disallow the entire
 *   event, skip over the field containing the unknown attribute, or
 *   skip over the unknown attribute, both at event registration and
 *   when receiving the side_call arguments.
 */

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

#endif /* SIDE_ABI_ATTRIBUTE_H */
