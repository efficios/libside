// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <side/trace.h>

static
void tracer_print_struct(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc);
static
void tracer_print_array(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc);
static
void tracer_print_vla(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc);
static
void tracer_print_vla_visitor(const struct side_type_description *type_desc, void *app_ctx);
static
void tracer_print_array_fixint(const struct side_type_description *type_desc, const struct side_arg_vec *item);
static
void tracer_print_vla_fixint(const struct side_type_description *type_desc, const struct side_arg_vec *item);
static
void tracer_print_dynamic(const struct side_arg_dynamic_vec *dynamic_item);

static
void tracer_print_attr_type(const struct side_attr *attr)
{
	printf("{ key: \"%s\", value: ", attr->key);
	switch (attr->value.type) {
	case SIDE_ATTR_TYPE_BOOL:
		printf("%s", attr->value.u.side_bool ? "true" : "false");
		break;
	case SIDE_ATTR_TYPE_U8:
		printf("%" PRIu8, attr->value.u.side_u8);
		break;
	case SIDE_ATTR_TYPE_U16:
		printf("%" PRIu16, attr->value.u.side_u16);
		break;
	case SIDE_ATTR_TYPE_U32:
		printf("%" PRIu32, attr->value.u.side_u32);
		break;
	case SIDE_ATTR_TYPE_U64:
		printf("%" PRIu64, attr->value.u.side_u64);
		break;
	case SIDE_ATTR_TYPE_S8:
		printf("%" PRId8, attr->value.u.side_s8);
		break;
	case SIDE_ATTR_TYPE_S16:
		printf("%" PRId16, attr->value.u.side_s16);
		break;
	case SIDE_ATTR_TYPE_S32:
		printf("%" PRId32, attr->value.u.side_s32);
		break;
	case SIDE_ATTR_TYPE_S64:
		printf("%" PRId64, attr->value.u.side_s64);
		break;
	case SIDE_ATTR_TYPE_FLOAT_BINARY16:
#if __HAVE_FLOAT16
		printf("%g", (double) attr->value.u.side_float_binary16);
		break;
#else
		printf("ERROR: Unsupported binary16 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_FLOAT_BINARY32:
#if __HAVE_FLOAT32
		printf("%g", (double) attr->value.u.side_float_binary32);
		break;
#else
		printf("ERROR: Unsupported binary32 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_FLOAT_BINARY64:
#if __HAVE_FLOAT64
		printf("%g", (double) attr->value.u.side_float_binary64);
		break;
#else
		printf("ERROR: Unsupported binary64 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_FLOAT_BINARY128:
#if __HAVE_FLOAT128
		printf("%Lg", (long double) attr->value.u.side_float_binary128);
		break;
#else
		printf("ERROR: Unsupported binary128 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_STRING:
		printf("\"%s\"", attr->value.u.string);
		break;
	default:
		printf("<UNKNOWN TYPE>");
		abort();
	}
	printf(" }");
}

static
void print_attributes(const char *prefix_str, const struct side_attr *attr, uint32_t nr_attr)
{
	int i;

	if (!nr_attr)
		return;
	printf("%s[ ", prefix_str);
	for (i = 0; i < nr_attr; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_attr_type(&attr[i]);
	}
	printf(" ]");
}

static
void print_enum(const struct side_enum_mappings *side_enum_mappings, int64_t value)
{
	int i, print_count = 0;

	print_attributes("attr: ", side_enum_mappings->attr, side_enum_mappings->nr_attr);
	printf("%s", side_enum_mappings->nr_attr ? ", " : "");
	printf("value: %" PRId64 ", labels: [ ", value);
	for (i = 0; i < side_enum_mappings->nr_mappings; i++) {
		const struct side_enum_mapping *mapping = &side_enum_mappings->mappings[i];

		if (mapping->range_end < mapping->range_begin) {
			printf("ERROR: Unexpected enum range: %" PRIu64 "-%" PRIu64 "\n",
				mapping->range_begin, mapping->range_end);
			abort();
		}
		if (value >= mapping->range_begin && value <= mapping->range_end) {
			printf("%s", print_count++ ? ", " : "");
			printf("\"%s\"", mapping->label);
		}
	}
	if (!print_count)
		printf("<NO LABEL>");
	printf(" ]");
}

static
void print_enum_bitmap(const struct side_enum_bitmap_mappings *side_enum_mappings, uint64_t value)
{
	int i, print_count = 0;

	print_attributes("attr: ", side_enum_mappings->attr, side_enum_mappings->nr_attr);
	printf("%s", side_enum_mappings->nr_attr ? ", " : "");
	printf("value: 0x%" PRIx64 ", labels: [ ", value);
	for (i = 0; i < side_enum_mappings->nr_mappings; i++) {
		const struct side_enum_bitmap_mapping *mapping = &side_enum_mappings->mappings[i];
		bool match = false;
		int64_t bit;

		if (mapping->range_begin < 0 || mapping->range_end > 63
				|| mapping->range_end < mapping->range_begin) {
			printf("ERROR: Unexpected enum bitmap range: %" PRIu64 "-%" PRIu64 "\n",
				mapping->range_begin, mapping->range_end);
			abort();
		}
		for (bit = mapping->range_begin; bit <= mapping->range_end; bit++) {
			if (value & (1ULL << bit)) {
				match = true;
				break;
			}
		}
		if (match) {
			printf("%s", print_count++ ? ", " : "");
			printf("\"%s\"", mapping->label);
		}
	}
	if (!print_count)
		printf("<NO LABEL>");
	printf(" ]");
}

static
void tracer_print_basic_type_header(const struct side_type_description *type_desc)
{
	print_attributes("attr: ", type_desc->u.side_basic.attr, type_desc->u.side_basic.nr_attr);
	printf("%s", type_desc->u.side_basic.nr_attr ? ", " : "");
	printf("value: ");
}

static
void tracer_print_type(const struct side_type_description *type_desc, const struct side_arg_vec *item)
{
	switch (item->type) {
	case SIDE_TYPE_ARRAY_U8:
	case SIDE_TYPE_ARRAY_U16:
	case SIDE_TYPE_ARRAY_U32:
	case SIDE_TYPE_ARRAY_U64:
	case SIDE_TYPE_ARRAY_S8:
	case SIDE_TYPE_ARRAY_S16:
	case SIDE_TYPE_ARRAY_S32:
	case SIDE_TYPE_ARRAY_S64:
	case SIDE_TYPE_ARRAY_BLOB:
		if (type_desc->type != SIDE_TYPE_ARRAY) {
			printf("ERROR: type mismatch between description and arguments\n");
			abort();
		}
		break;
	case SIDE_TYPE_VLA_U8:
	case SIDE_TYPE_VLA_U16:
	case SIDE_TYPE_VLA_U32:
	case SIDE_TYPE_VLA_U64:
	case SIDE_TYPE_VLA_S8:
	case SIDE_TYPE_VLA_S16:
	case SIDE_TYPE_VLA_S32:
	case SIDE_TYPE_VLA_S64:
	case SIDE_TYPE_VLA_BLOB:
		if (type_desc->type != SIDE_TYPE_VLA) {
			printf("ERROR: type mismatch between description and arguments\n");
			abort();
		}
		break;

	default:
		if (type_desc->type != item->type) {
			printf("ERROR: type mismatch between description and arguments\n");
			abort();
		}
		break;
	}
	printf("{ ");
	switch (item->type) {
	case SIDE_TYPE_BOOL:
		tracer_print_basic_type_header(type_desc);
		printf("%s", item->u.side_bool ? "true" : "false");
		break;
	case SIDE_TYPE_U8:
		tracer_print_basic_type_header(type_desc);
		printf("%" PRIu8, item->u.side_u8);
		break;
	case SIDE_TYPE_U16:
		tracer_print_basic_type_header(type_desc);
		printf("%" PRIu16, item->u.side_u16);
		break;
	case SIDE_TYPE_U32:
		tracer_print_basic_type_header(type_desc);
		printf("%" PRIu32, item->u.side_u32);
		break;
	case SIDE_TYPE_U64:
		tracer_print_basic_type_header(type_desc);
		printf("%" PRIu64, item->u.side_u64);
		break;
	case SIDE_TYPE_S8:
		tracer_print_basic_type_header(type_desc);
		printf("%" PRId8, item->u.side_s8);
		break;
	case SIDE_TYPE_S16:
		tracer_print_basic_type_header(type_desc);
		printf("%" PRId16, item->u.side_s16);
		break;
	case SIDE_TYPE_S32:
		tracer_print_basic_type_header(type_desc);
		printf("%" PRId32, item->u.side_s32);
		break;
	case SIDE_TYPE_S64:
		tracer_print_basic_type_header(type_desc);
		printf("%" PRId64, item->u.side_s64);
		break;
	case SIDE_TYPE_BLOB:
		tracer_print_basic_type_header(type_desc);
		printf("0x%" PRIx8, item->u.side_blob);
		break;

	case SIDE_TYPE_ENUM_U8:
		print_enum(type_desc->u.side_enum_mappings,
			(int64_t) item->u.side_u8);
		break;
	case SIDE_TYPE_ENUM_U16:
		print_enum(type_desc->u.side_enum_mappings,
			(int64_t) item->u.side_u16);
		break;
	case SIDE_TYPE_ENUM_U32:
		print_enum(type_desc->u.side_enum_mappings,
			(int64_t) item->u.side_u32);
		break;
	case SIDE_TYPE_ENUM_U64:
		print_enum(type_desc->u.side_enum_mappings,
			(int64_t) item->u.side_u64);
		break;
	case SIDE_TYPE_ENUM_S8:
		print_enum(type_desc->u.side_enum_mappings,
			(int64_t) item->u.side_s8);
		break;
	case SIDE_TYPE_ENUM_S16:
		print_enum(type_desc->u.side_enum_mappings,
			(int64_t) item->u.side_s16);
		break;
	case SIDE_TYPE_ENUM_S32:
		print_enum(type_desc->u.side_enum_mappings,
			(int64_t) item->u.side_s32);
		break;
	case SIDE_TYPE_ENUM_S64:
		print_enum(type_desc->u.side_enum_mappings,
			item->u.side_s64);
		break;

	case SIDE_TYPE_ENUM_BITMAP8:
		print_enum_bitmap(type_desc->u.side_enum_bitmap_mappings,
			(uint64_t) item->u.side_u8);
		break;
	case SIDE_TYPE_ENUM_BITMAP16:
		print_enum_bitmap(type_desc->u.side_enum_bitmap_mappings,
			(uint64_t) item->u.side_u16);
		break;
	case SIDE_TYPE_ENUM_BITMAP32:
		print_enum_bitmap(type_desc->u.side_enum_bitmap_mappings,
			(uint64_t) item->u.side_u32);
		break;
	case SIDE_TYPE_ENUM_BITMAP64:
		print_enum_bitmap(type_desc->u.side_enum_bitmap_mappings,
			item->u.side_u64);
		break;

	case SIDE_TYPE_FLOAT_BINARY16:
		tracer_print_basic_type_header(type_desc);
#if __HAVE_FLOAT16
		printf("%g", (double) item->u.side_float_binary16);
		break;
#else
		printf("ERROR: Unsupported binary16 float type\n");
		abort();
#endif
	case SIDE_TYPE_FLOAT_BINARY32:
		tracer_print_basic_type_header(type_desc);
#if __HAVE_FLOAT32
		printf("%g", (double) item->u.side_float_binary32);
		break;
#else
		printf("ERROR: Unsupported binary32 float type\n");
		abort();
#endif
	case SIDE_TYPE_FLOAT_BINARY64:
		tracer_print_basic_type_header(type_desc);
#if __HAVE_FLOAT64
		printf("%g", (double) item->u.side_float_binary64);
		break;
#else
		printf("ERROR: Unsupported binary64 float type\n");
		abort();
#endif
	case SIDE_TYPE_FLOAT_BINARY128:
		tracer_print_basic_type_header(type_desc);
#if __HAVE_FLOAT128
		printf("%Lg", (long double) item->u.side_float_binary128);
		break;
#else
		printf("ERROR: Unsupported binary128 float type\n");
		abort();
#endif
	case SIDE_TYPE_STRING:
		tracer_print_basic_type_header(type_desc);
		printf("\"%s\"", item->u.string);
		break;
	case SIDE_TYPE_STRUCT:
		tracer_print_struct(type_desc, item->u.side_struct);
		break;
	case SIDE_TYPE_ARRAY:
		tracer_print_array(type_desc, item->u.side_array);
		break;
	case SIDE_TYPE_VLA:
		tracer_print_vla(type_desc, item->u.side_vla);
		break;
	case SIDE_TYPE_VLA_VISITOR:
		tracer_print_vla_visitor(type_desc, item->u.side_vla_app_visitor_ctx);
		break;
	case SIDE_TYPE_ARRAY_U8:
	case SIDE_TYPE_ARRAY_U16:
	case SIDE_TYPE_ARRAY_U32:
	case SIDE_TYPE_ARRAY_U64:
	case SIDE_TYPE_ARRAY_S8:
	case SIDE_TYPE_ARRAY_S16:
	case SIDE_TYPE_ARRAY_S32:
	case SIDE_TYPE_ARRAY_S64:
	case SIDE_TYPE_ARRAY_BLOB:
		tracer_print_array_fixint(type_desc, item);
		break;
	case SIDE_TYPE_VLA_U8:
	case SIDE_TYPE_VLA_U16:
	case SIDE_TYPE_VLA_U32:
	case SIDE_TYPE_VLA_U64:
	case SIDE_TYPE_VLA_S8:
	case SIDE_TYPE_VLA_S16:
	case SIDE_TYPE_VLA_S32:
	case SIDE_TYPE_VLA_S64:
	case SIDE_TYPE_VLA_BLOB:
		tracer_print_vla_fixint(type_desc, item);
		break;
	case SIDE_TYPE_DYNAMIC:
		tracer_print_basic_type_header(type_desc);
		tracer_print_dynamic(&item->u.dynamic);
		break;
	default:
		printf("<UNKNOWN TYPE>");
		abort();
	}
	printf(" }");
}

static
void tracer_print_field(const struct side_event_field *item_desc, const struct side_arg_vec *item)
{
	printf("%s: ", item_desc->field_name);
	tracer_print_type(&item_desc->side_type, item);
}

static
void tracer_print_struct(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc)
{
	const struct side_arg_vec *sav = sav_desc->sav;
	uint32_t side_sav_len = sav_desc->len;
	int i;

	if (type_desc->u.side_struct->nr_fields != side_sav_len) {
		printf("ERROR: number of fields mismatch between description and arguments of structure\n");
		abort();
	}
	print_attributes("attr: ", type_desc->u.side_struct->attr, type_desc->u.side_struct->nr_attr);
	printf("%s", type_desc->u.side_struct->nr_attr ? ", " : "");
	printf("fields: { ");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_field(&type_desc->u.side_struct->fields[i], &sav[i]);
	}
	printf(" }");
}

static
void tracer_print_array(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc)
{
	const struct side_arg_vec *sav = sav_desc->sav;
	uint32_t side_sav_len = sav_desc->len;
	int i;

	if (type_desc->u.side_array.length != side_sav_len) {
		printf("ERROR: length mismatch between description and arguments of array\n");
		abort();
	}
	print_attributes("attr: ", type_desc->u.side_array.attr, type_desc->u.side_array.nr_attr);
	printf("%s", type_desc->u.side_array.nr_attr ? ", " : "");
	printf("elements: ");
	printf("[ ");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_type(type_desc->u.side_array.elem_type, &sav[i]);
	}
	printf(" ]");
}

static
void tracer_print_vla(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc)
{
	const struct side_arg_vec *sav = sav_desc->sav;
	uint32_t side_sav_len = sav_desc->len;
	int i;

	print_attributes("attr: ", type_desc->u.side_vla.attr, type_desc->u.side_vla.nr_attr);
	printf("%s", type_desc->u.side_vla.nr_attr ? ", " : "");
	printf("elements: ");
	printf("[ ");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_type(type_desc->u.side_vla.elem_type, &sav[i]);
	}
	printf(" ]");
}

struct tracer_visitor_priv {
	const struct side_type_description *elem_type;
	int i;
};

static
enum side_visitor_status tracer_write_elem_cb(const struct side_tracer_visitor_ctx *tracer_ctx,
			const struct side_arg_vec *elem)
{
	struct tracer_visitor_priv *tracer_priv = tracer_ctx->priv;

	printf("%s", tracer_priv->i++ ? ", " : "");
	tracer_print_type(tracer_priv->elem_type, elem);
	return SIDE_VISITOR_STATUS_OK;
}

static
void tracer_print_vla_visitor(const struct side_type_description *type_desc, void *app_ctx)
{
	enum side_visitor_status status;
	struct tracer_visitor_priv tracer_priv = {
		.elem_type = type_desc->u.side_vla_visitor.elem_type,
		.i = 0,
	};
	const struct side_tracer_visitor_ctx tracer_ctx = {
		.write_elem = tracer_write_elem_cb,
		.priv = &tracer_priv,
	};

	print_attributes("attr: ", type_desc->u.side_vla_visitor.attr, type_desc->u.side_vla_visitor.nr_attr);
	printf("%s", type_desc->u.side_vla_visitor.nr_attr ? ", " : "");
	printf("elements: ");
	printf("[ ");
	status = type_desc->u.side_vla_visitor.visitor(&tracer_ctx, app_ctx);
	switch (status) {
	case SIDE_VISITOR_STATUS_OK:
		break;
	case SIDE_VISITOR_STATUS_ERROR:
		printf("ERROR: Visitor error\n");
		abort();
	}
	printf(" ]");
}

void tracer_print_array_fixint(const struct side_type_description *type_desc, const struct side_arg_vec *item)
{
	const struct side_type_description *elem_type = type_desc->u.side_array.elem_type;
	uint32_t side_sav_len = type_desc->u.side_array.length;
	void *p = item->u.side_array_fixint;
	enum side_type side_type;
	int i;

	print_attributes("attr: ", type_desc->u.side_array.attr, type_desc->u.side_array.nr_attr);
	printf("%s", type_desc->u.side_array.nr_attr ? ", " : "");
	printf("elements: ");
	switch (item->type) {
	case SIDE_TYPE_ARRAY_U8:
		if (elem_type->type != SIDE_TYPE_U8)
			goto type_error;
		break;
	case SIDE_TYPE_ARRAY_U16:
		if (elem_type->type != SIDE_TYPE_U16)
			goto type_error;
		break;
	case SIDE_TYPE_ARRAY_U32:
		if (elem_type->type != SIDE_TYPE_U32)
			goto type_error;
		break;
	case SIDE_TYPE_ARRAY_U64:
		if (elem_type->type != SIDE_TYPE_U64)
			goto type_error;
		break;
	case SIDE_TYPE_ARRAY_S8:
		if (elem_type->type != SIDE_TYPE_S8)
			goto type_error;
		break;
	case SIDE_TYPE_ARRAY_S16:
		if (elem_type->type != SIDE_TYPE_S16)
			goto type_error;
		break;
	case SIDE_TYPE_ARRAY_S32:
		if (elem_type->type != SIDE_TYPE_S32)
			goto type_error;
		break;
	case SIDE_TYPE_ARRAY_S64:
		if (elem_type->type != SIDE_TYPE_S64)
			goto type_error;
		break;
	case SIDE_TYPE_ARRAY_BLOB:
		if (elem_type->type != SIDE_TYPE_BLOB)
			goto type_error;
		break;
	default:
		goto type_error;
	}
	side_type = elem_type->type;

	printf("[ ");
	for (i = 0; i < side_sav_len; i++) {
		struct side_arg_vec sav_elem = {
			.type = side_type,
		};

		switch (side_type) {
		case SIDE_TYPE_U8:
			sav_elem.u.side_u8 = ((const uint8_t *) p)[i];
			break;
		case SIDE_TYPE_S8:
			sav_elem.u.side_s8 = ((const int8_t *) p)[i];
			break;
		case SIDE_TYPE_U16:
			sav_elem.u.side_u16 = ((const uint16_t *) p)[i];
			break;
		case SIDE_TYPE_S16:
			sav_elem.u.side_s16 = ((const int16_t *) p)[i];
			break;
		case SIDE_TYPE_U32:
			sav_elem.u.side_u32 = ((const uint32_t *) p)[i];
			break;
		case SIDE_TYPE_S32:
			sav_elem.u.side_s32 = ((const int32_t *) p)[i];
			break;
		case SIDE_TYPE_U64:
			sav_elem.u.side_u64 = ((const uint64_t *) p)[i];
			break;
		case SIDE_TYPE_S64:
			sav_elem.u.side_s64 = ((const int64_t *) p)[i];
			break;
		case SIDE_TYPE_BLOB:
			sav_elem.u.side_blob = ((const uint8_t *) p)[i];
			break;

		default:
			printf("ERROR: Unexpected type\n");
			abort();
		}

		printf("%s", i ? ", " : "");
		tracer_print_type(elem_type, &sav_elem);
	}
	printf(" ]");
	return;

type_error:
	printf("ERROR: type mismatch\n");
	abort();
}

void tracer_print_vla_fixint(const struct side_type_description *type_desc, const struct side_arg_vec *item)
{
	const struct side_type_description *elem_type = type_desc->u.side_vla.elem_type;
	uint32_t side_sav_len = item->u.side_vla_fixint.length;
	void *p = item->u.side_vla_fixint.p;
	enum side_type side_type;
	int i;

	print_attributes("attr: ", type_desc->u.side_vla.attr, type_desc->u.side_vla.nr_attr);
	printf("%s", type_desc->u.side_vla.nr_attr ? ", " : "");
	printf("elements: ");
	switch (item->type) {
	case SIDE_TYPE_VLA_U8:
		if (elem_type->type != SIDE_TYPE_U8)
			goto type_error;
		break;
	case SIDE_TYPE_VLA_U16:
		if (elem_type->type != SIDE_TYPE_U16)
			goto type_error;
		break;
	case SIDE_TYPE_VLA_U32:
		if (elem_type->type != SIDE_TYPE_U32)
			goto type_error;
		break;
	case SIDE_TYPE_VLA_U64:
		if (elem_type->type != SIDE_TYPE_U64)
			goto type_error;
		break;
	case SIDE_TYPE_VLA_S8:
		if (elem_type->type != SIDE_TYPE_S8)
			goto type_error;
		break;
	case SIDE_TYPE_VLA_S16:
		if (elem_type->type != SIDE_TYPE_S16)
			goto type_error;
		break;
	case SIDE_TYPE_VLA_S32:
		if (elem_type->type != SIDE_TYPE_S32)
			goto type_error;
		break;
	case SIDE_TYPE_VLA_S64:
		if (elem_type->type != SIDE_TYPE_S64)
			goto type_error;
		break;
	case SIDE_TYPE_VLA_BLOB:
		if (elem_type->type != SIDE_TYPE_BLOB)
			goto type_error;
		break;
	default:
		goto type_error;
	}
	side_type = elem_type->type;

	printf("[ ");
	for (i = 0; i < side_sav_len; i++) {
		struct side_arg_vec sav_elem = {
			.type = side_type,
		};

		switch (side_type) {
		case SIDE_TYPE_U8:
			sav_elem.u.side_u8 = ((const uint8_t *) p)[i];
			break;
		case SIDE_TYPE_S8:
			sav_elem.u.side_s8 = ((const int8_t *) p)[i];
			break;
		case SIDE_TYPE_U16:
			sav_elem.u.side_u16 = ((const uint16_t *) p)[i];
			break;
		case SIDE_TYPE_S16:
			sav_elem.u.side_s16 = ((const int16_t *) p)[i];
			break;
		case SIDE_TYPE_U32:
			sav_elem.u.side_u32 = ((const uint32_t *) p)[i];
			break;
		case SIDE_TYPE_S32:
			sav_elem.u.side_s32 = ((const int32_t *) p)[i];
			break;
		case SIDE_TYPE_U64:
			sav_elem.u.side_u64 = ((const uint64_t *) p)[i];
			break;
		case SIDE_TYPE_S64:
			sav_elem.u.side_s64 = ((const int64_t *) p)[i];
			break;
		case SIDE_TYPE_BLOB:
			sav_elem.u.side_blob = ((const uint8_t *) p)[i];
			break;

		default:
			printf("ERROR: Unexpected type\n");
			abort();
		}

		printf("%s", i ? ", " : "");
		tracer_print_type(elem_type, &sav_elem);
	}
	printf(" ]");
	return;

type_error:
	printf("ERROR: type mismatch\n");
	abort();
}

static
void tracer_print_dynamic_struct(const struct side_arg_dynamic_event_struct *dynamic_struct)
{
	const struct side_arg_dynamic_event_field *fields = dynamic_struct->fields;
	uint32_t len = dynamic_struct->len;
	int i;

	print_attributes("attr:: ", dynamic_struct->attr, dynamic_struct->nr_attr);
	printf("%s", dynamic_struct->nr_attr ? ", " : "");
	printf("fields:: ");
	printf("[ ");
	for (i = 0; i < len; i++) {
		printf("%s", i ? ", " : "");
		printf("%s:: ", fields[i].field_name);
		tracer_print_dynamic(&fields[i].elem);
	}
	printf(" ]");
}

struct tracer_dynamic_struct_visitor_priv {
	int i;
};

static
enum side_visitor_status tracer_dynamic_struct_write_elem_cb(
			const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
			const struct side_arg_dynamic_event_field *dynamic_field)
{
	struct tracer_dynamic_struct_visitor_priv *tracer_priv = tracer_ctx->priv;

	printf("%s", tracer_priv->i++ ? ", " : "");
	printf("%s:: ", dynamic_field->field_name);
	tracer_print_dynamic(&dynamic_field->elem);
	return SIDE_VISITOR_STATUS_OK;
}

static
void tracer_print_dynamic_struct_visitor(const struct side_arg_dynamic_vec *item)
{
	enum side_visitor_status status;
	struct tracer_dynamic_struct_visitor_priv tracer_priv = {
		.i = 0,
	};
	const struct side_tracer_dynamic_struct_visitor_ctx tracer_ctx = {
		.write_field = tracer_dynamic_struct_write_elem_cb,
		.priv = &tracer_priv,
	};
	void *app_ctx = item->u.side_dynamic_struct_visitor.app_ctx;

	print_attributes("attr:: ", item->u.side_dynamic_struct_visitor.attr, item->u.side_dynamic_struct_visitor.nr_attr);
	printf("%s", item->u.side_dynamic_struct_visitor.nr_attr ? ", " : "");
	printf("fields:: ");
	printf("[ ");
	status = item->u.side_dynamic_struct_visitor.visitor(&tracer_ctx, app_ctx);
	switch (status) {
	case SIDE_VISITOR_STATUS_OK:
		break;
	case SIDE_VISITOR_STATUS_ERROR:
		printf("ERROR: Visitor error\n");
		abort();
	}
	printf(" ]");
}

static
void tracer_print_dynamic_vla(const struct side_arg_dynamic_vec_vla *vla)
{
	const struct side_arg_dynamic_vec *sav = vla->sav;
	uint32_t side_sav_len = vla->len;
	int i;

	print_attributes("attr:: ", vla->attr, vla->nr_attr);
	printf("%s", vla->nr_attr ? ", " : "");
	printf("elements:: ");
	printf("[ ");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_dynamic(&sav[i]);
	}
	printf(" ]");
}

