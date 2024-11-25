// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <string.h>

#include "visit-description.h"

static
void side_visit_type(const struct side_description_visitor *description_visitor,
		const struct side_type *type_desc, void *priv);

static
void visit_gather_type(const struct side_description_visitor *description_visitor,
		const struct side_type *type_desc, void *priv);

static
void visit_gather_elem(const struct side_description_visitor *description_visitor,
		const struct side_type *type_desc, void *priv);

static
void description_visitor_gather_enum(const struct side_description_visitor *description_visitor,
		const struct side_type_gather *type_gather, void *priv);

static
void side_visit_elem(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	if (description_visitor->before_elem_func)
		description_visitor->before_elem_func(type_desc, priv);
	side_visit_type(description_visitor, type_desc, priv);
	if (description_visitor->after_elem_func)
		description_visitor->after_elem_func(type_desc, priv);
}

static
void side_visit_field(const struct side_description_visitor *description_visitor, const struct side_event_field *item_desc, void *priv)
{
	if (description_visitor->before_field_func)
		description_visitor->before_field_func(item_desc, priv);
	side_visit_type(description_visitor, &item_desc->side_type, priv);
	if (description_visitor->after_field_func)
		description_visitor->after_field_func(item_desc, priv);
}

static
void side_visit_option(const struct side_description_visitor *description_visitor, const struct side_variant_option *option_desc, void *priv)
{
	if (description_visitor->before_option_func)
		description_visitor->before_option_func(option_desc, priv);
	side_visit_type(description_visitor, &option_desc->side_type, priv);
	if (description_visitor->after_option_func)
		description_visitor->after_option_func(option_desc, priv);
}

static
void description_visitor_enum(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	const struct side_type *elem_type = side_ptr_get(type_desc->u.side_enum.elem_type);

	if (description_visitor->before_enum_type_func)
		description_visitor->before_enum_type_func(type_desc, priv);
	side_visit_elem(description_visitor, elem_type, priv);
	if (description_visitor->after_enum_type_func)
		description_visitor->after_enum_type_func(type_desc, priv);
}

static
void description_visitor_enum_bitmap(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	const struct side_type *elem_type = side_ptr_get(type_desc->u.side_enum_bitmap.elem_type);

	if (description_visitor->before_enum_bitmap_type_func)
		description_visitor->before_enum_bitmap_type_func(type_desc, priv);
	side_visit_elem(description_visitor, elem_type, priv);
	if (description_visitor->after_enum_bitmap_type_func)
		description_visitor->after_enum_bitmap_type_func(type_desc, priv);
}

static
void description_visitor_struct(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	const struct side_type_struct *side_struct = side_ptr_get(type_desc->u.side_struct);
	uint32_t i, len = side_array_length(&side_struct->fields);

	if (description_visitor->before_struct_type_func)
		description_visitor->before_struct_type_func(side_struct, priv);
	for (i = 0; i < len; i++)
		side_visit_field(description_visitor, side_array_at(&side_struct->fields, i), priv);
	if (description_visitor->after_struct_type_func)
		description_visitor->after_struct_type_func(side_struct, priv);
}

static
void description_visitor_variant(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	const struct side_type_variant *side_type_variant = side_ptr_get(type_desc->u.side_variant);
	const struct side_type *selector_type = &side_type_variant->selector;
	uint32_t i, len = side_array_length(&side_type_variant->options);

	switch (side_enum_get(selector_type->type)) {
	case SIDE_TYPE_U8:
	case SIDE_TYPE_U16:
	case SIDE_TYPE_U32:
	case SIDE_TYPE_U64:
	case SIDE_TYPE_U128:
	case SIDE_TYPE_S8:
	case SIDE_TYPE_S16:
	case SIDE_TYPE_S32:
	case SIDE_TYPE_S64:
	case SIDE_TYPE_S128:
		break;
	default:
		fprintf(stderr, "ERROR: Expecting integer variant selector type\n");
		abort();
	}
	if (description_visitor->before_variant_type_func)
		description_visitor->before_variant_type_func(side_type_variant, priv);
	for (i = 0; i < len; i++)
		side_visit_option(description_visitor, side_array_at(&side_type_variant->options, i), priv);
	if (description_visitor->after_variant_type_func)
		description_visitor->after_variant_type_func(side_type_variant, priv);
}

