// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_ABI_TYPE_DESCRIPTION_H
#define SIDE_ABI_TYPE_DESCRIPTION_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

#include <side/abi/type-value.h>
#include <side/abi/attribute.h>
#include <side/abi/visitor.h>

/*
 * SIDE ABI for type description.
 *
 * This instrumentation ABI exposes 3 type systems:
 *
 * * Stack-copy type system: This is the core type system which can
 *   represent all supported types and into which all other type systems
 *   can be nested. This type system requires that every type is
 *   statically or dynamically declared and then registered, thus giving
 *   tracers a complete description of the events and their associated
 *   fields before the associated instrumentation is invoked. The
 *   application needs to copy each argument (side_arg_...()) onto the
 *   stack when calling the instrumentation.
 *
 *   This is the most expressive of the 3 type systems, althrough not the
 *   fastest due to the extra copy of the arguments.
 *
 * * Data-gathering type system: This type system requires every type to
 *   be statically or dynamically declared and registered, but does not
 *   require the application to copy its arguments onto the stack.
 *   Instead, the type description contains all the required information
 *   to fetch the data from the application memory. The only argument
 *   required from the instrumentation is the base pointer from which
 *   the data should be fetched.
 *
 *   This type system can be used as an event field, or nested within
 *   the stack-copy type system. Nesting of gather-vla within
 *   gather-array and gather-vla types is not allowed.
 *
 *   This type system is has the least overhead of the 3 type systems.
 *
 * * Dynamic type system: This type system receives both type
 *   description and actual data onto the stack at runtime. It has more
 *   overhead that the 2 other type systems, but does not require a
 *   prior registration of event field description. This makes it useful
 *   for seldom used types which are not performance critical, but for
 *   which registering each individual events would needlessly grow the
 *   number of events to declare and register.
 *
 *   Another use-case for this type system is for use by dynamically
 *   typed language runtimes, where the field type is only known when
 *   the instrumentation is called.
 *
 *   Those dynamic types can be either used as arguments to a variadic
 *   field list, or as on-stack instrumentation argument for a static
 *   type SIDE_TYPE_DYNAMIC place holder in the stack-copy type system.
 *
 * The extensibility scheme for the SIDE ABI for type description is as
 * follows:
 *
 * * Existing field types are never changed nor extended. Field types
 *   can be added to the ABI by reserving a label within
 *   enum side_type_label.
 * * Each union part of the ABI has an explicit size defined by a
 *   side_padding() member. Each structure and union have a static
 *   assert validating its size.
 * * Changing the semantic of the existing type fields is a breaking
 *   ABI change.
 *
 * Handling of unknown types by the tracers:
 *
 * * A tracer may choose to support only a subset of the types supported
 *   by libside. When encountering an unknown or unsupported type, the
 *   tracer has the option to either disallow the entire event or skip
 *   over the unknown type, both at event registration and when
 *   receiving the side_call arguments.
 */

enum side_type_label {
	/* Stack-copy basic types */
	SIDE_TYPE_NULL,
	SIDE_TYPE_BOOL,
	SIDE_TYPE_U8,
	SIDE_TYPE_U16,
	SIDE_TYPE_U32,
	SIDE_TYPE_U64,
	SIDE_TYPE_U128,
	SIDE_TYPE_S8,
	SIDE_TYPE_S16,
	SIDE_TYPE_S32,
	SIDE_TYPE_S64,
	SIDE_TYPE_S128,
	SIDE_TYPE_BYTE,
	SIDE_TYPE_POINTER,
	SIDE_TYPE_FLOAT_BINARY16,
	SIDE_TYPE_FLOAT_BINARY32,
	SIDE_TYPE_FLOAT_BINARY64,
	SIDE_TYPE_FLOAT_BINARY128,
	SIDE_TYPE_STRING_UTF8,
	SIDE_TYPE_STRING_UTF16,
	SIDE_TYPE_STRING_UTF32,

	/* Stack-copy compound types */
	SIDE_TYPE_STRUCT,
	SIDE_TYPE_VARIANT,
	SIDE_TYPE_ARRAY,
	SIDE_TYPE_VLA,
	SIDE_TYPE_VLA_VISITOR,

	/* Stack-copy enumeration types */
	SIDE_TYPE_ENUM,
	SIDE_TYPE_ENUM_BITMAP,

	/* Stack-copy place holder for dynamic types */
	SIDE_TYPE_DYNAMIC,

	/* Gather basic types */
	SIDE_TYPE_GATHER_BOOL,
	SIDE_TYPE_GATHER_INTEGER,
	SIDE_TYPE_GATHER_BYTE,
	SIDE_TYPE_GATHER_POINTER,
	SIDE_TYPE_GATHER_FLOAT,
	SIDE_TYPE_GATHER_STRING,

