// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "visit-description.h"

static
void *do_visit_side_pointer(const struct side_description_visitor *visitor, void *ptr)
{
	void *(*resolve_pointer)(void *pointer, void *priv) =
		visitor->callbacks->resolve_pointer_func;

	return resolve_pointer ? resolve_pointer(ptr, visitor->priv) : ptr;
}

#define visit_pointer(visitor, ptr)						\
	((__typeof__(ptr))do_visit_side_pointer(visitor, (void*)(ptr)))

#define visit_side_pointer(visitor, ptr)					\
	visit_pointer(visitor, side_ptr_get(ptr))

#define visit_side_array(visitor, array)							\
	{											\
		.elements = SIDE_PTR_INIT(visit_side_pointer(visitor, (array)->elements)),	\
		.length = (array)->length,							\
	}

static
void side_visit_type(const struct side_description_visitor *visitor, const struct side_type *type_desc);

static
void visit_gather_type(const struct side_description_visitor *visitor, const struct side_type *type_desc);

static
void visit_gather_elem(const struct side_description_visitor *visitor, const struct side_type *type_desc);

static
void description_visitor_gather_enum(const struct side_description_visitor *visitor, const struct side_type_gather *type_gather);

static
void side_visit_elem(const struct side_description_visitor *visitor, const struct side_type *type_desc)
{
	if (visitor->callbacks->before_elem_func)
		visitor->callbacks->before_elem_func(type_desc, visitor->priv);
	side_visit_type(visitor, type_desc);
	if (visitor->callbacks->after_elem_func)
		visitor->callbacks->after_elem_func(type_desc, visitor->priv);
}

static
void side_visit_field(const struct side_description_visitor *visitor, const struct side_event_field *item_desc)
{
	if (visitor->callbacks->before_field_func)
		visitor->callbacks->before_field_func(item_desc, visitor->priv);
	side_visit_type(visitor, &item_desc->side_type);
	if (visitor->callbacks->after_field_func)
		visitor->callbacks->after_field_func(item_desc, visitor->priv);
}

static
void side_visit_option(const struct side_description_visitor *visitor, const struct side_variant_option *option_desc)
{
	if (visitor->callbacks->before_option_func)
		visitor->callbacks->before_option_func(option_desc, visitor->priv);
	side_visit_type(visitor, &option_desc->side_type);
	if (visitor->callbacks->after_option_func)
		visitor->callbacks->after_option_func(option_desc, visitor->priv);
}

static
void description_visitor_enum(const struct side_description_visitor *visitor, const struct side_type_enum *type)
{
	const struct side_type *elem_type = visit_side_pointer(visitor, type->elem_type);

	if (visitor->callbacks->before_enum_type_func)
		visitor->callbacks->before_enum_type_func(type, visitor->priv);
	side_visit_elem(visitor, elem_type);
	if (visitor->callbacks->after_enum_type_func)
		visitor->callbacks->after_enum_type_func(type, visitor->priv);
}

static
void description_visitor_enum_bitmap(const struct side_description_visitor *visitor, const struct side_type_enum_bitmap *type)
{
	const struct side_type *elem_type = visit_side_pointer(visitor, type->elem_type);

	if (visitor->callbacks->before_enum_bitmap_type_func)
		visitor->callbacks->before_enum_bitmap_type_func(type, visitor->priv);
	side_visit_elem(visitor, elem_type);
	if (visitor->callbacks->after_enum_bitmap_type_func)
		visitor->callbacks->after_enum_bitmap_type_func(type, visitor->priv);
}

static
void description_visitor_struct(const struct side_description_visitor *visitor, const struct side_type *type_desc)
{
	const struct side_type_struct *side_struct = visit_side_pointer(visitor, type_desc->u.side_struct);
	side_array_t(const struct side_event_field) fields = visit_side_array(visitor, &side_struct->fields);
	uint32_t i, len = side_array_length(&fields);

	if (visitor->callbacks->before_struct_type_func)
		visitor->callbacks->before_struct_type_func(side_struct, visitor->priv);
	for (i = 0; i < len; i++)
		side_visit_field(visitor, side_array_at(&fields, i));
	if (visitor->callbacks->after_struct_type_func)
		visitor->callbacks->after_struct_type_func(side_struct, visitor->priv);
}

