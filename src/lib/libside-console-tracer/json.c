// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

/*
 * Emission of the events as JSON, one object per line.
 *
 * A value is emitted as the JSON value it is: an integer as a number,
 * a string as a string, a structure as an object, an array as an
 * array. An object describing the value is only used where there is
 * more to say than the value itself: the labels of an enumeration, and
 * the selector of a variant.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <side/trace.h>

#include "libside-tools/json-writer.h"
#include "libside-tools/visit-arg-vec.h"

#include "common.h"
#include "json.h"

static _Thread_local struct side_json_writer json_writer;

static
struct side_json_writer *writer_of(void *priv)
{
	(void) priv;
	return &json_writer;
}

static
void json_print_string_value(struct side_json_writer *writer, const void *p,
		uint8_t unit_size, enum side_type_label_byte_order byte_order)
{
	char *utf8_str = NULL;

	if (!p) {
		side_json_raw(writer, "null");
		return;
	}
	tracer_convert_string_to_utf8(p, unit_size, byte_order, NULL, &utf8_str);
	side_json_string(writer, utf8_str);
	if (utf8_str != p)
		free(utf8_str);
}

static
void json_print_integer_value(struct side_json_writer *writer,
		const struct side_type_integer *type_integer,
		const union side_integer_value *value, uint16_t offset_bits)
{
	union int_value v;

	v = tracer_load_integer_value(type_integer, value, offset_bits, NULL);
	if (type_integer->signedness)
		side_json_raw(writer, "%" PRId64, v.s[SIDE_INTEGER128_SPLIT_LOW]);
	else
		side_json_raw(writer, "%" PRIu64, v.u[SIDE_INTEGER128_SPLIT_LOW]);
}

/*
 * The labels of an enumeration are more than its value: emit both.
 */
static
void json_print_enum_value(struct side_json_writer *writer,
		const struct side_enum_mappings *mappings,
		const struct side_type_integer *type_integer,
		const union side_integer_value *value)
{
	const struct side_enum_mapping *mapping;
	uint32_t nr_labels = 0;
	union int_value v;

	v = tracer_load_integer_value(type_integer, value, 0, NULL);
	side_json_raw(writer, "{");
	side_json_push(writer);
	if (type_integer->signedness)
		side_json_item(writer, "\"value\": %" PRId64, v.s[SIDE_INTEGER128_SPLIT_LOW]);
	else
		side_json_item(writer, "\"value\": %" PRIu64, v.u[SIDE_INTEGER128_SPLIT_LOW]);
	side_json_item(writer, "\"labels\": [");
	side_json_push(writer);
	side_for_each_element_in_array(mapping, &mappings->mappings) {
		if (mapping->range_end < mapping->range_begin)
			continue;
		if (v.s[SIDE_INTEGER128_SPLIT_LOW] < mapping->range_begin ||
				v.s[SIDE_INTEGER128_SPLIT_LOW] > mapping->range_end)
			continue;
		side_json_next(writer);
		json_print_string_value(writer, side_ptr_get(mapping->label.p),
			mapping->label.unit_size, side_enum_get(mapping->label.byte_order));
		nr_labels++;
	}
	(void) nr_labels;
	side_json_pop(writer, ']');
	side_json_pop(writer, '}');
}

/* Events. */

static
void json_before_event(const struct side_event_description *desc,
		const struct side_arg_vec *side_arg_vec __attribute__((unused)),
		const struct side_arg_dynamic_struct *var_struct __attribute__((unused)),
		void *caller_addr __attribute__((unused)), void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_writer_init(writer, stdout, true);
	side_json_raw(writer, "{");
	side_json_push(writer);
	side_json_item(writer, "\"provider\": ");
	side_json_string(writer, side_ptr_rel_get(desc->provider_name));
	side_json_item(writer, "\"event\": ");
	side_json_string(writer, side_ptr_rel_get(desc->event_name));
}

static
void json_after_event(const struct side_event_description *desc __attribute__((unused)),
		const struct side_arg_vec *side_arg_vec __attribute__((unused)),
		const struct side_arg_dynamic_struct *var_struct __attribute__((unused)),
		void *caller_addr __attribute__((unused)), void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_pop(writer, '}');
	fputc('\n', writer->out);
	fflush(writer->out);
}

static
void json_before_static_fields(const struct side_arg_vec *side_arg_vec __attribute__((unused)),
		void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_item(writer, "\"fields\": {");
	side_json_push(writer);
}

static
void json_after_static_fields(const struct side_arg_vec *side_arg_vec __attribute__((unused)),
		void *priv)
{
	side_json_pop(writer_of(priv), '}');
}