	/* Gather compound types */
	SIDE_TYPE_GATHER_STRUCT,
	SIDE_TYPE_GATHER_ARRAY,
	SIDE_TYPE_GATHER_VLA,

	/* Gather enumeration types */
	SIDE_TYPE_GATHER_ENUM,

	/* Dynamic basic types */
	SIDE_TYPE_DYNAMIC_NULL,
	SIDE_TYPE_DYNAMIC_BOOL,
	SIDE_TYPE_DYNAMIC_INTEGER,
	SIDE_TYPE_DYNAMIC_BYTE,
	SIDE_TYPE_DYNAMIC_POINTER,
	SIDE_TYPE_DYNAMIC_FLOAT,
	SIDE_TYPE_DYNAMIC_STRING,

	/* Dynamic compound types */
	SIDE_TYPE_DYNAMIC_STRUCT,
	SIDE_TYPE_DYNAMIC_STRUCT_VISITOR,
	SIDE_TYPE_DYNAMIC_VLA,
	SIDE_TYPE_DYNAMIC_VLA_VISITOR,

	_NR_SIDE_TYPE_LABEL,	/* Last entry. */
};

enum side_visitor_status {
	SIDE_VISITOR_STATUS_OK = 0,
	SIDE_VISITOR_STATUS_ERROR = -1,
};

enum side_type_gather_access_mode {
	SIDE_TYPE_GATHER_ACCESS_DIRECT,
	SIDE_TYPE_GATHER_ACCESS_POINTER,	/* Pointer dereference */
};

/* Type descriptions */
struct side_type_null {
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_type_null, 20);

struct side_type_bool {
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
	uint16_t bool_size;		/* bytes */
	uint16_t len_bits;		/* bits. 0 for (bool_size * CHAR_BITS) */
	side_enum_t(enum side_type_label_byte_order, uint8_t) byte_order;
} SIDE_PACKED;
side_check_size(struct side_type_bool, 25);

struct side_type_byte {
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_type_byte, 20);

struct side_type_string {
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
	uint8_t unit_size;		/* 1, 2, or 4 bytes */
	side_enum_t(enum side_type_label_byte_order, uint8_t) byte_order;
} SIDE_PACKED;
side_check_size(struct side_type_string, 22);

struct side_type_integer {
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
	uint16_t integer_size;		/* bytes */
	uint16_t len_bits;		/* bits. 0 for (integer_size * CHAR_BITS) */
	uint8_t signedness;		/* true/false */
	side_enum_t(enum side_type_label_byte_order, uint8_t) byte_order;
} SIDE_PACKED;
side_check_size(struct side_type_integer, 26);

struct side_type_float {
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
	uint16_t float_size;		/* bytes */
	side_enum_t(enum side_type_label_byte_order, uint8_t) byte_order;
} SIDE_PACKED;
side_check_size(struct side_type_float, 23);

struct side_enum_mapping {
	int64_t range_begin;
	int64_t range_end;
	struct side_type_raw_string label;
} SIDE_PACKED;
side_check_size(struct side_enum_mapping, 16 + sizeof(struct side_type_raw_string));

struct side_enum_mappings {
	side_ptr_t(const struct side_enum_mapping) mappings;
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_mappings;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_enum_mappings, 40);

struct side_enum_bitmap_mapping {
	uint64_t range_begin;
	uint64_t range_end;
	struct side_type_raw_string label;
} SIDE_PACKED;
side_check_size(struct side_enum_bitmap_mapping, 16 + sizeof(struct side_type_raw_string));

struct side_enum_bitmap_mappings {
	side_ptr_t(const struct side_enum_bitmap_mapping) mappings;
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_mappings;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_enum_bitmap_mappings, 40);

struct side_type_struct {
	side_ptr_t(const struct side_event_field) fields;
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_fields;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_type_struct, 40);

struct side_type_array {
	side_ptr_t(const struct side_type) elem_type;
	side_ptr_t(const struct side_attr) attr;
	uint32_t length;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_type_array, 40);

struct side_type_vla {
	side_ptr_t(const struct side_type) elem_type;
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_type_vla, 36);

struct side_type_vla_visitor {
	side_ptr_t(const struct side_type) elem_type;
	side_func_ptr_t(side_visitor_func) visitor;
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_type_vla_visitor, 52);

struct side_type_enum {
	side_ptr_t(const struct side_enum_mappings) mappings;
	side_ptr_t(const struct side_type) elem_type;
} SIDE_PACKED;
side_check_size(struct side_type_enum, 32);