static
void description_visitor_optional(const struct side_description_visitor *description_visitor,
				const struct side_type_optional *optional, void *priv)
{
	const struct side_type *type_desc = side_ptr_get(optional->elem_type);

	if (description_visitor->before_optional_type_func)
		description_visitor->before_optional_type_func(type_desc, priv);
	side_visit_type(description_visitor, type_desc, priv);
	if (description_visitor->after_optional_type_func)
		description_visitor->after_optional_type_func(type_desc, priv);
}

static
void description_visitor_array(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	if (description_visitor->before_array_type_func)
		description_visitor->before_array_type_func(side_ptr_get(type_desc->u.side_array), priv);
	side_visit_elem(description_visitor, side_ptr_get(side_ptr_get(type_desc->u.side_array)->elem_type), priv);
	if (description_visitor->after_array_type_func)
		description_visitor->after_array_type_func(side_ptr_get(type_desc->u.side_array), priv);
}

static
void description_visitor_vla(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	if (description_visitor->before_vla_type_func)
		description_visitor->before_vla_type_func(side_ptr_get(type_desc->u.side_vla), priv);
	side_visit_elem(description_visitor, side_ptr_get(side_ptr_get(type_desc->u.side_vla)->length_type), priv);
	if (description_visitor->after_length_vla_type_func)
		description_visitor->after_length_vla_type_func(side_ptr_get(type_desc->u.side_vla), priv);
	side_visit_elem(description_visitor, side_ptr_get(side_ptr_get(type_desc->u.side_vla)->elem_type), priv);
	if (description_visitor->after_element_vla_type_func)
		description_visitor->after_element_vla_type_func(side_ptr_get(type_desc->u.side_vla), priv);
}

static
void description_visitor_vla_visitor(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	if (description_visitor->before_vla_visitor_type_func)
		description_visitor->before_vla_visitor_type_func(side_ptr_get(type_desc->u.side_vla_visitor), priv);
	side_visit_elem(description_visitor, side_ptr_get(side_ptr_get(type_desc->u.side_vla_visitor)->length_type), priv);
	if (description_visitor->after_length_vla_visitor_type_func)
		description_visitor->after_length_vla_visitor_type_func(side_ptr_get(type_desc->u.side_vla_visitor), priv);
	side_visit_elem(description_visitor, side_ptr_get(side_ptr_get(type_desc->u.side_vla_visitor)->elem_type), priv);
	if (description_visitor->after_element_vla_visitor_type_func)
		description_visitor->after_element_vla_visitor_type_func(side_ptr_get(type_desc->u.side_vla_visitor), priv);
}

static
void visit_gather_field(const struct side_description_visitor *description_visitor, const struct side_event_field *field, void *priv)
{
	if (description_visitor->before_field_func)
		description_visitor->before_field_func(field, priv);
	(void) visit_gather_type(description_visitor, &field->side_type, priv);
	if (description_visitor->after_field_func)
		description_visitor->after_field_func(field, priv);
}

static
void description_visitor_gather_struct(const struct side_description_visitor *description_visitor, const struct side_type_gather *type_gather, void *priv)
{
	const struct side_type_gather_struct *side_gather_struct = &type_gather->u.side_struct;
	const struct side_type_struct *side_struct = side_ptr_get(side_gather_struct->type);
	uint32_t i;

	if (description_visitor->before_gather_struct_type_func)
		description_visitor->before_gather_struct_type_func(side_gather_struct, priv);
	for (i = 0; i < side_array_length(&side_struct->fields); i++)
		visit_gather_field(description_visitor, side_array_at(&side_struct->fields, i), priv);
	if (description_visitor->after_gather_struct_type_func)
		description_visitor->after_gather_struct_type_func(side_gather_struct, priv);
}