static
void json_before_variadic_fields(const struct side_arg_dynamic_struct *var_struct __attribute__((unused)),
		void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_item(writer, "\"variadic-fields\": {");
	side_json_push(writer);
}

static
void json_after_variadic_fields(const struct side_arg_dynamic_struct *var_struct __attribute__((unused)),
		void *priv)
{
	side_json_pop(writer_of(priv), '}');
}

static
void json_before_field(const struct side_event_field *item_desc, void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_next(writer);
	side_json_string(writer, side_ptr_get(item_desc->field_name));
	side_json_raw(writer, ": ");
}

static
void json_after_field(const struct side_event_field *item_desc __attribute__((unused)),
		void *priv __attribute__((unused)))
{
}

static
void json_before_elem(const struct side_type *type_desc __attribute__((unused)), void *priv)
{
	side_json_next(writer_of(priv));
}

static
void json_after_elem(const struct side_type *type_desc __attribute__((unused)),
		void *priv __attribute__((unused)))
{
}

/* Stack-copy basic types. */

static
void json_print_null(const struct side_type *type_desc __attribute__((unused)),
		const struct side_arg *item __attribute__((unused)), void *priv)
{
	side_json_raw(writer_of(priv), "null");
}

static
void json_print_bool(const struct side_type *type_desc, const struct side_arg *item, void *priv)
{
	struct side_json_writer *writer = writer_of(priv);
	const struct side_type_bool *type_bool = &type_desc->u.side_bool;
	uint64_t v;

	switch (type_bool->bool_size) {
	case 1:
		v = item->u.side_static.bool_value.side_bool8;
		break;
	case 2:
		v = item->u.side_static.bool_value.side_bool16;
		break;
	case 4:
		v = item->u.side_static.bool_value.side_bool32;
		break;
	case 8:
		v = item->u.side_static.bool_value.side_bool64;
		break;
	default:
		side_json_raw(writer, "null");
		return;
	}
	side_json_raw(writer, "%s", v ? "true" : "false");
}

static
void json_print_integer(const struct side_type *type_desc, const struct side_arg *item, void *priv)
{
	json_print_integer_value(writer_of(priv), &type_desc->u.side_integer,
		&item->u.side_static.integer_value, 0);
}

static
void json_print_byte(const struct side_type *type_desc __attribute__((unused)),
		const struct side_arg *item, void *priv)
{
	side_json_raw(writer_of(priv), "%" PRIu8, item->u.side_static.byte_value);
}

static
void json_print_float(const struct side_type *type_desc, const struct side_arg *item, void *priv)
{
	struct side_json_writer *writer = writer_of(priv);
	const struct side_type_float *type_float = &type_desc->u.side_float;

	switch (type_float->float_size) {
#if __HAVE_FLOAT32
	case 4:
		side_json_raw(writer, "%g", (double) item->u.side_static.float_value.side_float_binary32);
		return;
#endif
#if __HAVE_FLOAT64
	case 8:
		side_json_raw(writer, "%g", (double) item->u.side_static.float_value.side_float_binary64);
		return;
#endif
	default:
		side_json_raw(writer, "null");
		return;
	}
}

static
void json_print_string(const struct side_type *type_desc, const struct side_arg *item, void *priv)
{
	const struct side_type_string *type_string = &type_desc->u.side_string;

	json_print_string_value(writer_of(priv),
		side_ptr_get(item->u.side_static.string_value),
		type_string->unit_size, side_enum_get(type_string->byte_order));
}

/* Stack-copy compound types. */

static
void json_before_struct(const struct side_type_struct *side_struct __attribute__((unused)),
		const struct side_arg_vec *side_arg_vec __attribute__((unused)), void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_raw(writer, "{");
	side_json_push(writer);
}

static
void json_after_struct(const struct side_type_struct *side_struct __attribute__((unused)),
		const struct side_arg_vec *side_arg_vec __attribute__((unused)), void *priv)
{
	side_json_pop(writer_of(priv), '}');
}

static
void json_before_array(const struct side_type_array *side_array __attribute__((unused)),
		const struct side_arg_vec *side_arg_vec __attribute__((unused)), void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_raw(writer, "[");
	side_json_push(writer);
}

static
void json_after_array(const struct side_type_array *side_array __attribute__((unused)),
		const struct side_arg_vec *side_arg_vec __attribute__((unused)), void *priv)
{
	side_json_pop(writer_of(priv), ']');
}