struct tracer_dynamic_vla_visitor_priv {
	int i;
};

static
enum side_visitor_status tracer_dynamic_vla_write_elem_cb(
			const struct side_tracer_dynamic_vla_visitor_ctx *tracer_ctx,
			const struct side_arg_dynamic_vec *elem)
{
	struct tracer_dynamic_vla_visitor_priv *tracer_priv = tracer_ctx->priv;

	printf("%s", tracer_priv->i++ ? ", " : "");
	tracer_print_dynamic(elem);
	return SIDE_VISITOR_STATUS_OK;
}

static
void tracer_print_dynamic_vla_visitor(const struct side_arg_dynamic_vec *item)
{
	enum side_visitor_status status;
	struct tracer_dynamic_vla_visitor_priv tracer_priv = {
		.i = 0,
	};
	const struct side_tracer_dynamic_vla_visitor_ctx tracer_ctx = {
		.write_elem = tracer_dynamic_vla_write_elem_cb,
		.priv = &tracer_priv,
	};
	void *app_ctx = item->u.side_dynamic_vla_visitor.app_ctx;

	print_attributes("attr:: ", item->u.side_dynamic_vla_visitor.attr, item->u.side_dynamic_vla_visitor.nr_attr);
	printf("%s", item->u.side_dynamic_vla_visitor.nr_attr ? ", " : "");
	printf("elements:: ");
	printf("[ ");
	status = item->u.side_dynamic_vla_visitor.visitor(&tracer_ctx, app_ctx);
	switch (status) {
	case SIDE_VISITOR_STATUS_OK:
		break;
	case SIDE_VISITOR_STATUS_ERROR:
		printf("ERROR: Visitor error\n");
		abort();
	}
	printf(" ]");
}

