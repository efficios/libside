// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include <stdlib.h>
#include <stdio.h>

#include "libside-tools/visit-description.h"

#include "visitors/common.h"

struct json_context {
	struct side_json_writer writer;
};

/*
 * The writer of this context. The resolution of the pointers of a
 * description is that of the reader: the descriptions are read from a
 * file, and are mapped elsewhere.
 */
static struct side_json_writer *writer_of(const struct visitor_context *ctx)
{
	struct json_context *jctx = ctx->context;

	jctx->writer.resolve = ctx->resolve;
	jctx->writer.resolve_priv = (void *) ctx;
	return &jctx->writer;
}

#define printf_nest(ctx, fmt, ...)					\
	side_json_item(writer_of(ctx), fmt, ##__VA_ARGS__)

static inline void push_nest(struct visitor_context *ctx)
{
	side_json_push(writer_of(ctx));
}

static inline void pop_nest(struct visitor_context *ctx)
{
	side_json_pop(writer_of(ctx), '}');
}

static void print_attributes(const struct side_attr *attr,
			size_t nr_attr,
			void *ctx)
{
	side_json_attributes(writer_of(ctx), attr, nr_attr);
}

static void print_type_attributes(const struct side_type *type, void *ctx)
{
	const struct side_attr *attrs = NULL;
	u32 nr_attr = 0;
	side_type_attributes(type, ctx,
			&attrs, &nr_attr) ;
	print_attributes(attrs, nr_attr, ctx);
}

static void begin_event(const struct side_event_description *desc, void *ctx)
{
	const u32 *state_version = (const u32 *)visit_side_pointer(ctx, desc->state);

	printf_nest(ctx, "{");
	push_nest(ctx);
	printf_nest(ctx, "\"version\": %" PRIu32, desc->version);
	printf_nest(ctx, "\"state-version\": %" PRId64, state_version ? cast(s64, *state_version) : -1);
	side_json_item_string(writer_of(ctx), "provider",
		cast(char *, visit_side_pointer(ctx, desc->provider_name)));
	side_json_item_string(writer_of(ctx), "event",
		cast(char *, visit_side_pointer(ctx, desc->event_name)));
	printf_nest(ctx, "\"loglevel\": \"%s\"", side_loglevel_to_string(side_enum_get(desc->loglevel)));
	print_attributes(desc->attributes.length ? visit_side_pointer(ctx, desc->attributes.elements) : NULL,
			desc->attributes.length,
			ctx);
}

static void end_event(const struct side_event_description *desc, void *ctx)
{
	(void) desc;

	pop_nest(ctx);
}

static void begin_field(const struct side_event_field *field, void *ctx)
{
	const char *field_name = visit_side_pointer(ctx, field->field_name);
	const struct side_type *side_type = &field->side_type;

	side_json_item_name(writer_of(ctx), field_name);
	side_json_raw(writer_of(ctx), "{");
	push_nest(ctx);
	printf_nest(ctx, "\"type\": \"%s\"", side_type_to_string(side_enum_get(side_type->type)));
	print_type_attributes(side_type, ctx);
}

static void end_field(const struct side_event_field *field, void *ctx)
{
	(void) field;

	pop_nest(ctx);

}

static void begin_event_fields(const struct side_event_description *desc, void *ctx)
{
	(void) desc;

	printf_nest(ctx, "\"fields\": {");
	push_nest(ctx);
}

static void end_event_fields(const struct side_event_description *desc, void *ctx)
{
	(void) desc;
	pop_nest(ctx);
}

static void begin_elem_type(const struct side_type *side_type, void *ctx)
{
	printf_nest(ctx, "\"element\": {");
	push_nest(ctx);
	printf_nest(ctx, "\"type\": \"%s\"", side_type_to_string(side_enum_get(side_type->type)));
	print_type_attributes(side_type, ctx);
}

static void end_elem_type(const struct side_type *side_type, void *ctx)
{
	(void) side_type;

	pop_nest(ctx);
}

static void print_null_type_json(const struct side_type_null *type, void *ctx)
{
	(void) type;
	(void) ctx;
}

static void print_bool_type_json(const struct side_type_bool *type, void *ctx)
{
	printf_nest(ctx, "\"bool-size\": %" PRIu16, type->bool_size);
	printf_nest(ctx, "\"len-bits\": %" PRIu16, type->len_bits);
	printf_nest(ctx, "\"byte-order\": \"%s\"",
		side_byte_order_to_string(side_enum_get(type->byte_order)));
}

static void print_integer_type_json(const struct side_type_integer *type, void *ctx)
{
	printf_nest(ctx, "\"integer-size\": %" PRIu16, type->integer_size);
	printf_nest(ctx, "\"len-bits\": %" PRIu16, type->len_bits);
	printf_nest(ctx, "\"signed\": %s", type->signedness ? "true" : "false");
	printf_nest(ctx, "\"byte-order\": \"%s\"",
		side_byte_order_to_string(side_enum_get(type->byte_order)));
}

static void print_byte_type_json(const struct side_type_byte *type, void *ctx)
{
	(void) type;
	(void) ctx;
}

static void print_float_type_json(const struct side_type_float *type, void *ctx)
{
	printf_nest(ctx, "\"float-size\": %" PRIu16, type->float_size);
	printf_nest(ctx, "\"byte-order\": \"%s\"",
		side_byte_order_to_string(side_enum_get(type->byte_order)));
}

static void print_string_type_json(const struct side_type_string *type, void *ctx)
{
	printf_nest(ctx, "\"unit-size\": %" PRIu16, type->unit_size);
	printf_nest(ctx, "\"byte-order\": \"%s\"",
		side_byte_order_to_string(side_enum_get(type->byte_order)));
}

static void print_gather_bool_type_json(const struct side_type_gather_bool *type, void *ctx)
{
	printf_nest(ctx, "\"offset\": %" PRIu64, type->offset);
	printf_nest(ctx, "\"offset-bits\": %" PRIu16, type->offset_bits);
	printf_nest(ctx, "\"access-mode\": \"%s\"",
		side_access_mode_to_string(side_enum_get(type->access_mode)));
	printf_nest(ctx, "\"gather\": {");
	push_nest(ctx); {
		print_bool_type_json(&type->type, ctx);
	} pop_nest(ctx);
}

static void print_gather_integer_type_json(const struct side_type_gather_integer *type, void *ctx)
{
	printf_nest(ctx, "\"offset\": %" PRIu64, type->offset);
	printf_nest(ctx, "\"offset-bits\": %" PRIu16, type->offset_bits);
	printf_nest(ctx, "\"access-mode\": \"%s\"",
		side_access_mode_to_string(side_enum_get(type->access_mode)));
	printf_nest(ctx, "\"gather\": {");
	push_nest(ctx); {
		print_integer_type_json(&type->type, ctx);
	} pop_nest(ctx);
}

static void print_gather_byte_type_json(const struct side_type_gather_byte *type, void *ctx)
{
	printf_nest(ctx, "\"offset\": %" PRIu64, type->offset);
	printf_nest(ctx, "\"access-mode\": \"%s\"",
		side_access_mode_to_string(side_enum_get(type->access_mode)));
	printf_nest(ctx, "\"gather\": {");
	push_nest(ctx); {
		print_byte_type_json(&type->type, ctx);
	} pop_nest(ctx);
}

static void print_gather_float_type_json(const struct side_type_gather_float *type, void *ctx)
{
	printf_nest(ctx, "\"offset\": %" PRIu64, type->offset);
	printf_nest(ctx, "\"access-mode\": \"%s\"",
		side_access_mode_to_string(side_enum_get(type->access_mode)));
	printf_nest(ctx, "\"gather\": {");
	push_nest(ctx); {
		print_float_type_json(&type->type, ctx);
	} pop_nest(ctx);
}

static void print_gather_string_type_json(const struct side_type_gather_string *type, void *ctx)
{
	printf_nest(ctx, "\"offset\": %" PRIu64, type->offset);
	printf_nest(ctx, "\"access-mode\": \"%s\"",
		side_access_mode_to_string(side_enum_get(type->access_mode)));
	printf_nest(ctx, "\"gather\": {");
	push_nest(ctx); {
		print_string_type_json(&type->type, ctx);
	} pop_nest(ctx);
}

static void begin_struct(const struct side_type_struct *side_struct, void *ctx)
{
	(void) side_struct;

	printf_nest(ctx, "\"fields\": {");
	push_nest(ctx);
}

static void end_struct(const struct side_type_struct *side_struct, void *ctx)
{
	(void) side_struct;

	pop_nest(ctx);
}

static void begin_gather_struct(const struct side_type_gather_struct *type, void *ctx)
{
	printf_nest(ctx, "\"offset\": %" PRIu64, type->offset);
	printf_nest(ctx, "\"size\": %" PRIu32, type->size);
	printf_nest(ctx, "\"access-mode\": \"%s\"",
		side_access_mode_to_string(side_enum_get(type->access_mode)));
	printf_nest(ctx, "\"gather\": {");
	push_nest(ctx);
	begin_struct(visit_side_pointer(ctx, type->type), ctx);
}

static void end_gather_struct(const struct side_type_gather_struct *type, void *ctx)
{
	end_struct(visit_side_pointer(ctx, type->type), ctx);
	pop_nest(ctx);
}

static void begin_array(const struct side_type_array *side_array, void *ctx)
{
	printf_nest(ctx, "\"length\": %" PRIu32, side_array->length);
}

static void begin_gather_array(const struct side_type_gather_array *type, void *ctx)
{
	printf_nest(ctx, "\"offset\": %" PRIu64, type->offset);
	printf_nest(ctx, "\"access-mode\": \"%s\"",
		side_access_mode_to_string(side_enum_get(type->access_mode)));
	printf_nest(ctx, "\"gather\": {");
	push_nest(ctx);
	begin_array(&type->type, ctx);
}

static void end_gather_array(const struct side_type_gather_array *type, void *ctx)
{
	(void) type;

	pop_nest(ctx);
}

static void begin_vla(const struct side_type_vla *vla, void *ctx)
{
	(void) vla;

	printf_nest(ctx, "\"length\": {");
	push_nest(ctx);
}

static void after_vla_length(const struct side_type_vla *vla, void *ctx)
{
	(void) vla;

	pop_nest(ctx);
}

static void begin_gather_vla(const struct side_type_gather_vla *type, void *ctx)
{
	printf_nest(ctx, "\"offset\": %" PRIu64, type->offset);
	printf_nest(ctx, "\"access-mode\": \"%s\"",
		side_access_mode_to_string(side_enum_get(type->access_mode)));
	printf_nest(ctx, "\"gather\": {");
	push_nest(ctx);
	begin_vla(&type->type, ctx);
}

static void after_gather_vla_length(const struct side_type_gather_vla *type, void *ctx)
{
	after_vla_length(&type->type, ctx);
}

static void after_gather_vla_element(const struct side_type_gather_vla *type, void *ctx)
{
	(void) type;

	pop_nest(ctx);
}

static void begin_variant(const struct side_type_variant *variant, void *ctx)
{
	const struct side_type *selector = &variant->selector;

	printf_nest(ctx, "\"selector\": {");
	push_nest(ctx);
	printf_nest(ctx, "\"type\": \"%s\"", side_type_to_string(side_enum_get(selector->type)));
	print_type_attributes(selector, ctx);
}

static void after_variant_selector(const struct side_type *type, void *ctx)
{
	(void) type;

	pop_nest(ctx);
	printf_nest(ctx, "\"options\": {");
	push_nest(ctx);
}

static void end_variant(const struct side_type_variant *variant, void *ctx)
{
	(void) variant;

	pop_nest(ctx);
}

static void begin_option(const struct side_variant_option *option, void *ctx)
{
	const struct side_type *option_type = &option->side_type;

	printf_nest(ctx, "\"%" PRId64 "-%" PRId64 "\": {",
		option->range_begin, option->range_end);
	push_nest(ctx);
	printf_nest(ctx, "\"type\": \"%s\"", side_type_to_string(side_enum_get(option_type->type)));
	print_type_attributes(option_type, ctx);
}

static void end_option(const struct side_variant_option *option, void *ctx)
{
	(void) option;
	pop_nest(ctx);
}

static void print_enum_mapping(const struct side_enum_mapping *map, void *ctx)
{
	if (!map) {
		return;
	}

	/* FIXME: Decode raw string. */
	side_json_item_name(writer_of(ctx), cast(char *, visit_side_pointer(ctx, map->label.p)));
	side_json_raw(writer_of(ctx), "[%" PRId64 ", %" PRId64 "]",
		map->range_begin, map->range_end);
}

static void print_enum_bitmap_mapping(const struct side_enum_bitmap_mapping *map, void *ctx)
{
	if (!map) {
		return;
	}

	/* FIXME: Decode raw string. */
	side_json_item_name(writer_of(ctx), cast(char *, visit_side_pointer(ctx, map->label.p)));
	side_json_raw(writer_of(ctx), "[%" PRId64 ", %" PRId64 "]",
		map->range_begin, map->range_end);
}

static void print_enum_mappings(const struct side_enum_mappings *mappings,
				void *ctx)
{
	if (!mappings) {
		return;
	}

	const struct side_enum_mapping *maps =
		visit_side_pointer(ctx, mappings->mappings.elements);

	printf_nest(ctx, "\"mappings\": {");
	push_nest(ctx);
	if (maps) {
		for (size_t k = 0; k < mappings->mappings.length; ++k) {
			print_enum_mapping(&maps[k], ctx);
		}
	}
	pop_nest(ctx);
}

static void begin_enum(const struct side_type_enum *type, void *ctx)
{
	print_enum_mappings(visit_side_pointer(ctx, type->mappings), ctx);
}

static void begin_enum_bitmap(const struct side_type_enum_bitmap *type, void *ctx)
{
	printf_nest(ctx, "\"mappings\": {");
	push_nest(ctx);
	{
		const struct side_enum_bitmap_mappings *mappings =
			visit_side_pointer(ctx, type->mappings);

		if (!mappings) {
			goto out;
		}

		const struct side_enum_bitmap_mapping *maps =
			visit_side_pointer(ctx, mappings->mappings.elements);

		if (!maps) {
			goto out;
		}

		for (size_t k = 0; k < mappings->mappings.length; ++k) {
			print_enum_bitmap_mapping(&maps[k], ctx);
		}

	}
out:
	pop_nest(ctx);
}

static void begin_gather_enum(const struct side_type_gather_enum *type, void *ctx)
{
	print_enum_mappings(visit_side_pointer(ctx, type->mappings), ctx);
	printf_nest(ctx, "\"gather\": {");
	push_nest(ctx);
}

static void end_gather_enum(const struct side_type_gather_enum *type, void *ctx)
{
	(void) type;

	pop_nest(ctx);
}

static void begin_json(struct visitor_context *ctx)
{
	struct side_json_writer *writer = writer_of(ctx);

	side_json_writer_init(writer, stdout, false);
	/* The descriptions are the elements of an array. */
	writer->nesting = 1;
	printf("[");
}

static void end_json(struct visitor_context *ctx)
{
	struct side_json_writer *writer = writer_of(ctx);

	writer->nesting = 0;
	writer->first_element = false;
	printf("\n]\n");
}

static void *make_json_context(void)
{
	struct json_context *ctx = xcalloc(1, sizeof(struct json_context));

	return ctx;
}

static void drop_json_context(void *raw_ctx)
{
	xfree(raw_ctx);
}

struct visitor json_visitor = {
	.description = {

		/* Events. */
		.before_event_func = begin_event,
		.after_event_func  = end_event,

		/* Fields */
		.before_static_fields_func = begin_event_fields,
		.after_static_fields_func  = end_event_fields,
		.before_field_func         = begin_field,
		.after_field_func          = end_field,

		/* Elements. */
		.before_elem_func = begin_elem_type,
		.after_elem_func  = end_elem_type,

		/* Options. */
		.before_option_func = begin_option,
		.after_option_func  = end_option,

		/* Basic types. */
		.null_type_func	   =  print_null_type_json,
		.bool_type_func	   =  print_bool_type_json,
		.integer_type_func =  print_integer_type_json,
		.byte_type_func    =  print_byte_type_json,
		.pointer_type_func =  print_integer_type_json,
		.float_type_func   =  print_float_type_json,
		.string_type_func  =  print_string_type_json,

		/* Compound types. */
		.before_struct_type_func = begin_struct,
		.after_struct_type_func  = end_struct,

		.before_variant_type_func         = begin_variant,
		.after_variant_selector_type_func = after_variant_selector,
		.after_variant_type_func          = end_variant,

		.before_array_type_func = begin_array,
		// .after_array_type_func

		.before_vla_type_func       = begin_vla,
		.after_length_vla_type_func = after_vla_length,
		// .after_element_vla_type_func

		// .before_optional_type_func
		// .after_optional_type_func

		.before_enum_type_func = begin_enum,
		// .after_enum_type_func

		.before_enum_bitmap_type_func = begin_enum_bitmap,
		// .after_enum_bitmap_type_func

		/* Basic gather types. */
		.gather_bool_type_func	  =  print_gather_bool_type_json,
		.gather_integer_type_func =  print_gather_integer_type_json,
		.gather_byte_type_func    =  print_gather_byte_type_json,
		.gather_pointer_type_func =  print_gather_integer_type_json,
		.gather_float_type_func   =  print_gather_float_type_json,
		.gather_string_type_func  =  print_gather_string_type_json,

		/* Compound gather types. */
		.before_gather_struct_type_func = begin_gather_struct,
		.after_gather_struct_type_func  = end_gather_struct,

		.before_gather_array_type_func = begin_gather_array,
		.after_gather_array_type_func  = end_gather_array,

		.before_gather_vla_type_func        = begin_gather_vla,
		.after_length_gather_vla_type_func  = after_gather_vla_length,
		.after_element_gather_vla_type_func = after_gather_vla_element,

		/* Gather enumeration types. */
		.before_gather_enum_type_func = begin_gather_enum,
		.after_gather_enum_type_func  = end_gather_enum,

		/* Dynamic types. */
		// .dynamic_type_func
	},
	.begin = begin_json,
	.end = end_json,
	.make_context = make_json_context,
	.drop_context = drop_json_context,
};