static
void description_visitor_variant(const struct side_description_visitor *visitor, const struct side_type *type_desc)
{
	const struct side_type_variant *side_type_variant = visit_side_pointer(visitor, type_desc->u.side_variant);
	const struct side_type *selector_type = &side_type_variant->selector;
	side_array_t(const struct side_variant_option) options = visit_side_array(visitor, &side_type_variant->options);
	uint32_t i, len = side_array_length(&options);

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
	if (visitor->callbacks->before_variant_type_func)
		visitor->callbacks->before_variant_type_func(side_type_variant, visitor->priv);
	side_visit_type(visitor, selector_type);
	if (visitor->callbacks->after_variant_selector_type_func)
		visitor->callbacks->after_variant_selector_type_func(selector_type, visitor->priv);
	for (i = 0; i < len; i++)
		side_visit_option(visitor, side_array_at(&options, i));
	if (visitor->callbacks->after_variant_type_func)
		visitor->callbacks->after_variant_type_func(side_type_variant, visitor->priv);
}

static
void description_visitor_optional(const struct side_description_visitor *visitor, const struct side_type_optional *optional)
{
	const struct side_type *type_desc = visit_side_pointer(visitor, optional->elem_type);

	if (visitor->callbacks->before_optional_type_func)
		visitor->callbacks->before_optional_type_func(optional, visitor->priv);
	side_visit_type(visitor, type_desc);
	if (visitor->callbacks->after_optional_type_func)
		visitor->callbacks->after_optional_type_func(optional, visitor->priv);
}

static
void description_visitor_array(const struct side_description_visitor *visitor, const struct side_type *type_desc)
{
	if (visitor->callbacks->before_array_type_func)
		visitor->callbacks->before_array_type_func(visit_side_pointer(visitor, type_desc->u.side_array), visitor->priv);
	side_visit_elem(visitor, visit_side_pointer(visitor, visit_side_pointer(visitor, type_desc->u.side_array)->elem_type));
	if (visitor->callbacks->after_array_type_func)
		visitor->callbacks->after_array_type_func(visit_side_pointer(visitor, type_desc->u.side_array), visitor->priv);
}

static
void description_visitor_vla(const struct side_description_visitor *visitor, const struct side_type *type_desc)
{
	if (visitor->callbacks->before_vla_type_func)
		visitor->callbacks->before_vla_type_func(visit_side_pointer(visitor, type_desc->u.side_vla), visitor->priv);
	side_visit_elem(visitor, visit_side_pointer(visitor, visit_side_pointer(visitor, type_desc->u.side_vla)->length_type));
	if (visitor->callbacks->after_length_vla_type_func)
		visitor->callbacks->after_length_vla_type_func(visit_side_pointer(visitor, type_desc->u.side_vla), visitor->priv);
	side_visit_elem(visitor, visit_side_pointer(visitor, visit_side_pointer(visitor, type_desc->u.side_vla)->elem_type));
	if (visitor->callbacks->after_element_vla_type_func)
		visitor->callbacks->after_element_vla_type_func(visit_side_pointer(visitor, type_desc->u.side_vla), visitor->priv);
}

static
void description_visitor_vla_visitor(const struct side_description_visitor *visitor, const struct side_type *type_desc)
{
	if (visitor->callbacks->before_vla_visitor_type_func)
		visitor->callbacks->before_vla_visitor_type_func(visit_side_pointer(visitor, type_desc->u.side_vla_visitor), visitor->priv);
	side_visit_elem(visitor, visit_side_pointer(visitor, visit_side_pointer(visitor, type_desc->u.side_vla_visitor)->length_type));
	if (visitor->callbacks->after_length_vla_visitor_type_func)
		visitor->callbacks->after_length_vla_visitor_type_func(visit_side_pointer(visitor, type_desc->u.side_vla_visitor), visitor->priv);
	side_visit_elem(visitor, visit_side_pointer(visitor, visit_side_pointer(visitor, type_desc->u.side_vla_visitor)->elem_type));
	if (visitor->callbacks->after_element_vla_visitor_type_func)
		visitor->callbacks->after_element_vla_visitor_type_func(visit_side_pointer(visitor, type_desc->u.side_vla_visitor), visitor->priv);
}