static
void tracer_print_dynamic_basic_type_header(const struct side_arg_dynamic_vec *item)
{
	print_attributes("attr:: ", item->u.side_basic.attr, item->u.side_basic.nr_attr);
	printf("%s", item->u.side_basic.nr_attr ? ", " : "");
	printf("value:: ");
}

static
void tracer_print_dynamic(const struct side_arg_dynamic_vec *item)
{
	printf("{ ");
	switch (item->dynamic_type) {
	case SIDE_DYNAMIC_TYPE_NULL:
		tracer_print_dynamic_basic_type_header(item);
		printf("<NULL TYPE>");
		break;
	case SIDE_DYNAMIC_TYPE_BOOL:
		tracer_print_dynamic_basic_type_header(item);
		printf("%s", item->u.side_basic.u.side_bool ? "true" : "false");
		break;
	case SIDE_DYNAMIC_TYPE_U8:
		tracer_print_dynamic_basic_type_header(item);
		printf("%" PRIu8, item->u.side_basic.u.side_u8);
		break;
	case SIDE_DYNAMIC_TYPE_U16:
		tracer_print_dynamic_basic_type_header(item);
		printf("%" PRIu16, item->u.side_basic.u.side_u16);
		break;
	case SIDE_DYNAMIC_TYPE_U32:
		tracer_print_dynamic_basic_type_header(item);
		printf("%" PRIu32, item->u.side_basic.u.side_u32);
		break;
	case SIDE_DYNAMIC_TYPE_U64:
		tracer_print_dynamic_basic_type_header(item);
		printf("%" PRIu64, item->u.side_basic.u.side_u64);
		break;
	case SIDE_DYNAMIC_TYPE_S8:
		tracer_print_dynamic_basic_type_header(item);
		printf("%" PRId8, item->u.side_basic.u.side_s8);
		break;
	case SIDE_DYNAMIC_TYPE_S16:
		tracer_print_dynamic_basic_type_header(item);
		printf("%" PRId16, item->u.side_basic.u.side_s16);
		break;
	case SIDE_DYNAMIC_TYPE_S32:
		tracer_print_dynamic_basic_type_header(item);
		printf("%" PRId32, item->u.side_basic.u.side_s32);
		break;
	case SIDE_DYNAMIC_TYPE_S64:
		tracer_print_dynamic_basic_type_header(item);
		printf("%" PRId64, item->u.side_basic.u.side_s64);
		break;
	case SIDE_DYNAMIC_TYPE_BLOB:
		tracer_print_dynamic_basic_type_header(item);
		printf("0x%" PRIx8, item->u.side_basic.u.side_blob);
		break;

	case SIDE_DYNAMIC_TYPE_FLOAT_BINARY16:
		tracer_print_dynamic_basic_type_header(item);
#if __HAVE_FLOAT16
		printf("%g", (double) item->u.side_basic.u.side_float_binary16);
		break;
#else
		printf("ERROR: Unsupported binary16 float type\n");
		abort();
#endif
	case SIDE_DYNAMIC_TYPE_FLOAT_BINARY32:
		tracer_print_dynamic_basic_type_header(item);
#if __HAVE_FLOAT32
		printf("%g", (double) item->u.side_basic.u.side_float_binary32);
		break;
#else
		printf("ERROR: Unsupported binary32 float type\n");
		abort();
#endif
	case SIDE_DYNAMIC_TYPE_FLOAT_BINARY64:
		tracer_print_dynamic_basic_type_header(item);
#if __HAVE_FLOAT64
		printf("%g", (double) item->u.side_basic.u.side_float_binary64);
		break;
#else
		printf("ERROR: Unsupported binary64 float type\n");
		abort();
#endif
	case SIDE_DYNAMIC_TYPE_FLOAT_BINARY128:
		tracer_print_dynamic_basic_type_header(item);
#if __HAVE_FLOAT128
		printf("%Lg", (long double) item->u.side_basic.u.side_float_binary128);
		break;
#else
		printf("ERROR: Unsupported binary128 float type\n");
		abort();
#endif
	case SIDE_DYNAMIC_TYPE_STRING:
		tracer_print_dynamic_basic_type_header(item);
		printf("\"%s\"", item->u.side_basic.u.string);
		break;
	case SIDE_DYNAMIC_TYPE_STRUCT:
		tracer_print_dynamic_struct(item->u.side_dynamic_struct);
		break;
	case SIDE_DYNAMIC_TYPE_STRUCT_VISITOR:
		tracer_print_dynamic_struct_visitor(item);
		break;
	case SIDE_DYNAMIC_TYPE_VLA:
		tracer_print_dynamic_vla(item->u.side_dynamic_vla);
		break;
	case SIDE_DYNAMIC_TYPE_VLA_VISITOR:
		tracer_print_dynamic_vla_visitor(item);
		break;
	default:
		printf("<UNKNOWN TYPE>");
		abort();
	}
	printf(" }");
}

