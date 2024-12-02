// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include <stdlib.h>
#include <stdio.h>

#include "libside-tools/visit-description.h"

#include "visitors/common.h"

struct json_context {
	bool first_element;
};

static void nest_indent(const struct visitor_context *ctx)
{
	struct json_context *jctx = ctx->context;

	if (jctx->first_element) {
		putchar('\n');
		jctx->first_element = false;
	} else {
		putchar(',');
		putchar('\n');
	}

	for (size_t k = 0; k < ctx->nesting; ++k) {
		putchar('\t');
	}
}

#define printf_nest(ctx, fmt, ...)		\
	do {					\
		nest_indent(ctx);		\
		printf(fmt, ##__VA_ARGS__);	\
	} while(0)

static inline void push_nest(struct visitor_context *ctx)
{
	struct json_context *jctx = ctx->context;

	ctx->nesting++;

	jctx->first_element = true;
}

static inline void pop_nest(struct visitor_context *ctx)
{
	struct json_context *jctx = ctx->context;

	ctx->nesting--;

	jctx->first_element = false;

	putchar('\n');
	for (size_t k = 0; k < ctx->nesting; ++k) {
		putchar('\t');
	}
	putchar('}');
}

static const char *side_attr_value_to_string(const struct side_attr_value *value,
					void *ctx)
{
	_Thread_local static char buffer[4096];

	switch (side_enum_get(value->type)) {
	case SIDE_ATTR_TYPE_NULL:
		return "null";
	case SIDE_ATTR_TYPE_BOOL:
		if (value->u.bool_value) {
			return "true";
		}
		return "false";
	case  SIDE_ATTR_TYPE_U8:
		snprintf(buffer, sizeof(buffer), "%" PRIu8,
			value->u.integer_value.side_u8);
		return buffer;
	case  SIDE_ATTR_TYPE_U16:
		snprintf(buffer, sizeof(buffer), "%" PRIu16,
			value->u.integer_value.side_u16);
		return buffer;
	case  SIDE_ATTR_TYPE_U32:
		snprintf(buffer, sizeof(buffer), "%" PRIu32,
			value->u.integer_value.side_u32);
		return buffer;
	case  SIDE_ATTR_TYPE_U64:
		snprintf(buffer, sizeof(buffer), "%" PRIu64,
			value->u.integer_value.side_u64);
		return buffer;
	case  SIDE_ATTR_TYPE_U128:
		snprintf(buffer, sizeof(buffer), "%" PRIu64 "%" PRIu64,
			value->u.integer_value.side_u128_split[SIDE_INTEGER128_SPLIT_HIGH],
			value->u.integer_value.side_u128_split[SIDE_INTEGER128_SPLIT_LOW]);
		return buffer;
	case  SIDE_ATTR_TYPE_S8:
		snprintf(buffer, sizeof(buffer), "%" PRId8,
			value->u.integer_value.side_s8);
		return buffer;
	case  SIDE_ATTR_TYPE_S16:
		snprintf(buffer, sizeof(buffer), "%" PRId16,
			value->u.integer_value.side_s16);
		return buffer;
	case  SIDE_ATTR_TYPE_S32:
		snprintf(buffer, sizeof(buffer), "%" PRId32,
			value->u.integer_value.side_s32);
		return buffer;
	case  SIDE_ATTR_TYPE_S64:
		snprintf(buffer, sizeof(buffer), "%" PRId64,
			value->u.integer_value.side_s64);
		return buffer;
	case  SIDE_ATTR_TYPE_S128:
		snprintf(buffer, sizeof(buffer), "%" PRId64 "%" PRId64,
			value->u.integer_value.side_s128_split[SIDE_INTEGER128_SPLIT_HIGH],
			value->u.integer_value.side_s128_split[SIDE_INTEGER128_SPLIT_LOW]);
		return buffer;
#if __HAVE_FLOAT16
	case SIDE_ATTR_TYPE_FLOAT_BINARY16:
		snprintf(buffer, sizeof(buffer), "%g",
			cast(double, value->u.float_value.side_float_binary16));
		return buffer;
#endif
#if __HAVE_FLOAT32
	case SIDE_ATTR_TYPE_FLOAT_BINARY32:
		snprintf(buffer, sizeof(buffer), "%g",
			cast(double, value->u.float_value.side_float_binary32));
		return buffer;
#endif
#if __HAVE_FLOAT64
	case SIDE_ATTR_TYPE_FLOAT_BINARY64:
		snprintf(buffer, sizeof(buffer), "%g",
			cast(double, value->u.float_value.side_float_binary64));
		return buffer;
#endif
#if __HAVE_FLOAT128
	case SIDE_ATTR_TYPE_FLOAT_BINARY128:
		snprintf(buffer, sizeof(buffer), "%Lg",
			cast(long double, value->u.float_value.side_float_binary128));
		return buffer;
#endif
	case SIDE_ATTR_TYPE_STRING:
		snprintf(buffer, sizeof(buffer), "\"%s\"",
			cast(const char *, visit_side_pointer(ctx, value->u.string_value.p)));
		return buffer;
	default:
		return "\"<UKNOWN>\"";
	}
}

static void print_attribute(const struct side_attr *attribute,
			void *ctx)
{
	const char *key;
	const char *value;

	key = visit_side_pointer(ctx, attribute->key.p);
	value = side_attr_value_to_string(&attribute->value, ctx);

	printf_nest(ctx, "\"%s\": %s", key, value);
}

static void print_attributes(const struct side_attr *attr,
			size_t nr_attr,
			void *ctx)
{
	if (nr_attr && attr) {
		printf_nest(ctx, "\"attributes\": {");
		push_nest(ctx); {
			for (size_t k = 0; k < nr_attr; ++k) {
				print_attribute(&attr[k], ctx);
			}
		} pop_nest(ctx);
	} else {
		printf_nest(ctx, "\"attributes\": {}");
	}
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
	printf_nest(ctx, "\"provider\": \"%s\"", cast(char *, visit_side_pointer(ctx, desc->provider_name)));
	printf_nest(ctx, "\"event\": \"%s\"", cast(char *, visit_side_pointer(ctx, desc->event_name)));
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

	printf_nest(ctx, "\"%s\": {", field_name);
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

static void begin_vla_visitor(const struct side_type_vla_visitor *vla_visitor, void *ctx)
{
	(void) vla_visitor;

	/*
	 * TODO: Resolve visitor pointer to string if possible.
	 *
	 * NOTE: JSON does not support Hexadecimal numbers.
	 */
	printf_nest(ctx, "\"visitor:\": %" PRIuPTR,
		cast(uptr, visit_side_pointer(ctx, vla_visitor->visitor)));
	printf_nest(ctx, "\"length\": {");
	push_nest(ctx);
}

static void after_vla_visitor_length(const struct side_type_vla_visitor *vla_visitor, void *ctx)
{
	(void) vla_visitor;

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
	printf_nest(ctx, "\"%s\": [%" PRId64 ", %" PRId64 "]",
		cast(char *, visit_side_pointer(ctx, map->label.p)),
		map->range_begin,
		map->range_end);
}

static void print_enum_bitmap_mapping(const struct side_enum_bitmap_mapping *map, void *ctx)
{
	if (!map) {
		return;
	}

	/* FIXME: Decode raw string. */
	printf_nest(ctx, "\"%s\": [%" PRId64 ", %" PRId64 "]",
		cast(char *, visit_side_pointer(ctx, map->label.p)),
		map->range_begin,
		map->range_end);
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
	struct json_context *jctx;

	jctx = ctx->context;

	ctx->nesting = 1;
	jctx->first_element = true;

	printf("[");

}

static void end_json(struct visitor_context *ctx)
{
	struct json_context *jctx;

	jctx = ctx->context;

	ctx->nesting = 0;
	jctx->first_element = false;

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

		.before_vla_visitor_type_func = begin_vla_visitor,
		.after_length_vla_visitor_type_func = after_vla_visitor_length,
		// .after_element_vla_visitor_type_func

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