struct side_type_enum_bitmap {
	side_ptr_t(const struct side_enum_bitmap_mappings) mappings;
	side_ptr_t(const struct side_type) elem_type;
} SIDE_PACKED;
side_check_size(struct side_type_enum_bitmap, 32);

struct side_type_gather_bool {
	uint64_t offset;	/* bytes */
	uint16_t offset_bits;	/* bits */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_bool type;
} SIDE_PACKED;
side_check_size(struct side_type_gather_bool, 11 + sizeof(struct side_type_bool));

struct side_type_gather_byte {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_byte type;
} SIDE_PACKED;
side_check_size(struct side_type_gather_byte, 9 + sizeof(struct side_type_byte));

struct side_type_gather_integer {
	uint64_t offset;	/* bytes */
	uint16_t offset_bits;	/* bits */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_integer type;
} SIDE_PACKED;
side_check_size(struct side_type_gather_integer, 11 + sizeof(struct side_type_integer));

struct side_type_gather_float {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_float type;
} SIDE_PACKED;
side_check_size(struct side_type_gather_float, 9 + sizeof(struct side_type_float));

struct side_type_gather_string {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_string type;
} SIDE_PACKED;
side_check_size(struct side_type_gather_string, 9 + sizeof(struct side_type_string));

struct side_type_gather_enum {
	side_ptr_t(const struct side_enum_mappings) mappings;
	side_ptr_t(const struct side_type) elem_type;
} SIDE_PACKED;
side_check_size(struct side_type_gather_enum, 32);

struct side_type_gather_struct {
	side_ptr_t(const struct side_type_struct) type;
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	uint32_t size;		/* bytes */
} SIDE_PACKED;
side_check_size(struct side_type_gather_struct, 29);

struct side_type_gather_array {
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_array type;
} SIDE_PACKED;
side_check_size(struct side_type_gather_array, 9 + sizeof(struct side_type_array));

struct side_type_gather_vla {
	side_ptr_t(const struct side_type) length_type;	/* side_length() */
	uint64_t offset;	/* bytes */
	uint8_t access_mode;	/* enum side_type_gather_access_mode */
	struct side_type_vla type;
} SIDE_PACKED;
side_check_size(struct side_type_gather_vla, 25 + sizeof(struct side_type_vla));

struct side_type_gather {
	union {
		struct side_type_gather_bool side_bool;
		struct side_type_gather_byte side_byte;
		struct side_type_gather_integer side_integer;
		struct side_type_gather_float side_float;
		struct side_type_gather_string side_string;
		struct side_type_gather_enum side_enum;
		struct side_type_gather_array side_array;
		struct side_type_gather_vla side_vla;
		struct side_type_gather_struct side_struct;
		side_padding(61);
	} SIDE_PACKED u;
} SIDE_PACKED;
side_check_size(struct side_type_gather, 61);

struct side_type {
	side_enum_t(enum side_type_label, uint16_t) type;
	union {
		/* Stack-copy basic types */
		struct side_type_null side_null;
		struct side_type_bool side_bool;
		struct side_type_byte side_byte;
		struct side_type_string side_string;
		struct side_type_integer side_integer;
		struct side_type_float side_float;

		/* Stack-copy compound types */
		struct side_type_array side_array;
		struct side_type_vla side_vla;
		struct side_type_vla_visitor side_vla_visitor;
		side_ptr_t(const struct side_type_struct) side_struct;
		side_ptr_t(const struct side_type_variant) side_variant;

		/* Stack-copy enumeration types */
		struct side_type_enum side_enum;
		struct side_type_enum_bitmap side_enum_bitmap;

		/* Gather types */
		struct side_type_gather side_gather;
		side_padding(62);
	} SIDE_PACKED u;
} SIDE_PACKED;
side_check_size(struct side_type, 64);

struct side_variant_option {
	int64_t range_begin;
	int64_t range_end;
	const struct side_type side_type;
} SIDE_PACKED;
side_check_size(struct side_variant_option, 16 + sizeof(const struct side_type));

struct side_type_variant {
	side_ptr_t(const struct side_variant_option) options;
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_options;
	uint32_t nr_attr;
	const struct side_type selector;
} SIDE_PACKED;
side_check_size(struct side_type_variant, 40 + sizeof(const struct side_type));

struct side_event_field {
	side_ptr_t(const char) field_name;
	struct side_type side_type;
} SIDE_PACKED;
side_check_size(struct side_event_field, 16 + sizeof(struct side_type));

#endif /* SIDE_ABI_TYPE_DESCRIPTION_H */