static
void json_before_vla(const struct side_type_vla *side_vla __attribute__((unused)),
		const struct side_arg_vec *side_arg_vec __attribute__((unused)), void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_raw(writer, "[");
	side_json_push(writer);
}

static
void json_after_vla(const struct side_type_vla *side_vla __attribute__((unused)),
		const struct side_arg_vec *side_arg_vec __attribute__((unused)), void *priv)
{
	side_json_pop(writer_of(priv), ']');
}

/*
 * The selector of a variant is more than the value of its option: emit
 * both, the option as the value.
 */
static
void json_before_variant(const struct side_type_variant *side_type_variant,
		const struct side_arg_variant *side_arg_variant, void *priv)
{
	struct side_json_writer *writer = writer_of(priv);
	const struct side_type *selector_type = &side_type_variant->selector;

	side_json_raw(writer, "{");
	side_json_push(writer);
	side_json_item(writer, "\"selector\": ");
	json_print_integer_value(writer, &selector_type->u.side_integer,
		&side_arg_variant->selector.u.side_static.integer_value, 0);
	side_json_item(writer, "\"value\": ");
}

static
void json_after_variant(const struct side_type_variant *side_type_variant __attribute__((unused)),
		const struct side_arg_variant *side_arg_variant __attribute__((unused)), void *priv)
{
	side_json_pop(writer_of(priv), '}');
}

/* Stack-copy enumeration types. */

static
void json_print_enum(const struct side_type *type_desc, const struct side_arg *item, void *priv)
{
	const struct side_type_enum *side_enum = &type_desc->u.side_enum;
	const struct side_type *elem_type = side_ptr_get(side_enum->elem_type);

	json_print_enum_value(writer_of(priv), side_ptr_get(side_enum->mappings),
		&elem_type->u.side_integer, &item->u.side_static.integer_value);
}

static
void json_print_enum_bitmap(const struct side_type *type_desc __attribute__((unused)),
		const struct side_arg *item __attribute__((unused)), void *priv)
{
	/* Bitmaps are not described yet. */
	side_json_raw(writer_of(priv), "null");
}

/* Gather types: the values are gathered from memory, and emitted alike. */

static
void json_print_gather_bool(const struct side_type_gather_bool *type,
		const union side_bool_value *value, void *priv)
{
	struct side_json_writer *writer = writer_of(priv);
	uint64_t v;

	switch (type->type.bool_size) {
	case 1:
		v = value->side_bool8;
		break;
	case 2:
		v = value->side_bool16;
		break;
	case 4:
		v = value->side_bool32;
		break;
	case 8:
		v = value->side_bool64;
		break;
	default:
		side_json_raw(writer, "null");
		return;
	}
	side_json_raw(writer, "%s", v ? "true" : "false");
}

static
void json_print_gather_byte(const struct side_type_gather_byte *type __attribute__((unused)),
		const uint8_t *_ptr, void *priv)
{
	side_json_raw(writer_of(priv), "%" PRIu8, *_ptr);
}

static
void json_print_gather_integer(const struct side_type_gather_integer *type,
		const union side_integer_value *value, void *priv)
{
	json_print_integer_value(writer_of(priv), &type->type, value, type->offset_bits);
}

static
void json_print_gather_float(const struct side_type_gather_float *type,
		const union side_float_value *value, void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	switch (type->type.float_size) {
#if __HAVE_FLOAT32
	case 4:
		side_json_raw(writer, "%g", (double) value->side_float_binary32);
		return;
#endif
#if __HAVE_FLOAT64
	case 8:
		side_json_raw(writer, "%g", (double) value->side_float_binary64);
		return;
#endif
	default:
		side_json_raw(writer, "null");
		return;
	}
}

static
void json_print_gather_string(const struct side_type_gather_string *type __attribute__((unused)),
		const void *p, uint8_t unit_size, enum side_type_label_byte_order byte_order,
		size_t strlen_with_null __attribute__((unused)), void *priv)
{
	json_print_string_value(writer_of(priv), p, unit_size, byte_order);
}

static
void json_print_gather_enum(const struct side_type_gather_enum *type,
		const union side_integer_value *value, void *priv)
{
	const struct side_type *elem_type = side_ptr_get(type->elem_type);

	json_print_enum_value(writer_of(priv), side_ptr_get(type->mappings),
		&elem_type->u.side_gather.u.side_integer.type, value);
}

static
void json_before_gather_struct(const struct side_type_struct *side_struct __attribute__((unused)),
		void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_raw(writer, "{");
	side_json_push(writer);
}