static
void visit_gather_field(const struct side_description_visitor *visitor, const struct side_event_field *field)
{
	if (visitor->callbacks->before_field_func)
		visitor->callbacks->before_field_func(field, visitor->priv);
	(void) visit_gather_type(visitor, &field->side_type);
	if (visitor->callbacks->after_field_func)
		visitor->callbacks->after_field_func(field, visitor->priv);
}

static
void description_visitor_gather_struct(const struct side_description_visitor *visitor, const struct side_type_gather *type_gather)
{
	const struct side_type_gather_struct *side_gather_struct = &type_gather->u.side_struct;
	const struct side_type_struct *side_struct = visit_side_pointer(visitor, side_gather_struct->type);
	side_array_t(const struct side_event_field) fields = visit_side_array(visitor, &side_struct->fields);;
	uint32_t i;

	if (visitor->callbacks->before_gather_struct_type_func)
		visitor->callbacks->before_gather_struct_type_func(side_gather_struct, visitor->priv);
	for (i = 0; i < side_array_length(&fields); i++)
		visit_gather_field(visitor, side_array_at(&fields, i));
	if (visitor->callbacks->after_gather_struct_type_func)
		visitor->callbacks->after_gather_struct_type_func(side_gather_struct, visitor->priv);
}

static
void description_visitor_gather_array(const struct side_description_visitor *visitor, const struct side_type_gather *type_gather)
{
	const struct side_type_gather_array *side_gather_array = &type_gather->u.side_array;
	const struct side_type_array *side_array = &side_gather_array->type;
	const struct side_type *elem_type = visit_side_pointer(visitor, side_array->elem_type);

	if (visitor->callbacks->before_gather_array_type_func)
		visitor->callbacks->before_gather_array_type_func(side_gather_array, visitor->priv);
	switch (side_enum_get(elem_type->type)) {
	case SIDE_TYPE_GATHER_VLA:
		fprintf(stderr, "<gather VLA only supported within gather structures>\n");
		abort();
	default:
		break;
	}
	visit_gather_elem(visitor, elem_type);
	if (visitor->callbacks->after_gather_array_type_func)
		visitor->callbacks->after_gather_array_type_func(side_gather_array, visitor->priv);
}

static
void description_visitor_gather_vla(const struct side_description_visitor *visitor, const struct side_type_gather *type_gather)
{
	const struct side_type_gather_vla *side_gather_vla = &type_gather->u.side_vla;
	const struct side_type_vla *side_vla = &side_gather_vla->type;
	const struct side_type *length_type = visit_side_pointer(visitor, side_gather_vla->type.length_type);
	const struct side_type *elem_type = visit_side_pointer(visitor, side_vla->elem_type);

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
	if (visitor->callbacks->before_gather_vla_type_func)
		visitor->callbacks->before_gather_vla_type_func(side_gather_vla, visitor->priv);
	visit_gather_elem(visitor, length_type);
	if (visitor->callbacks->after_length_gather_vla_type_func)
		visitor->callbacks->after_length_gather_vla_type_func(side_gather_vla, visitor->priv);
	visit_gather_elem(visitor, elem_type);
	if (visitor->callbacks->after_element_gather_vla_type_func)
		visitor->callbacks->after_element_gather_vla_type_func(side_gather_vla, visitor->priv);
}

static
void description_visitor_gather_bool(const struct side_description_visitor *visitor, const struct side_type_gather *type_gather)
{
	if (visitor->callbacks->gather_bool_type_func)
		visitor->callbacks->gather_bool_type_func(&type_gather->u.side_bool, visitor->priv);
}

static
void description_visitor_gather_byte(const struct side_description_visitor *visitor, const struct side_type_gather *type_gather)
{
	if (visitor->callbacks->gather_byte_type_func)
		visitor->callbacks->gather_byte_type_func(&type_gather->u.side_byte, visitor->priv);
}

static
void description_visitor_gather_integer(const struct side_description_visitor *visitor,
					const struct side_type_gather *type_gather,
					enum side_type_label integer_type)
{
	switch (integer_type) {
	case SIDE_TYPE_GATHER_INTEGER:
		if (visitor->callbacks->gather_integer_type_func)
			visitor->callbacks->gather_integer_type_func(&type_gather->u.side_integer, visitor->priv);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		if (visitor->callbacks->gather_pointer_type_func)
			visitor->callbacks->gather_pointer_type_func(&type_gather->u.side_integer, visitor->priv);
		break;
	default:
		fprintf(stderr, "Unexpected integer type\n");
		abort();
	}
}