static
void description_visitor_gather_array(const struct side_description_visitor *description_visitor, const struct side_type_gather *type_gather, void *priv)
{
	const struct side_type_gather_array *side_gather_array = &type_gather->u.side_array;
	const struct side_type_array *side_array = &side_gather_array->type;
	const struct side_type *elem_type = side_ptr_get(side_array->elem_type);

	if (description_visitor->before_gather_array_type_func)
		description_visitor->before_gather_array_type_func(side_gather_array, priv);
	switch (side_enum_get(elem_type->type)) {
	case SIDE_TYPE_GATHER_VLA:
		fprintf(stderr, "<gather VLA only supported within gather structures>\n");
		abort();
	default:
		break;
	}
	visit_gather_elem(description_visitor, elem_type, priv);
	if (description_visitor->after_gather_array_type_func)
		description_visitor->after_gather_array_type_func(side_gather_array, priv);
}

static
void description_visitor_gather_vla(const struct side_description_visitor *description_visitor, const struct side_type_gather *type_gather, void *priv)
{
	const struct side_type_gather_vla *side_gather_vla = &type_gather->u.side_vla;
	const struct side_type_vla *side_vla = &side_gather_vla->type;
	const struct side_type *length_type = side_ptr_get(side_gather_vla->type.length_type);
	const struct side_type *elem_type = side_ptr_get(side_vla->elem_type);

	/* Access length */
	switch (side_enum_get(length_type->type)) {
	case SIDE_TYPE_GATHER_INTEGER:
		break;
	default:
		fprintf(stderr, "<gather VLA expects integer gather length type>\n");
		abort();
	}
	switch (side_enum_get(elem_type->type)) {
	case SIDE_TYPE_GATHER_VLA:
		fprintf(stderr, "<gather VLA only supported within gather structures>\n");
		abort();
	default:
		break;
	}
	if (description_visitor->before_gather_vla_type_func)
		description_visitor->before_gather_vla_type_func(side_gather_vla, priv);
	visit_gather_elem(description_visitor, length_type, priv);
	if (description_visitor->after_length_gather_vla_type_func)
		description_visitor->after_length_gather_vla_type_func(side_gather_vla, priv);
	visit_gather_elem(description_visitor, elem_type, priv);
	if (description_visitor->after_element_gather_vla_type_func)
		description_visitor->after_element_gather_vla_type_func(side_gather_vla, priv);
}

static
void description_visitor_gather_bool(const struct side_description_visitor *description_visitor, const struct side_type_gather *type_gather, void *priv)
{
	if (description_visitor->gather_bool_type_func)
		description_visitor->gather_bool_type_func(&type_gather->u.side_bool, priv);
}

static
void description_visitor_gather_byte(const struct side_description_visitor *description_visitor, const struct side_type_gather *type_gather, void *priv)
{
	if (description_visitor->gather_byte_type_func)
		description_visitor->gather_byte_type_func(&type_gather->u.side_byte, priv);
}

static
void description_visitor_gather_integer(const struct side_description_visitor *description_visitor, const struct side_type_gather *type_gather,
		enum side_type_label integer_type, void *priv)
{
	switch (integer_type) {
	case SIDE_TYPE_GATHER_INTEGER:
		if (description_visitor->gather_integer_type_func)
			description_visitor->gather_integer_type_func(&type_gather->u.side_integer, priv);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		if (description_visitor->gather_pointer_type_func)
			description_visitor->gather_pointer_type_func(&type_gather->u.side_integer, priv);
		break;
	default:
		fprintf(stderr, "Unexpected integer type\n");
		abort();
	}
}

static
void description_visitor_gather_float(const struct side_description_visitor *description_visitor, const struct side_type_gather *type_gather, void *priv)
{
	if (description_visitor->gather_float_type_func)
		description_visitor->gather_float_type_func(&type_gather->u.side_float, priv);
}

static
void description_visitor_gather_string(const struct side_description_visitor *description_visitor, const struct side_type_gather *type_gather, void *priv)
{

	if (description_visitor->gather_string_type_func)
		description_visitor->gather_string_type_func(&type_gather->u.side_string, priv);
}