static
void json_after_gather_struct(const struct side_type_struct *side_struct __attribute__((unused)),
		void *priv)
{
	side_json_pop(writer_of(priv), '}');
}

static
void json_before_gather_array(const struct side_type_array *side_array __attribute__((unused)),
		void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_raw(writer, "[");
	side_json_push(writer);
}

static
void json_after_gather_array(const struct side_type_array *side_array __attribute__((unused)),
		void *priv)
{
	side_json_pop(writer_of(priv), ']');
}

static
void json_before_gather_vla(const struct side_type_vla *side_vla __attribute__((unused)),
		uint32_t length __attribute__((unused)), void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_raw(writer, "[");
	side_json_push(writer);
}

static
void json_after_gather_vla(const struct side_type_vla *side_vla __attribute__((unused)),
		uint32_t length __attribute__((unused)), void *priv)
{
	side_json_pop(writer_of(priv), ']');
}

/* Dynamic types: the value describes itself. */

static
void json_before_dynamic_field(const struct side_arg_dynamic_field *field, void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_next(writer);
	side_json_string(writer, side_ptr_get(field->field_name));
	side_json_raw(writer, ": ");
}

static
void json_after_dynamic_field(const struct side_arg_dynamic_field *field __attribute__((unused)),
		void *priv __attribute__((unused)))
{
}

static
void json_before_dynamic_elem(const struct side_arg *dynamic_item __attribute__((unused)), void *priv)
{
	side_json_next(writer_of(priv));
}

static
void json_after_dynamic_elem(const struct side_arg *dynamic_item __attribute__((unused)),
		void *priv __attribute__((unused)))
{
}

static
void json_print_dynamic_null(const struct side_arg *item __attribute__((unused)), void *priv)
{
	side_json_raw(writer_of(priv), "null");
}

static
void json_print_dynamic_bool(const struct side_arg *item, void *priv)
{
	side_json_raw(writer_of(priv), "%s",
		item->u.side_dynamic.side_bool.value.side_bool8 ? "true" : "false");
}

static
void json_print_dynamic_integer(const struct side_arg *item, void *priv)
{
	json_print_integer_value(writer_of(priv), &item->u.side_dynamic.side_integer.type,
		&item->u.side_dynamic.side_integer.value, 0);
}

static
void json_print_dynamic_byte(const struct side_arg *item, void *priv)
{
	side_json_raw(writer_of(priv), "%" PRIu8, item->u.side_dynamic.side_byte.value);
}

static
void json_print_dynamic_float(const struct side_arg *item, void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	switch (item->u.side_dynamic.side_float.type.float_size) {
#if __HAVE_FLOAT32
	case 4:
		side_json_raw(writer, "%g",
			(double) item->u.side_dynamic.side_float.value.side_float_binary32);
		return;
#endif
#if __HAVE_FLOAT64
	case 8:
		side_json_raw(writer, "%g",
			(double) item->u.side_dynamic.side_float.value.side_float_binary64);
		return;
#endif
	default:
		side_json_raw(writer, "null");
		return;
	}
}

static
void json_print_dynamic_string(const struct side_arg *item, void *priv)
{
	json_print_string_value(writer_of(priv),
		(const void *) (uintptr_t) item->u.side_dynamic.side_string.value,
		item->u.side_dynamic.side_string.type.unit_size,
		side_enum_get(item->u.side_dynamic.side_string.type.byte_order));
}

static
void json_before_dynamic_struct(const struct side_arg_dynamic_struct *dynamic_struct __attribute__((unused)),
		void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_raw(writer, "{");
	side_json_push(writer);
}

static
void json_after_dynamic_struct(const struct side_arg_dynamic_struct *dynamic_struct __attribute__((unused)),
		void *priv)
{
	side_json_pop(writer_of(priv), '}');
}

static
void json_before_dynamic_vla(const struct side_arg_dynamic_vla *vla __attribute__((unused)), void *priv)
{
	struct side_json_writer *writer = writer_of(priv);

	side_json_raw(writer, "[");
	side_json_push(writer);
}

static
void json_after_dynamic_vla(const struct side_arg_dynamic_vla *vla __attribute__((unused)), void *priv)
{
	side_json_pop(writer_of(priv), ']');
}