static
void description_visitor_gather_float(const struct side_description_visitor *visitor, const struct side_type_gather *type_gather)
{
	if (visitor->callbacks->gather_float_type_func)
		visitor->callbacks->gather_float_type_func(&type_gather->u.side_float, visitor->priv);
}

static
void description_visitor_gather_string(const struct side_description_visitor *visitor, const struct side_type_gather *type_gather)
{

	if (visitor->callbacks->gather_string_type_func)
		visitor->callbacks->gather_string_type_func(&type_gather->u.side_string, visitor->priv);
}

static
void visit_gather_type(const struct side_description_visitor *visitor, const struct side_type *type_desc)
{
	switch (side_enum_get(type_desc->type)) {
		/* Gather basic types */
	case SIDE_TYPE_GATHER_BOOL:
		description_visitor_gather_bool(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_INTEGER:
		description_visitor_gather_integer(visitor, &type_desc->u.side_gather, SIDE_TYPE_GATHER_INTEGER);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		description_visitor_gather_byte(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		description_visitor_gather_integer(visitor, &type_desc->u.side_gather, SIDE_TYPE_GATHER_POINTER);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		description_visitor_gather_float(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_STRING:
		description_visitor_gather_string(visitor, &type_desc->u.side_gather);
		break;

		/* Gather enumeration types */
	case SIDE_TYPE_GATHER_ENUM:
		description_visitor_gather_enum(visitor, &type_desc->u.side_gather);
		break;

		/* Gather compound types */
	case SIDE_TYPE_GATHER_STRUCT:
		description_visitor_gather_struct(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_ARRAY:
		description_visitor_gather_array(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_VLA:
		description_visitor_gather_vla(visitor, &type_desc->u.side_gather);
		break;
	default:
		fprintf(stderr, "<UNKNOWN GATHER TYPE>");
		abort();
	}
}

static
void visit_gather_elem(const struct side_description_visitor *visitor, const struct side_type *type_desc)
{
	if (visitor->callbacks->before_elem_func)
		visitor->callbacks->before_elem_func(type_desc, visitor->priv);
	visit_gather_type(visitor, type_desc);
	if (visitor->callbacks->after_elem_func)
		visitor->callbacks->after_elem_func(type_desc, visitor->priv);
}

static
void description_visitor_gather_enum(const struct side_description_visitor *visitor, const struct side_type_gather *type_gather)
{
	const struct side_type *elem_type = visit_side_pointer(visitor, type_gather->u.side_enum.elem_type);

	if (visitor->callbacks->before_gather_enum_type_func)
		visitor->callbacks->before_gather_enum_type_func(&type_gather->u.side_enum, visitor->priv);
	side_visit_elem(visitor, elem_type);
	if (visitor->callbacks->after_gather_enum_type_func)
		visitor->callbacks->after_gather_enum_type_func(&type_gather->u.side_enum, visitor->priv);
}

static
void side_visit_type(const struct side_description_visitor *visitor, const struct side_type *type_desc)
{
	enum side_type_label type = side_enum_get(type_desc->type);

	switch (type) {
		/* Stack-copy basic types */
	case SIDE_TYPE_NULL:
		if (visitor->callbacks->null_type_func)
			visitor->callbacks->null_type_func(&type_desc->u.side_null, visitor->priv);
		break;
	case SIDE_TYPE_BOOL:
		if (visitor->callbacks->bool_type_func)
			visitor->callbacks->bool_type_func(&type_desc->u.side_bool, visitor->priv);
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
		if (visitor->callbacks->integer_type_func)
			visitor->callbacks->integer_type_func(&type_desc->u.side_integer, visitor->priv);
		break;
	case SIDE_TYPE_BYTE:
		if (visitor->callbacks->byte_type_func)
			visitor->callbacks->byte_type_func(&type_desc->u.side_byte, visitor->priv);
		break;
	case SIDE_TYPE_POINTER:
		if (visitor->callbacks->pointer_type_func)
			visitor->callbacks->pointer_type_func(&type_desc->u.side_integer, visitor->priv);
		break;
	case SIDE_TYPE_FLOAT_BINARY16:	/* Fallthrough */
	case SIDE_TYPE_FLOAT_BINARY32:	/* Fallthrough */
	case SIDE_TYPE_FLOAT_BINARY64:	/* Fallthrough */
	case SIDE_TYPE_FLOAT_BINARY128:
		if (visitor->callbacks->float_type_func)
			visitor->callbacks->float_type_func(&type_desc->u.side_float, visitor->priv);
		break;
	case SIDE_TYPE_STRING_UTF8:	/* Fallthrough */
	case SIDE_TYPE_STRING_UTF16:	/* Fallthrough */
	case SIDE_TYPE_STRING_UTF32:
		if (visitor->callbacks->string_type_func)
			visitor->callbacks->string_type_func(&type_desc->u.side_string, visitor->priv);
		break;
	case SIDE_TYPE_ENUM:
		description_visitor_enum(visitor, &type_desc->u.side_enum);
		break;
	case SIDE_TYPE_ENUM_BITMAP:
		description_visitor_enum_bitmap(visitor, &type_desc->u.side_enum_bitmap);
		break;

		/* Stack-copy compound types */
	case SIDE_TYPE_STRUCT:
		description_visitor_struct(visitor, type_desc);
		break;
	case SIDE_TYPE_VARIANT:
		description_visitor_variant(visitor, type_desc);
		break;
	case SIDE_TYPE_ARRAY:
		description_visitor_array(visitor, type_desc);
		break;
	case SIDE_TYPE_VLA:
		description_visitor_vla(visitor, type_desc);
		break;
	case SIDE_TYPE_VLA_VISITOR:
		description_visitor_vla_visitor(visitor, type_desc);
		break;

		/* Gather basic types */
	case SIDE_TYPE_GATHER_BOOL:
		description_visitor_gather_bool(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_INTEGER:
		description_visitor_gather_integer(visitor, &type_desc->u.side_gather, SIDE_TYPE_GATHER_INTEGER);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		description_visitor_gather_byte(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		description_visitor_gather_integer(visitor, &type_desc->u.side_gather, SIDE_TYPE_GATHER_POINTER);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		description_visitor_gather_float(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_STRING:
		description_visitor_gather_string(visitor, &type_desc->u.side_gather);
		break;

		/* Gather compound type */
	case SIDE_TYPE_GATHER_STRUCT:
		description_visitor_gather_struct(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_ARRAY:
		description_visitor_gather_array(visitor, &type_desc->u.side_gather);
		break;
	case SIDE_TYPE_GATHER_VLA:
		description_visitor_gather_vla(visitor, &type_desc->u.side_gather);
		break;

		/* Gather enumeration types */
	case SIDE_TYPE_GATHER_ENUM:
		description_visitor_gather_enum(visitor, &type_desc->u.side_gather);
		break;

	/* Dynamic type */
	case SIDE_TYPE_DYNAMIC:
		if (visitor->callbacks->dynamic_type_func)
			visitor->callbacks->dynamic_type_func(type_desc, visitor->priv);
		break;

	case SIDE_TYPE_OPTIONAL:
		description_visitor_optional(visitor, visit_side_pointer(visitor, type_desc->u.side_optional));
		break;

	default:
		fprintf(stderr, "<UNKNOWN TYPE>\n");
		abort();
	}
}

void visit_event_description(const struct side_description_visitor *visitor,
			const struct side_event_description *event_desc)
{
	side_array_t(const struct side_event_field) fields = visit_side_array(visitor, &event_desc->fields);
	uint32_t i, len = side_array_length(&fields);

	if (visitor->callbacks->before_event_func)
		visitor->callbacks->before_event_func(event_desc, visitor->priv);
	if (len) {
		if (visitor->callbacks->before_static_fields_func)
			visitor->callbacks->before_static_fields_func(event_desc, visitor->priv);
		for (i = 0; i < len; i++)
			side_visit_field(visitor, side_array_at(&fields, i));
		if (visitor->callbacks->after_static_fields_func)
			visitor->callbacks->after_static_fields_func(event_desc, visitor->priv);
	}
	if (visitor->callbacks->after_event_func)
		visitor->callbacks->after_event_func(event_desc, visitor->priv);
}