static
void visit_gather_type(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	switch (side_enum_get(type_desc->type)) {
		/* Gather basic types */
	case SIDE_TYPE_GATHER_BOOL:
		description_visitor_gather_bool(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_INTEGER:
		description_visitor_gather_integer(description_visitor, &type_desc->u.side_gather, SIDE_TYPE_GATHER_INTEGER, priv);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		description_visitor_gather_byte(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		description_visitor_gather_integer(description_visitor, &type_desc->u.side_gather, SIDE_TYPE_GATHER_POINTER, priv);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		description_visitor_gather_float(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_STRING:
		description_visitor_gather_string(description_visitor, &type_desc->u.side_gather, priv);
		break;

		/* Gather enumeration types */
	case SIDE_TYPE_GATHER_ENUM:
		description_visitor_gather_enum(description_visitor, &type_desc->u.side_gather, priv);
		break;

		/* Gather compound types */
	case SIDE_TYPE_GATHER_STRUCT:
		description_visitor_gather_struct(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_ARRAY:
		description_visitor_gather_array(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_VLA:
		description_visitor_gather_vla(description_visitor, &type_desc->u.side_gather, priv);
		break;
	default:
		fprintf(stderr, "<UNKNOWN GATHER TYPE>");
		abort();
	}
}

static
void visit_gather_elem(const struct side_description_visitor *description_visitor, const struct side_type *type_desc, void *priv)
{
	if (description_visitor->before_elem_func)
		description_visitor->before_elem_func(type_desc, priv);
	visit_gather_type(description_visitor, type_desc, priv);
	if (description_visitor->after_elem_func)
		description_visitor->after_elem_func(type_desc, priv);
}

static
void description_visitor_gather_enum(const struct side_description_visitor *description_visitor, const struct side_type_gather *type_gather, void *priv)
{
	const struct side_type *elem_type = side_ptr_get(type_gather->u.side_enum.elem_type);

	if (description_visitor->before_gather_enum_type_func)
		description_visitor->before_gather_enum_type_func(&type_gather->u.side_enum, priv);
	side_visit_elem(description_visitor, elem_type, priv);
	if (description_visitor->after_gather_enum_type_func)
		description_visitor->after_gather_enum_type_func(&type_gather->u.side_enum, priv);
}

static
void side_visit_type(const struct side_description_visitor *description_visitor,
		const struct side_type *type_desc, void *priv)
{
	enum side_type_label type = side_enum_get(type_desc->type);

	switch (type) {
		/* Stack-copy basic types */
	case SIDE_TYPE_NULL:
		if (description_visitor->null_type_func)
			description_visitor->null_type_func(type_desc, priv);
		break;
	case SIDE_TYPE_BOOL:
		if (description_visitor->bool_type_func)
			description_visitor->bool_type_func(type_desc, priv);
		break;
	case SIDE_TYPE_U8:		/* Fallthrough */
	case SIDE_TYPE_U16:		/* Fallthrough */
	case SIDE_TYPE_U32:		/* Fallthrough */
	case SIDE_TYPE_U64:		/* Fallthrough */
	case SIDE_TYPE_U128:		/* Fallthrough */
	case SIDE_TYPE_S8:		/* Fallthrough */
	case SIDE_TYPE_S16:		/* Fallthrough */
	case SIDE_TYPE_S32:		/* Fallthrough */
	case SIDE_TYPE_S64:		/* Fallthrough */
	case SIDE_TYPE_S128:
		if (description_visitor->integer_type_func)
			description_visitor->integer_type_func(type_desc, priv);
		break;
	case SIDE_TYPE_BYTE:
		if (description_visitor->byte_type_func)
			description_visitor->byte_type_func(type_desc, priv);
		break;
	case SIDE_TYPE_POINTER:
		if (description_visitor->pointer_type_func)
			description_visitor->pointer_type_func(type_desc, priv);
		break;
	case SIDE_TYPE_FLOAT_BINARY16:	/* Fallthrough */
	case SIDE_TYPE_FLOAT_BINARY32:	/* Fallthrough */
	case SIDE_TYPE_FLOAT_BINARY64:	/* Fallthrough */
	case SIDE_TYPE_FLOAT_BINARY128:
		if (description_visitor->float_type_func)
			description_visitor->float_type_func(type_desc, priv);
		break;
	case SIDE_TYPE_STRING_UTF8:	/* Fallthrough */
	case SIDE_TYPE_STRING_UTF16:	/* Fallthrough */
	case SIDE_TYPE_STRING_UTF32:
		if (description_visitor->string_type_func)
			description_visitor->string_type_func(type_desc, priv);
		break;
	case SIDE_TYPE_ENUM:
		description_visitor_enum(description_visitor, type_desc, priv);
		break;
	case SIDE_TYPE_ENUM_BITMAP:
		description_visitor_enum_bitmap(description_visitor, type_desc, priv);
		break;

		/* Stack-copy compound types */
	case SIDE_TYPE_STRUCT:
		description_visitor_struct(description_visitor, type_desc, priv);
		break;
	case SIDE_TYPE_VARIANT:
		description_visitor_variant(description_visitor, type_desc, priv);
		break;
	case SIDE_TYPE_ARRAY:
		description_visitor_array(description_visitor, type_desc, priv);
		break;
	case SIDE_TYPE_VLA:
		description_visitor_vla(description_visitor, type_desc, priv);
		break;
	case SIDE_TYPE_VLA_VISITOR:
		description_visitor_vla_visitor(description_visitor, type_desc, priv);
		break;

		/* Gather basic types */
	case SIDE_TYPE_GATHER_BOOL:
		description_visitor_gather_bool(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_INTEGER:
		description_visitor_gather_integer(description_visitor, &type_desc->u.side_gather, SIDE_TYPE_GATHER_INTEGER, priv);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		description_visitor_gather_byte(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		description_visitor_gather_integer(description_visitor, &type_desc->u.side_gather, SIDE_TYPE_GATHER_POINTER, priv);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		description_visitor_gather_float(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_STRING:
		description_visitor_gather_string(description_visitor, &type_desc->u.side_gather, priv);
		break;

		/* Gather compound type */
	case SIDE_TYPE_GATHER_STRUCT:
		description_visitor_gather_struct(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_ARRAY:
		description_visitor_gather_array(description_visitor, &type_desc->u.side_gather, priv);
		break;
	case SIDE_TYPE_GATHER_VLA:
		description_visitor_gather_vla(description_visitor, &type_desc->u.side_gather, priv);
		break;

		/* Gather enumeration types */
	case SIDE_TYPE_GATHER_ENUM:
		description_visitor_gather_enum(description_visitor, &type_desc->u.side_gather, priv);
		break;

	/* Dynamic type */
	case SIDE_TYPE_DYNAMIC:
		if (description_visitor->dynamic_type_func)
			description_visitor->dynamic_type_func(type_desc, priv);
		break;

	case SIDE_TYPE_OPTIONAL:
		description_visitor_optional(description_visitor, side_ptr_get(type_desc->u.side_optional), priv);
		break;

	default:
		fprintf(stderr, "<UNKNOWN TYPE>\n");
		abort();
	}
}

void description_visitor_event(const struct side_description_visitor *description_visitor,
		const struct side_event_description *desc, void *priv)
{
	uint32_t i, len = side_array_length(&desc->fields);

	if (description_visitor->before_event_func)
		description_visitor->before_event_func(desc, priv);
	if (len) {
		if (description_visitor->before_static_fields_func)
			description_visitor->before_static_fields_func(desc, priv);
		for (i = 0; i < len; i++)
			side_visit_field(description_visitor, side_array_at(&desc->fields, i), priv);
		if (description_visitor->after_static_fields_func)
			description_visitor->after_static_fields_func(desc, priv);
	}
	if (description_visitor->after_event_func)
		description_visitor->after_event_func(desc, priv);
}