const struct side_type_visitor json_type_visitor = {
	.before_event_func = json_before_event,
	.after_event_func = json_after_event,
	.before_static_fields_func = json_before_static_fields,
	.after_static_fields_func = json_after_static_fields,
	.before_variadic_fields_func = json_before_variadic_fields,
	.after_variadic_fields_func = json_after_variadic_fields,

	/* Stack-copy basic types. */
	.before_field_func = json_before_field,
	.after_field_func = json_after_field,
	.before_elem_func = json_before_elem,
	.after_elem_func = json_after_elem,
	.null_type_func = json_print_null,
	.bool_type_func = json_print_bool,
	.integer_type_func = json_print_integer,
	.byte_type_func = json_print_byte,
	.pointer_type_func = json_print_integer,
	.float_type_func = json_print_float,
	.string_type_func = json_print_string,

	/* Stack-copy compound types. */
	.before_struct_type_func = json_before_struct,
	.after_struct_type_func = json_after_struct,
	.before_array_type_func = json_before_array,
	.after_array_type_func = json_after_array,
	.before_vla_type_func = json_before_vla,
	.after_vla_type_func = json_after_vla,
	.before_variant_type_func = json_before_variant,
	.after_variant_type_func = json_after_variant,

	/* Stack-copy enumeration types. */
	.enum_type_func = json_print_enum,
	.enum_bitmap_type_func = json_print_enum_bitmap,

	/* Gather basic types. */
	.gather_bool_type_func = json_print_gather_bool,
	.gather_byte_type_func = json_print_gather_byte,
	.gather_integer_type_func = json_print_gather_integer,
	.gather_pointer_type_func = json_print_gather_integer,
	.gather_float_type_func = json_print_gather_float,
	.gather_string_type_func = json_print_gather_string,

	/* Gather compound types. */
	.before_gather_struct_type_func = json_before_gather_struct,
	.after_gather_struct_type_func = json_after_gather_struct,
	.before_gather_array_type_func = json_before_gather_array,
	.after_gather_array_type_func = json_after_gather_array,
	.before_gather_vla_type_func = json_before_gather_vla,
	.after_gather_vla_type_func = json_after_gather_vla,

	/* Gather enumeration types. */
	.gather_enum_type_func = json_print_gather_enum,

	/* Dynamic basic types. */
	.before_dynamic_field_func = json_before_dynamic_field,
	.after_dynamic_field_func = json_after_dynamic_field,
	.before_dynamic_elem_func = json_before_dynamic_elem,
	.after_dynamic_elem_func = json_after_dynamic_elem,

	.dynamic_null_func = json_print_dynamic_null,
	.dynamic_bool_func = json_print_dynamic_bool,
	.dynamic_integer_func = json_print_dynamic_integer,
	.dynamic_byte_func = json_print_dynamic_byte,
	.dynamic_pointer_func = json_print_dynamic_integer,
	.dynamic_float_func = json_print_dynamic_float,
	.dynamic_string_func = json_print_dynamic_string,

	/* Dynamic compound types. */
	.before_dynamic_struct_func = json_before_dynamic_struct,
	.after_dynamic_struct_func = json_after_dynamic_struct,
	.before_dynamic_vla_func = json_before_dynamic_vla,
	.after_dynamic_vla_func = json_after_dynamic_vla,
};

/*
 * The events of a notification, as one object per line: what they are
 * called, and whether they were inserted or removed. The exhaustive
 * description of their types is what readside emits.
 */
void json_print_event_notification(enum side_tracer_notification notif,
		struct side_event_description **events, uint32_t nr_events)
{
	struct side_json_writer *writer = &json_writer;
	uint32_t i;

	side_json_writer_init(writer, stdout, true);
	side_json_raw(writer, "{");
	side_json_push(writer);
	side_json_item(writer, "\"notification\": ");
	side_json_string(writer,
		notif == SIDE_TRACER_NOTIFICATION_INSERT_EVENTS ? "insert" : "remove");
	side_json_item(writer, "\"events\": [");
	side_json_push(writer);
	for (i = 0; i < nr_events; i++) {
		const struct side_event_description *desc = events[i];

		if (!desc)
			continue;
		side_json_next(writer);
		side_json_raw(writer, "{");
		side_json_push(writer);
		side_json_item(writer, "\"provider\": ");
		side_json_string(writer, side_ptr_rel_get(desc->provider_name));
		side_json_item(writer, "\"event\": ");
		side_json_string(writer, side_ptr_rel_get(desc->event_name));
		side_json_item(writer, "\"loglevel\": ");
		side_json_string(writer, side_loglevel_to_string(side_enum_get(desc->loglevel)));
		side_json_attributes(writer, side_array_rel_elements(&desc->attributes),
			side_array_length(&desc->attributes));
		side_json_pop(writer, '}');
	}
	side_json_pop(writer, ']');
	side_json_pop(writer, '}');
	fputc('\n', writer->out);
	fflush(writer->out);
}
