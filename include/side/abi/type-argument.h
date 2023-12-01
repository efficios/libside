// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef SIDE_ABI_TYPE_ARGUMENT_H
#define SIDE_ABI_TYPE_ARGUMENT_H

#include <stdint.h>
#include <side/macros.h>
#include <side/endian.h>

#include <side/abi/type-value.h>
#include <side/abi/attribute.h>
#include <side/abi/type-description.h>
#include <side/abi/visitor.h>

/*
 * SIDE ABI for arguments passed to instrumentation call site.
 *
 * The extensibility scheme for the SIDE ABI for call site arguments is
 * as follows:
 *
 * * Existing argument types are never changed nor extended. Argument
 *   types can be added to the ABI by reserving a label within
 *   enum side_type_label.
 * * Each union part of the ABI has an explicit size defined by a
 *   side_padding() member. Each structure and union have a static
 *   assert validating its size.
 * * Changing the semantic of the existing argument type fields is a
 *   breaking ABI change.
 *
 * Handling of unknown argument types by the tracers:
 *
 * * A tracer may choose to support only a subset of the types supported
 *   by libside. When encountering an unknown or unsupported type, the
 *   tracer has the option to either disallow the entire event or skip
 *   over the unknown type, both at event registration and when
 *   receiving the side_call arguments.
 */

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

enum side_arg_flag_bit {
	SIDE_ARG_FLAG_INCOMPLETE_BIT = 0,

	_NR_SIDE_ARG_FLAG_BIT,
};

enum side_arg_flag {
	SIDE_ARG_FLAG_INCOMPLETE = (1U << SIDE_ARG_FLAG_INCOMPLETE_BIT),
};

union side_arg_static {
	/* Stack-copy basic types */
	union side_bool_value bool_value;
	uint8_t byte_value;
	side_ptr_t(const void) string_value;	/* const {uint8_t, uint16_t, uint32_t} * */
	union side_integer_value integer_value;
	union side_float_value float_value;

	/* Stack-copy compound types */
	side_ptr_t(const struct side_arg_vec) side_struct;
	side_ptr_t(const struct side_arg_variant) side_variant;
	side_ptr_t(const struct side_arg_vec) side_array;
	side_ptr_t(const struct side_arg_vec) side_vla;
	void *side_vla_app_visitor_ctx;

	/* Gather basic types */
	side_ptr_t(const void) side_bool_gather_ptr;
	side_ptr_t(const void) side_byte_gather_ptr;
	side_ptr_t(const void) side_integer_gather_ptr;
	side_ptr_t(const void) side_float_gather_ptr;
	side_ptr_t(const void) side_string_gather_ptr;

	/* Gather compound types */
	side_ptr_t(const void) side_array_gather_ptr;
	side_ptr_t(const void) side_struct_gather_ptr;
	struct {
		side_ptr_t(const void) ptr;
		side_ptr_t(const void) length_ptr;
	} SIDE_PACKED side_vla_gather;
	side_padding(32);
} SIDE_PACKED;
side_check_size(union side_arg_static, 32);

struct side_arg_dynamic_vla {
	side_ptr_t(const struct side_arg) sav;
	side_ptr_t(const struct side_attr) attr;
	uint32_t len;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_arg_dynamic_vla, 40);

struct side_arg_dynamic_struct {
	side_ptr_t(const struct side_arg_dynamic_field) fields;
	side_ptr_t(const struct side_attr) attr;
	uint32_t len;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_arg_dynamic_struct, 40);

struct side_dynamic_struct_visitor {
	side_func_ptr_t(side_dynamic_struct_visitor_func) visitor;
	side_ptr_t(void) app_ctx;
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_dynamic_struct_visitor, 52);

struct side_dynamic_vla_visitor {
	side_func_ptr_t(side_visitor_func) visitor;
	side_ptr_t(void) app_ctx;
	side_ptr_t(const struct side_attr) attr;
	uint32_t nr_attr;
} SIDE_PACKED;
side_check_size(struct side_dynamic_vla_visitor, 52);

union side_arg_dynamic {
	/* Dynamic basic types */
	struct side_type_null side_null;
	struct {
		struct side_type_bool type;
		union side_bool_value value;
	} SIDE_PACKED side_bool;
	struct {
		struct side_type_byte type;
		uint8_t value;
	} SIDE_PACKED side_byte;
	struct {
		struct side_type_string type;
		uint64_t value;	/* const char * */
	} SIDE_PACKED side_string;
	struct {
		struct side_type_integer type;
		union side_integer_value value;
	} SIDE_PACKED side_integer;
	struct {
		struct side_type_float type;
		union side_float_value value;
	} SIDE_PACKED side_float;

	/* Dynamic compound types */
	side_ptr_t(const struct side_arg_dynamic_struct) side_dynamic_struct;
	side_ptr_t(const struct side_arg_dynamic_vla) side_dynamic_vla;

	struct side_dynamic_struct_visitor side_dynamic_struct_visitor;
	struct side_dynamic_vla_visitor side_dynamic_vla_visitor;

	side_padding(58);
} SIDE_PACKED;
side_check_size(union side_arg_dynamic, 58);

struct side_arg {
	side_enum_t(enum side_type_label, uint16_t) type;
	uint16_t flags;
	union {
		union side_arg_static side_static;
		union side_arg_dynamic side_dynamic;
		side_padding(60);
	} SIDE_PACKED u;
} SIDE_PACKED;
side_check_size(struct side_arg, 64);

struct side_arg_variant {
	struct side_arg selector;
	struct side_arg option;
} SIDE_PACKED;
side_check_size(struct side_arg_variant, 128);

struct side_arg_vec {
	side_ptr_t(const struct side_arg) sav;
	uint32_t len;
} SIDE_PACKED;
side_check_size(struct side_arg_vec, 20);

struct side_arg_dynamic_field {
	side_ptr_t(const char) field_name;
	const struct side_arg elem;
} SIDE_PACKED;
side_check_size(struct side_arg_dynamic_field, 16 + sizeof(const struct side_arg));

#endif /* SIDE_ABI_TYPE_ARGUMENT_H */