static
void tracer_print_static_fields(const struct side_event_description *desc,
		const struct side_arg_vec_description *sav_desc,
		int *nr_items)
{
	const struct side_arg_vec *sav = sav_desc->sav;
	uint32_t side_sav_len = sav_desc->len;
	int i;

	printf("provider: %s, event: %s", desc->provider_name, desc->event_name);
	if (desc->nr_fields != side_sav_len) {
		printf("ERROR: number of fields mismatch between description and arguments\n");
		abort();
	}
	print_attributes(", attributes: ", desc->attr, desc->nr_attr);
	printf("%s", side_sav_len ? ", fields: [ " : "");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_field(&desc->fields[i], &sav[i]);
	}
	if (nr_items)
		*nr_items = i;
}

void tracer_call(const struct side_event_description *desc,
		const struct side_arg_vec_description *sav_desc,
		void *priv __attribute__((unused)))
{
	int nr_fields = 0;

	tracer_print_static_fields(desc, sav_desc, &nr_fields);
	if (nr_fields)
		printf(" ]");
	printf("\n");
}

void tracer_call_variadic(const struct side_event_description *desc,
		const struct side_arg_vec_description *sav_desc,
		const struct side_arg_dynamic_event_struct *var_struct,
		void *priv __attribute__((unused)))
{
	uint32_t var_struct_len = var_struct->len;
	int nr_fields = 0, i;

	tracer_print_static_fields(desc, sav_desc, &nr_fields);

	if (side_unlikely(!(desc->flags & SIDE_EVENT_FLAG_VARIADIC))) {
		printf("ERROR: unexpected non-variadic event description\n");
		abort();
	}
	printf("%s", var_struct_len && !nr_fields ? ", fields: [ " : "");
	for (i = 0; i < var_struct_len; i++, nr_fields++) {
		printf("%s", nr_fields ? ", " : "");
		printf("%s:: ", var_struct->fields[i].field_name);
		tracer_print_dynamic(&var_struct->fields[i].elem);
	}
	if (i)
		printf(" ]");
	printf("\n");
}
