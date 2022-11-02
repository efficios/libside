// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <side/trace.h>

enum tracer_display_base {
	TRACER_DISPLAY_BASE_2,
	TRACER_DISPLAY_BASE_8,
	TRACER_DISPLAY_BASE_10,
	TRACER_DISPLAY_BASE_16,
};

static struct side_tracer_handle *tracer_handle;

static
void tracer_print_struct(const struct side_type_description *type_desc, const struct side_arg_vec_description *sav_desc);
static
void tracer_print_struct_sg(const struct side_type_description *type_desc, void *ptr);
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
void tracer_print_type(const struct side_type_description *type_desc, const struct side_arg_vec *item);

static
int64_t get_attr_integer_value(const struct side_attr *attr)
{
	int64_t val;

	switch (attr->value.type) {
	case SIDE_ATTR_TYPE_U8:
		val = attr->value.u.side_u8;
		break;
	case SIDE_ATTR_TYPE_U16:
		val = attr->value.u.side_u16;
		break;
	case SIDE_ATTR_TYPE_U32:
		val = attr->value.u.side_u32;
		break;
	case SIDE_ATTR_TYPE_U64:
		val = attr->value.u.side_u64;
		break;
	case SIDE_ATTR_TYPE_S8:
		val = attr->value.u.side_s8;
		break;
	case SIDE_ATTR_TYPE_S16:
		val = attr->value.u.side_s16;
		break;
	case SIDE_ATTR_TYPE_S32:
		val = attr->value.u.side_s32;
		break;
	case SIDE_ATTR_TYPE_S64:
		val = attr->value.u.side_s64;
		break;
	default:
		fprintf(stderr, "Unexpected attribute type\n");
		abort();
	}
	return val;
}

static
enum tracer_display_base get_attr_display_base(const struct side_attr *_attr, uint32_t nr_attr)
{
	uint32_t i;

	for (i = 0; i < nr_attr; i++) {
		const struct side_attr *attr = &_attr[i];

		if (!strcmp(attr->key, "std.integer.base")) {
			int64_t val = get_attr_integer_value(attr);

			switch (val) {
			case 2:
				return TRACER_DISPLAY_BASE_2;
			case 8:
				return TRACER_DISPLAY_BASE_8;
			case 10:
				return TRACER_DISPLAY_BASE_10;
			case 16:
				return TRACER_DISPLAY_BASE_16;
			default:
				fprintf(stderr, "Unexpected integer display base: %" PRId64 "\n", val);
				abort();
			}
		}
	}
	return TRACER_DISPLAY_BASE_10;	/* Default */
}

static
bool type_to_host_reverse_bo(const struct side_type_description *type_desc)
{
	switch (type_desc->type) {
	case SIDE_TYPE_U8:
	case SIDE_TYPE_S8:
	case SIDE_TYPE_BYTE:
		return false;
        case SIDE_TYPE_U16:
        case SIDE_TYPE_U32:
        case SIDE_TYPE_U64:
        case SIDE_TYPE_S16:
        case SIDE_TYPE_S32:
        case SIDE_TYPE_S64:
        case SIDE_TYPE_POINTER32:
        case SIDE_TYPE_POINTER64:
		if (type_desc->u.side_basic.byte_order != SIDE_TYPE_BYTE_ORDER_HOST)
			return true;
		else
			return false;
		break;
        case SIDE_TYPE_FLOAT_BINARY16:
        case SIDE_TYPE_FLOAT_BINARY32:
        case SIDE_TYPE_FLOAT_BINARY64:
        case SIDE_TYPE_FLOAT_BINARY128:
		if (type_desc->u.side_basic.byte_order != SIDE_TYPE_FLOAT_WORD_ORDER_HOST)
			return true;
		else
			return false;
		break;
	default:
		fprintf(stderr, "Unexpected type\n");
		abort();
	}
}

static
bool sg_type_to_host_reverse_bo(const struct side_type_sg_description *sg_type)
{
	switch (sg_type->type) {
	case SIDE_TYPE_SG_UNSIGNED_INT:
	case SIDE_TYPE_SG_SIGNED_INT:
		if (sg_type->u.side_basic.byte_order != SIDE_TYPE_BYTE_ORDER_HOST)
			return true;
		else
			return false;
		break;
	default:
		fprintf(stderr, "Unexpected type\n");
		abort();
	}
}

static
bool dynamic_type_to_host_reverse_bo(const struct side_arg_dynamic_vec *item)
{
	switch (item->dynamic_type) {
	case SIDE_DYNAMIC_TYPE_U8:
	case SIDE_DYNAMIC_TYPE_S8:
	case SIDE_DYNAMIC_TYPE_BYTE:
		return false;
        case SIDE_DYNAMIC_TYPE_U16:
        case SIDE_DYNAMIC_TYPE_U32:
        case SIDE_DYNAMIC_TYPE_U64:
        case SIDE_DYNAMIC_TYPE_S16:
        case SIDE_DYNAMIC_TYPE_S32:
        case SIDE_DYNAMIC_TYPE_S64:
        case SIDE_DYNAMIC_TYPE_POINTER32:
        case SIDE_DYNAMIC_TYPE_POINTER64:
		if (item->u.side_basic.byte_order != SIDE_TYPE_BYTE_ORDER_HOST)
			return true;
		else
			return false;
		break;
        case SIDE_DYNAMIC_TYPE_FLOAT_BINARY16:
        case SIDE_DYNAMIC_TYPE_FLOAT_BINARY32:
        case SIDE_DYNAMIC_TYPE_FLOAT_BINARY64:
        case SIDE_DYNAMIC_TYPE_FLOAT_BINARY128:
		if (item->u.side_basic.byte_order != SIDE_TYPE_FLOAT_WORD_ORDER_HOST)
			return true;
		else
			return false;
		break;
	default:
		fprintf(stderr, "Unexpected type\n");
		abort();
	}
}

static
void tracer_print_attr_type(const char *separator, const struct side_attr *attr)
{
	printf("{ key%s \"%s\", value%s ", separator, attr->key, separator);
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
	case SIDE_ATTR_TYPE_POINTER32:
		printf("0x%" PRIx32, attr->value.u.side_u32);
		break;
	case SIDE_ATTR_TYPE_POINTER64:
		printf("0x%" PRIx64, attr->value.u.side_u64);
		break;
	case SIDE_ATTR_TYPE_FLOAT_BINARY16:
#if __HAVE_FLOAT16
		printf("%g", (double) attr->value.u.side_float_binary16);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary16 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_FLOAT_BINARY32:
#if __HAVE_FLOAT32
		printf("%g", (double) attr->value.u.side_float_binary32);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary32 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_FLOAT_BINARY64:
#if __HAVE_FLOAT64
		printf("%g", (double) attr->value.u.side_float_binary64);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary64 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_FLOAT_BINARY128:
#if __HAVE_FLOAT128
		printf("%Lg", (long double) attr->value.u.side_float_binary128);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary128 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_STRING:
		printf("\"%s\"", (const char *)(uintptr_t) attr->value.u.string);
		break;
	default:
		fprintf(stderr, "ERROR: <UNKNOWN ATTRIBUTE TYPE>");
		abort();
	}
	printf(" }");
}

static
void print_attributes(const char *prefix_str, const char *separator,
		const struct side_attr *attr, uint32_t nr_attr)
{
	int i;

	if (!nr_attr)
		return;
	printf("%s%s [ ", prefix_str, separator);
	for (i = 0; i < nr_attr; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_attr_type(separator, &attr[i]);
	}
	printf(" ]");
}

static
void print_enum(const struct side_type_description *type_desc, const struct side_arg_vec *item)
{
	const struct side_enum_mappings *mappings = type_desc->u.side_enum.mappings;
	const struct side_type_description *elem_type = type_desc->u.side_enum.elem_type;
	int i, print_count = 0;
	int64_t value;

	if (elem_type->type != item->type) {
		fprintf(stderr, "ERROR: Unexpected enum element type\n");
		abort();
	}
	switch (item->type) {
	case SIDE_TYPE_U8:
		value = (int64_t) item->u.side_u8;
		break;
	case SIDE_TYPE_U16:
	{
		uint16_t v;

		v = item->u.side_u16;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_16(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_U32:
	{
		uint32_t v;

		v = item->u.side_u32;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_32(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_U64:
	{
		uint64_t v;

		v = item->u.side_u64;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_64(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_S8:
		value = (int64_t) item->u.side_s8;
		break;
	case SIDE_TYPE_S16:
	{
		int16_t v;

		v = item->u.side_s16;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_16(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_S32:
	{
		int32_t v;

		v = item->u.side_s32;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_32(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_S64:
	{
		int64_t v;

		v = item->u.side_s64;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_64(v);
		value = v;
		break;
	}
	default:
		fprintf(stderr, "ERROR: Unexpected enum element type\n");
		abort();
	}
	print_attributes("attr", ":", mappings->attr, mappings->nr_attr);
	printf("%s", mappings->nr_attr ? ", " : "");
	tracer_print_type(type_desc->u.side_enum.elem_type, item);
	printf(", labels: [ ");
	for (i = 0; i < mappings->nr_mappings; i++) {
		const struct side_enum_mapping *mapping = &mappings->mappings[i];

		if (mapping->range_end < mapping->range_begin) {
			fprintf(stderr, "ERROR: Unexpected enum range: %" PRIu64 "-%" PRIu64 "\n",
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
uint32_t enum_elem_type_to_stride(const struct side_type_description *elem_type)
{
	uint32_t stride_bit;

	switch (elem_type->type) {
	case SIDE_TYPE_U8:	/* Fall-through */
	case SIDE_TYPE_BYTE:
		stride_bit = 8;
		break;
	case SIDE_TYPE_U16:
		stride_bit = 16;
		break;
	case SIDE_TYPE_U32:
		stride_bit = 32;
		break;
	case SIDE_TYPE_U64:
		stride_bit = 64;
		break;
	default:
		fprintf(stderr, "ERROR: Unexpected enum element type\n");
		abort();
	}
	return stride_bit;
}

static
void print_enum_bitmap(const struct side_type_description *type_desc,
		const struct side_arg_vec *item)
{
	const struct side_type_description *elem_type = type_desc->u.side_enum_bitmap.elem_type;
	const struct side_enum_bitmap_mappings *side_enum_mappings = type_desc->u.side_enum_bitmap.mappings;
	int i, print_count = 0;
	uint32_t stride_bit, nr_items;
	bool reverse_byte_order = false;
	const struct side_arg_vec *array_item;

	switch (elem_type->type) {
	case SIDE_TYPE_U8:		/* Fall-through */
	case SIDE_TYPE_BYTE:		/* Fall-through */
	case SIDE_TYPE_U16:		/* Fall-through */
	case SIDE_TYPE_U32:		/* Fall-through */
	case SIDE_TYPE_U64:
		stride_bit = enum_elem_type_to_stride(elem_type);
		reverse_byte_order = type_to_host_reverse_bo(elem_type);
		array_item = item;
		nr_items = 1;
		break;
	case SIDE_TYPE_ARRAY:
		stride_bit = enum_elem_type_to_stride(elem_type->u.side_array.elem_type);
		reverse_byte_order = type_to_host_reverse_bo(elem_type->u.side_array.elem_type);
		array_item = item->u.side_array->sav;
		nr_items = type_desc->u.side_array.length;
		break;
	case SIDE_TYPE_VLA:
		stride_bit = enum_elem_type_to_stride(elem_type->u.side_vla.elem_type);
		reverse_byte_order = type_to_host_reverse_bo(elem_type->u.side_vla.elem_type);
		array_item = item->u.side_vla->sav;
		nr_items = item->u.side_vla->len;
		break;
	default:
		fprintf(stderr, "ERROR: Unexpected enum element type\n");
		abort();
	}

	print_attributes("attr", ":", side_enum_mappings->attr, side_enum_mappings->nr_attr);
	printf("%s", side_enum_mappings->nr_attr ? ", " : "");
	printf("labels: [ ");
	for (i = 0; i < side_enum_mappings->nr_mappings; i++) {
		const struct side_enum_bitmap_mapping *mapping = &side_enum_mappings->mappings[i];
		bool match = false;
		uint64_t bit;

		if (mapping->range_end < mapping->range_begin) {
			fprintf(stderr, "ERROR: Unexpected enum bitmap range: %" PRIu64 "-%" PRIu64 "\n",
				mapping->range_begin, mapping->range_end);
			abort();
		}
		for (bit = mapping->range_begin; bit <= mapping->range_end; bit++) {
			if (bit > (nr_items * stride_bit) - 1)
				break;
			switch (stride_bit) {
			case 8:
			{
				uint8_t v = array_item[bit / 8].u.side_u8;
				if (v & (1ULL << (bit % 8))) {
					match = true;
					goto match;
				}
				break;
			}
			case 16:
			{
				uint16_t v = array_item[bit / 16].u.side_u16;
				if (reverse_byte_order)
					v = side_bswap_16(v);
				if (v & (1ULL << (bit % 16))) {
					match = true;
					goto match;
				}
				break;
			}
			case 32:
			{
				uint32_t v = array_item[bit / 32].u.side_u32;
				if (reverse_byte_order)
					v = side_bswap_32(v);
				if (v & (1ULL << (bit % 32))) {
					match = true;
					goto match;
				}
				break;
			}
			case 64:
			{
				uint64_t v = array_item[bit / 64].u.side_u64;
				if (reverse_byte_order)
					v = side_bswap_64(v);
				if (v & (1ULL << (bit % 64))) {
					match = true;
					goto match;
				}
				break;
			}
			default:
				abort();
			}
		}
match:
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
void print_integer_binary(uint64_t v, int bits)
{
	int i;

	printf("0b");
	v <<= 64 - bits;
	for (i = 0; i < bits; i++) {
		printf("%c", v & (1ULL << 63) ? '1' : '0');
		v <<= 1;
	}
}

static
void tracer_print_basic_type_header(const struct side_type_description *type_desc)
{
	print_attributes("attr", ":", type_desc->u.side_basic.attr, type_desc->u.side_basic.nr_attr);
	printf("%s", type_desc->u.side_basic.nr_attr ? ", " : "");
	printf("value: ");
}

static
void tracer_print_type(const struct side_type_description *type_desc, const struct side_arg_vec *item)
{
	enum side_type type;
	enum tracer_display_base base = TRACER_DISPLAY_BASE_10;

	switch (type_desc->type) {
	case SIDE_TYPE_ARRAY:
		switch (item->type) {
		case SIDE_TYPE_ARRAY_U8:
		case SIDE_TYPE_ARRAY_U16:
		case SIDE_TYPE_ARRAY_U32:
		case SIDE_TYPE_ARRAY_U64:
		case SIDE_TYPE_ARRAY_S8:
		case SIDE_TYPE_ARRAY_S16:
		case SIDE_TYPE_ARRAY_S32:
		case SIDE_TYPE_ARRAY_S64:
		case SIDE_TYPE_ARRAY_POINTER32:
		case SIDE_TYPE_ARRAY_POINTER64:
		case SIDE_TYPE_ARRAY_BYTE:
		case SIDE_TYPE_ARRAY:
			break;
		default:
			fprintf(stderr, "ERROR: type mismatch between description and arguments\n");
			abort();
			break;
		}
		break;

	case SIDE_TYPE_VLA:
		switch (item->type) {
		case SIDE_TYPE_VLA_U8:
		case SIDE_TYPE_VLA_U16:
		case SIDE_TYPE_VLA_U32:
		case SIDE_TYPE_VLA_U64:
		case SIDE_TYPE_VLA_S8:
		case SIDE_TYPE_VLA_S16:
		case SIDE_TYPE_VLA_S32:
		case SIDE_TYPE_VLA_S64:
		case SIDE_TYPE_VLA_BYTE:
		case SIDE_TYPE_VLA_POINTER32:
		case SIDE_TYPE_VLA_POINTER64:
		case SIDE_TYPE_VLA:
			break;
		default:
			fprintf(stderr, "ERROR: type mismatch between description and arguments\n");
			abort();
			break;
		}
		break;

	case SIDE_TYPE_ENUM:
		switch (item->type) {
		case SIDE_TYPE_U8:
		case SIDE_TYPE_U16:
		case SIDE_TYPE_U32:
		case SIDE_TYPE_U64:
		case SIDE_TYPE_S8:
		case SIDE_TYPE_S16:
		case SIDE_TYPE_S32:
		case SIDE_TYPE_S64:
			break;
		default:
			fprintf(stderr, "ERROR: type mismatch between description and arguments\n");
			abort();
			break;
		}
		break;

	case SIDE_TYPE_ENUM_BITMAP:
		switch (item->type) {
		case SIDE_TYPE_U8:
		case SIDE_TYPE_BYTE:
		case SIDE_TYPE_U16:
		case SIDE_TYPE_U32:
		case SIDE_TYPE_U64:
		case SIDE_TYPE_ARRAY:
		case SIDE_TYPE_VLA:
			break;
		default:
			fprintf(stderr, "ERROR: type mismatch between description and arguments\n");
			abort();
			break;
		}
		break;

	default:
		if (type_desc->type != item->type) {
			fprintf(stderr, "ERROR: type mismatch between description and arguments\n");
			abort();
		}
		break;
	}

	if (type_desc->type == SIDE_TYPE_ENUM || type_desc->type == SIDE_TYPE_ENUM_BITMAP)
		type = type_desc->type;
	else
		type = item->type;

	switch (type) {
	case SIDE_TYPE_U8:
	case SIDE_TYPE_U16:
	case SIDE_TYPE_U32:
	case SIDE_TYPE_U64:
	case SIDE_TYPE_S8:
	case SIDE_TYPE_S16:
	case SIDE_TYPE_S32:
	case SIDE_TYPE_S64:
		base = get_attr_display_base(type_desc->u.side_basic.attr,
				type_desc->u.side_basic.nr_attr);
		break;
	default:
		break;
	}

	printf("{ ");
	switch (type) {
	case SIDE_TYPE_BOOL:
		tracer_print_basic_type_header(type_desc);
		printf("%s", item->u.side_bool ? "true" : "false");
		break;
	case SIDE_TYPE_U8:
	{
		uint8_t v;

		v = item->u.side_u8;
		tracer_print_basic_type_header(type_desc);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 8);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo8, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRIu8, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx8, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_TYPE_U16:
	{
		uint16_t v;

		v = item->u.side_u16;
		if (type_to_host_reverse_bo(type_desc))
			v = side_bswap_16(v);
		tracer_print_basic_type_header(type_desc);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 16);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo16, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRIu16, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx16, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_TYPE_U32:
	{
		uint32_t v;

		v = item->u.side_u32;
		if (type_to_host_reverse_bo(type_desc))
			v = side_bswap_32(v);
		tracer_print_basic_type_header(type_desc);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 32);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo32, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRIu32, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx32, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_TYPE_U64:
	{
		uint64_t v;

		v = item->u.side_u64;
		if (type_to_host_reverse_bo(type_desc))
			v = side_bswap_64(v);
		tracer_print_basic_type_header(type_desc);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 64);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo64, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRIu64, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx64, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_TYPE_S8:
	{
		int8_t v;

		v = item->u.side_s8;
		tracer_print_basic_type_header(type_desc);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 8);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo8, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRId8, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx8, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_TYPE_S16:
	{
		int16_t v;

		v = item->u.side_s16;
		if (type_to_host_reverse_bo(type_desc))
			v = side_bswap_16(v);
		tracer_print_basic_type_header(type_desc);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 16);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo16, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRId16, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx16, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_TYPE_S32:
	{
		int32_t v;

		v = item->u.side_s32;
		if (type_to_host_reverse_bo(type_desc))
			v = side_bswap_32(v);
		tracer_print_basic_type_header(type_desc);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 32);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo32, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRId32, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx32, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_TYPE_S64:
	{
		int64_t v;

		v = item->u.side_s64;
		if (type_to_host_reverse_bo(type_desc))
			v = side_bswap_64(v);
		tracer_print_basic_type_header(type_desc);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 64);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo64, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRId64, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx64, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_TYPE_POINTER32:
	{
		uint32_t v;

		v = item->u.side_u32;
		if (type_to_host_reverse_bo(type_desc))
			v = side_bswap_32(v);
		tracer_print_basic_type_header(type_desc);
		printf("0x%" PRIx32, v);
		break;
	}
	case SIDE_TYPE_POINTER64:
	{
		uint64_t v;

		v = item->u.side_u64;
		if (type_to_host_reverse_bo(type_desc))
			v = side_bswap_64(v);
		tracer_print_basic_type_header(type_desc);
		printf("0x%" PRIx64, v);
		break;
	}
	case SIDE_TYPE_BYTE:
		tracer_print_basic_type_header(type_desc);
		printf("0x%" PRIx8, item->u.side_byte);
		break;

	case SIDE_TYPE_ENUM:
		print_enum(type_desc, item);
		break;

	case SIDE_TYPE_ENUM_BITMAP:
		print_enum_bitmap(type_desc, item);
		break;

	case SIDE_TYPE_FLOAT_BINARY16:
	{
#if __HAVE_FLOAT16
		union {
			_Float16 f;
			uint16_t u;
		} float16 = {
			.f = item->u.side_float_binary16,
		};

		if (type_to_host_reverse_bo(type_desc))
			float16.u = side_bswap_16(float16.u);
		tracer_print_basic_type_header(type_desc);
		printf("%g", (double) float16.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary16 float type\n");
		abort();
#endif
	}
	case SIDE_TYPE_FLOAT_BINARY32:
	{
#if __HAVE_FLOAT32
		union {
			_Float32 f;
			uint32_t u;
		} float32 = {
			.f = item->u.side_float_binary32,
		};

		if (type_to_host_reverse_bo(type_desc))
			float32.u = side_bswap_32(float32.u);
		tracer_print_basic_type_header(type_desc);
		printf("%g", (double) float32.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary32 float type\n");
		abort();
#endif
	}
	case SIDE_TYPE_FLOAT_BINARY64:
	{
#if __HAVE_FLOAT64
		union {
			_Float64 f;
			uint64_t u;
		} float64 = {
			.f = item->u.side_float_binary64,
		};

		if (type_to_host_reverse_bo(type_desc))
			float64.u = side_bswap_64(float64.u);
		tracer_print_basic_type_header(type_desc);
		printf("%g", (double) float64.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary64 float type\n");
		abort();
#endif
	}
	case SIDE_TYPE_FLOAT_BINARY128:
	{
#if __HAVE_FLOAT128
		union {
			_Float128 f;
			char arr[16];
		} float128 = {
			.f = item->u.side_float_binary128,
		};

		if (type_to_host_reverse_bo(type_desc))
			side_bswap_128p(float128.arr);
		tracer_print_basic_type_header(type_desc);
		printf("%Lg", (long double) float128.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary128 float type\n");
		abort();
#endif
	}
	case SIDE_TYPE_STRING:
		tracer_print_basic_type_header(type_desc);
		printf("\"%s\"", (const char *)(uintptr_t) item->u.string);
		break;
	case SIDE_TYPE_STRUCT:
		tracer_print_struct(type_desc, item->u.side_struct);
		break;
	case SIDE_TYPE_STRUCT_SG:
		tracer_print_struct_sg(type_desc, item->u.side_struct_sg_ptr);
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
	case SIDE_TYPE_ARRAY_BYTE:
	case SIDE_TYPE_ARRAY_POINTER32:
	case SIDE_TYPE_ARRAY_POINTER64:
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
	case SIDE_TYPE_VLA_BYTE:
	case SIDE_TYPE_VLA_POINTER32:
	case SIDE_TYPE_VLA_POINTER64:
		tracer_print_vla_fixint(type_desc, item);
		break;
	case SIDE_TYPE_DYNAMIC:
		tracer_print_basic_type_header(type_desc);
		tracer_print_dynamic(&item->u.dynamic);
		break;
	default:
		fprintf(stderr, "<UNKNOWN TYPE>");
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
		fprintf(stderr, "ERROR: number of fields mismatch between description and arguments of structure\n");
		abort();
	}
	print_attributes("attr", ":", type_desc->u.side_struct->attr, type_desc->u.side_struct->nr_attr);
	printf("%s", type_desc->u.side_struct->nr_attr ? ", " : "");
	printf("fields: { ");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_field(&type_desc->u.side_struct->fields[i], &sav[i]);
	}
	printf(" }");
}

static
void tracer_print_sg_integer_type_header(const struct side_type_sg_description *sg_type)
{
	print_attributes("attr", ":", sg_type->u.side_basic.attr, sg_type->u.side_basic.nr_attr);
	printf("%s", sg_type->u.side_basic.nr_attr ? ", " : "");
	printf("value: ");
}

static
void tracer_print_sg_type(const struct side_type_sg_description *sg_type, void *_ptr)
{
	enum tracer_display_base base = TRACER_DISPLAY_BASE_10;
	const char *ptr = (const char *) _ptr;

	switch (sg_type->type) {
	case SIDE_TYPE_SG_UNSIGNED_INT:
	case SIDE_TYPE_SG_SIGNED_INT:
		base = get_attr_display_base(sg_type->u.side_basic.attr,
				sg_type->u.side_basic.nr_attr);
		break;
	default:
		break;
	}

	printf("{ ");
	switch (sg_type->type) {
	case SIDE_TYPE_SG_UNSIGNED_INT:
	{
		tracer_print_sg_integer_type_header(sg_type);
		switch (sg_type->u.side_basic.integer_size_bits) {
		case 8:
		{
			uint8_t v;

			if (!sg_type->u.side_basic.len_bits || sg_type->u.side_basic.len_bits + sg_type->u.side_basic.offset_bits > 8)
				abort();
			memcpy(&v, ptr + sg_type->u.side_basic.integer_offset, sizeof(v));
			v >>= sg_type->u.side_basic.offset_bits;
			if (sg_type->u.side_basic.len_bits < 8)
				v &= (1U << sg_type->u.side_basic.len_bits) - 1;
			switch (base) {
			case TRACER_DISPLAY_BASE_2:
				print_integer_binary(v, sg_type->u.side_basic.len_bits);
				break;
			case TRACER_DISPLAY_BASE_8:
				printf("0%" PRIo8, v);
				break;
			case TRACER_DISPLAY_BASE_10:
				printf("%" PRIu8, v);
				break;
			case TRACER_DISPLAY_BASE_16:
				printf("0x%" PRIx8, v);
				break;
			default:
				abort();
			}
			break;
		}
		case 16:
		{
			uint16_t v;

			if (!sg_type->u.side_basic.len_bits || sg_type->u.side_basic.len_bits + sg_type->u.side_basic.offset_bits > 16)
				abort();
			memcpy(&v, ptr + sg_type->u.side_basic.integer_offset, sizeof(v));
			if (sg_type_to_host_reverse_bo(sg_type))
				v = side_bswap_16(v);
			v >>= sg_type->u.side_basic.offset_bits;
			if (sg_type->u.side_basic.len_bits < 16)
				v &= (1U << sg_type->u.side_basic.len_bits) - 1;
			switch (base) {
			case TRACER_DISPLAY_BASE_2:
				print_integer_binary(v, sg_type->u.side_basic.len_bits);
				break;
			case TRACER_DISPLAY_BASE_8:
				printf("0%" PRIo16, v);
				break;
			case TRACER_DISPLAY_BASE_10:
				printf("%" PRIu16, v);
				break;
			case TRACER_DISPLAY_BASE_16:
				printf("0x%" PRIx16, v);
				break;
			default:
				abort();
			}
			break;
		}
		case 32:
		{
			uint32_t v;

			if (!sg_type->u.side_basic.len_bits || sg_type->u.side_basic.len_bits + sg_type->u.side_basic.offset_bits > 32)
				abort();
			memcpy(&v, ptr + sg_type->u.side_basic.integer_offset, sizeof(v));
			if (sg_type_to_host_reverse_bo(sg_type))
				v = side_bswap_32(v);
			v >>= sg_type->u.side_basic.offset_bits;
			if (sg_type->u.side_basic.len_bits < 32)
				v &= (1U << sg_type->u.side_basic.len_bits) - 1;
			switch (base) {
			case TRACER_DISPLAY_BASE_2:
				print_integer_binary(v, sg_type->u.side_basic.len_bits);
				break;
			case TRACER_DISPLAY_BASE_8:
				printf("0%" PRIo32, v);
				break;
			case TRACER_DISPLAY_BASE_10:
				printf("%" PRIu32, v);
				break;
			case TRACER_DISPLAY_BASE_16:
				printf("0x%" PRIx32, v);
				break;
			default:
				abort();
			}
			break;
		}
		case 64:
		{
			uint64_t v;

			if (!sg_type->u.side_basic.len_bits || sg_type->u.side_basic.len_bits + sg_type->u.side_basic.offset_bits > 64)
				abort();
			memcpy(&v, ptr + sg_type->u.side_basic.integer_offset, sizeof(v));
			if (sg_type_to_host_reverse_bo(sg_type))
				v = side_bswap_64(v);
			v >>= sg_type->u.side_basic.offset_bits;
			if (sg_type->u.side_basic.len_bits < 64)
				v &= (1ULL << sg_type->u.side_basic.len_bits) - 1;
			switch (base) {
			case TRACER_DISPLAY_BASE_2:
				print_integer_binary(v, sg_type->u.side_basic.len_bits);
				break;
			case TRACER_DISPLAY_BASE_8:
				printf("0%" PRIo64, v);
				break;
			case TRACER_DISPLAY_BASE_10:
				printf("%" PRIu64, v);
				break;
			case TRACER_DISPLAY_BASE_16:
				printf("0x%" PRIx64, v);
				break;
			default:
				abort();
			}
			break;
		}
		default:
			fprintf(stderr, "<UNKNOWN SCATTER-GATHER INTEGER SIZE>");
			abort();
		}
		break;
	}
	case SIDE_TYPE_SG_SIGNED_INT:
	{
		tracer_print_sg_integer_type_header(sg_type);
		switch (sg_type->u.side_basic.integer_size_bits) {
		case 8:
		{
			int8_t v;

			if (!sg_type->u.side_basic.len_bits || sg_type->u.side_basic.len_bits + sg_type->u.side_basic.offset_bits > 8)
				abort();
			memcpy(&v, ptr + sg_type->u.side_basic.integer_offset, sizeof(v));
			v >>= sg_type->u.side_basic.offset_bits;
			if (sg_type->u.side_basic.len_bits < 8)
				v &= (1U << sg_type->u.side_basic.len_bits) - 1;
			switch (base) {
			case TRACER_DISPLAY_BASE_2:
				print_integer_binary(v, sg_type->u.side_basic.len_bits);
				break;
			case TRACER_DISPLAY_BASE_8:
				printf("0%" PRIo8, v);
				break;
			case TRACER_DISPLAY_BASE_10:
				/* Sign-extend. */
				if (sg_type->u.side_basic.len_bits < 8) {
					if (v & (1U << (sg_type->u.side_basic.len_bits - 1)))
						v |= ~((1U << sg_type->u.side_basic.len_bits) - 1);
				}
				printf("%" PRId8, v);
				break;
			case TRACER_DISPLAY_BASE_16:
				printf("0x%" PRIx8, v);
				break;
			default:
				abort();
			}
			break;
		}
		case 16:
		{
			int16_t v;

			if (!sg_type->u.side_basic.len_bits || sg_type->u.side_basic.len_bits + sg_type->u.side_basic.offset_bits > 16)
				abort();
			memcpy(&v, ptr + sg_type->u.side_basic.integer_offset, sizeof(v));
			if (sg_type_to_host_reverse_bo(sg_type))
				v = side_bswap_16(v);
			v >>= sg_type->u.side_basic.offset_bits;
			if (sg_type->u.side_basic.len_bits < 16)
				v &= (1U << sg_type->u.side_basic.len_bits) - 1;
			switch (base) {
			case TRACER_DISPLAY_BASE_2:
				print_integer_binary(v, sg_type->u.side_basic.len_bits);
				break;
			case TRACER_DISPLAY_BASE_8:
				printf("0%" PRIo16, v);
				break;
			case TRACER_DISPLAY_BASE_10:
				/* Sign-extend. */
				if (sg_type->u.side_basic.len_bits < 16) {
					if (v & (1U << (sg_type->u.side_basic.len_bits - 1)))
						v |= ~((1U << sg_type->u.side_basic.len_bits) - 1);
				}
				printf("%" PRId16, v);
				break;
			case TRACER_DISPLAY_BASE_16:
				printf("0x%" PRIx16, v);
				break;
			default:
				abort();
			}
			break;
		}
		case 32:
		{
			uint32_t v;

			if (!sg_type->u.side_basic.len_bits || sg_type->u.side_basic.len_bits + sg_type->u.side_basic.offset_bits > 32)
				abort();
			memcpy(&v, ptr + sg_type->u.side_basic.integer_offset, sizeof(v));
			if (sg_type_to_host_reverse_bo(sg_type))
				v = side_bswap_32(v);
			v >>= sg_type->u.side_basic.offset_bits;
			if (sg_type->u.side_basic.len_bits < 32)
				v &= (1U << sg_type->u.side_basic.len_bits) - 1;
			switch (base) {
			case TRACER_DISPLAY_BASE_2:
				print_integer_binary(v, sg_type->u.side_basic.len_bits);
				break;
			case TRACER_DISPLAY_BASE_8:
				printf("0%" PRIo32, v);
				break;
			case TRACER_DISPLAY_BASE_10:
				/* Sign-extend. */
				if (sg_type->u.side_basic.len_bits < 32) {
					if (v & (1U << (sg_type->u.side_basic.len_bits - 1)))
						v |= ~((1U << sg_type->u.side_basic.len_bits) - 1);
				}
				printf("%" PRId32, v);
				break;
			case TRACER_DISPLAY_BASE_16:
				printf("0x%" PRIx32, v);
				break;
			default:
				abort();
			}
			break;
		}
		case 64:
		{
			uint64_t v;

			if (!sg_type->u.side_basic.len_bits || sg_type->u.side_basic.len_bits + sg_type->u.side_basic.offset_bits > 64)
				abort();
			memcpy(&v, ptr + sg_type->u.side_basic.integer_offset, sizeof(v));
			if (sg_type_to_host_reverse_bo(sg_type))
				v = side_bswap_64(v);
			v >>= sg_type->u.side_basic.offset_bits;
			if (sg_type->u.side_basic.len_bits < 64)
				v &= (1ULL << sg_type->u.side_basic.len_bits) - 1;
			switch (base) {
			case TRACER_DISPLAY_BASE_2:
				print_integer_binary(v, sg_type->u.side_basic.len_bits);
				break;
			case TRACER_DISPLAY_BASE_8:
				printf("0%" PRIo64, v);
				break;
			case TRACER_DISPLAY_BASE_10:
				/* Sign-extend. */
				if (sg_type->u.side_basic.len_bits < 64) {
					if (v & (1ULL << (sg_type->u.side_basic.len_bits - 1)))
						v |= ~((1ULL << sg_type->u.side_basic.len_bits) - 1);
				}
				printf("%" PRId64, v);
				break;
			case TRACER_DISPLAY_BASE_16:
				printf("0x%" PRIx64, v);
				break;
			default:
				abort();
			}
			break;
		}
		default:
			fprintf(stderr, "<UNKNOWN SCATTER-GATHER INTEGER SIZE>");
			abort();
		}
		break;
	}
	default:
		fprintf(stderr, "<UNKNOWN SCATTER-GATHER TYPE>");
		abort();
	}
	printf(" }");
}

static
void tracer_print_sg_field(const struct side_struct_field_sg *field_sg, void *ptr)
{
	printf("%s: ", field_sg->field_name);
	tracer_print_sg_type(&field_sg->side_type, ptr);
}

static
void tracer_print_struct_sg(const struct side_type_description *type_desc, void *ptr)
{
	const struct side_type_struct_sg *struct_sg = type_desc->u.side_struct_sg;
	int i;

	print_attributes("attr", ":", struct_sg->attr, struct_sg->nr_attr);
	printf("%s", struct_sg->nr_attr ? ", " : "");
	printf("fields: { ");
	for (i = 0; i < struct_sg->nr_fields; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_sg_field(&struct_sg->fields_sg[i], ptr);
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
		fprintf(stderr, "ERROR: length mismatch between description and arguments of array\n");
		abort();
	}
	print_attributes("attr", ":", type_desc->u.side_array.attr, type_desc->u.side_array.nr_attr);
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

	print_attributes("attr", ":", type_desc->u.side_vla.attr, type_desc->u.side_vla.nr_attr);
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

	print_attributes("attr", ":", type_desc->u.side_vla_visitor.attr, type_desc->u.side_vla_visitor.nr_attr);
	printf("%s", type_desc->u.side_vla_visitor.nr_attr ? ", " : "");
	printf("elements: ");
	printf("[ ");
	status = type_desc->u.side_vla_visitor.visitor(&tracer_ctx, app_ctx);
	switch (status) {
	case SIDE_VISITOR_STATUS_OK:
		break;
	case SIDE_VISITOR_STATUS_ERROR:
		fprintf(stderr, "ERROR: Visitor error\n");
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

	print_attributes("attr", ":", type_desc->u.side_array.attr, type_desc->u.side_array.nr_attr);
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
	case SIDE_TYPE_ARRAY_BYTE:
		if (elem_type->type != SIDE_TYPE_BYTE)
			goto type_error;
		break;
	case SIDE_TYPE_ARRAY_POINTER32:
		if (elem_type->type != SIDE_TYPE_POINTER32)
			goto type_error;
	case SIDE_TYPE_ARRAY_POINTER64:
		if (elem_type->type != SIDE_TYPE_POINTER64)
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
		case SIDE_TYPE_BYTE:
			sav_elem.u.side_byte = ((const uint8_t *) p)[i];
			break;
		case SIDE_TYPE_POINTER32:
			sav_elem.u.side_u32 = ((const uint32_t *) p)[i];
			break;
		case SIDE_TYPE_POINTER64:
			sav_elem.u.side_u64 = ((const uint64_t *) p)[i];
			break;

		default:
			fprintf(stderr, "ERROR: Unexpected type\n");
			abort();
		}

		printf("%s", i ? ", " : "");
		tracer_print_type(elem_type, &sav_elem);
	}
	printf(" ]");
	return;

type_error:
	fprintf(stderr, "ERROR: type mismatch\n");
	abort();
}

void tracer_print_vla_fixint(const struct side_type_description *type_desc, const struct side_arg_vec *item)
{
	const struct side_type_description *elem_type = type_desc->u.side_vla.elem_type;
	uint32_t side_sav_len = item->u.side_vla_fixint.length;
	void *p = item->u.side_vla_fixint.p;
	enum side_type side_type;
	int i;

	print_attributes("attr", ":", type_desc->u.side_vla.attr, type_desc->u.side_vla.nr_attr);
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
	case SIDE_TYPE_VLA_BYTE:
		if (elem_type->type != SIDE_TYPE_BYTE)
			goto type_error;
		break;
	case SIDE_TYPE_VLA_POINTER32:
		if (elem_type->type != SIDE_TYPE_POINTER32)
			goto type_error;
	case SIDE_TYPE_VLA_POINTER64:
		if (elem_type->type != SIDE_TYPE_POINTER64)
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
		case SIDE_TYPE_BYTE:
			sav_elem.u.side_byte = ((const uint8_t *) p)[i];
			break;
		case SIDE_TYPE_POINTER32:
			sav_elem.u.side_u32 = ((const uint32_t *) p)[i];
			break;
		case SIDE_TYPE_POINTER64:
			sav_elem.u.side_u64 = ((const uint64_t *) p)[i];
			break;

		default:
			fprintf(stderr, "ERROR: Unexpected type\n");
			abort();
		}

		printf("%s", i ? ", " : "");
		tracer_print_type(elem_type, &sav_elem);
	}
	printf(" ]");
	return;

type_error:
	fprintf(stderr, "ERROR: type mismatch\n");
	abort();
}

static
void tracer_print_dynamic_struct(const struct side_arg_dynamic_event_struct *dynamic_struct)
{
	const struct side_arg_dynamic_event_field *fields = dynamic_struct->fields;
	uint32_t len = dynamic_struct->len;
	int i;

	print_attributes("attr", "::", dynamic_struct->attr, dynamic_struct->nr_attr);
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

	print_attributes("attr", "::", item->u.side_dynamic_struct_visitor.attr, item->u.side_dynamic_struct_visitor.nr_attr);
	printf("%s", item->u.side_dynamic_struct_visitor.nr_attr ? ", " : "");
	printf("fields:: ");
	printf("[ ");
	status = item->u.side_dynamic_struct_visitor.visitor(&tracer_ctx, app_ctx);
	switch (status) {
	case SIDE_VISITOR_STATUS_OK:
		break;
	case SIDE_VISITOR_STATUS_ERROR:
		fprintf(stderr, "ERROR: Visitor error\n");
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

	print_attributes("attr", "::", vla->attr, vla->nr_attr);
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

	print_attributes("attr", "::", item->u.side_dynamic_vla_visitor.attr, item->u.side_dynamic_vla_visitor.nr_attr);
	printf("%s", item->u.side_dynamic_vla_visitor.nr_attr ? ", " : "");
	printf("elements:: ");
	printf("[ ");
	status = item->u.side_dynamic_vla_visitor.visitor(&tracer_ctx, app_ctx);
	switch (status) {
	case SIDE_VISITOR_STATUS_OK:
		break;
	case SIDE_VISITOR_STATUS_ERROR:
		fprintf(stderr, "ERROR: Visitor error\n");
		abort();
	}
	printf(" ]");
}

static
void tracer_print_dynamic_basic_type_header(const struct side_arg_dynamic_vec *item)
{
	print_attributes("attr", "::", item->u.side_basic.attr, item->u.side_basic.nr_attr);
	printf("%s", item->u.side_basic.nr_attr ? ", " : "");
	printf("value:: ");
}

static
void tracer_print_dynamic(const struct side_arg_dynamic_vec *item)
{
	enum tracer_display_base base = TRACER_DISPLAY_BASE_10;

	switch (item->dynamic_type) {
	case SIDE_DYNAMIC_TYPE_U8:
	case SIDE_DYNAMIC_TYPE_U16:
	case SIDE_DYNAMIC_TYPE_U32:
	case SIDE_DYNAMIC_TYPE_U64:
	case SIDE_DYNAMIC_TYPE_S8:
	case SIDE_DYNAMIC_TYPE_S16:
	case SIDE_DYNAMIC_TYPE_S32:
	case SIDE_DYNAMIC_TYPE_S64:
		base = get_attr_display_base(item->u.side_basic.attr,
				item->u.side_basic.nr_attr);
		break;
	default:
		break;
	}


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
	{
		uint8_t v;

		v = item->u.side_basic.u.side_u8;
		tracer_print_dynamic_basic_type_header(item);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 8);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo8, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRIu8, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx8, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_DYNAMIC_TYPE_U16:
	{
		uint16_t v;

		v = item->u.side_basic.u.side_u16;
		if (dynamic_type_to_host_reverse_bo(item))
			v = side_bswap_16(v);
		tracer_print_dynamic_basic_type_header(item);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 16);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo16, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRIu16, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx16, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_DYNAMIC_TYPE_U32:
	{
		uint32_t v;

		v = item->u.side_basic.u.side_u32;
		if (dynamic_type_to_host_reverse_bo(item))
			v = side_bswap_32(v);
		tracer_print_dynamic_basic_type_header(item);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 32);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo32, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRIu32, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx32, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_DYNAMIC_TYPE_U64:
	{
		uint64_t v;

		v = item->u.side_basic.u.side_u64;
		if (dynamic_type_to_host_reverse_bo(item))
			v = side_bswap_64(v);
		tracer_print_dynamic_basic_type_header(item);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 64);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo64, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRIu64, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx64, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_DYNAMIC_TYPE_S8:
	{
		int8_t v;

		v = item->u.side_basic.u.side_s8;
		tracer_print_dynamic_basic_type_header(item);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 8);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo8, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRId8, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx8, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_DYNAMIC_TYPE_S16:
	{
		int16_t v;

		v = item->u.side_basic.u.side_u16;
		if (dynamic_type_to_host_reverse_bo(item))
			v = side_bswap_16(v);
		tracer_print_dynamic_basic_type_header(item);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 16);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo16, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRId16, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx16, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_DYNAMIC_TYPE_S32:
	{
		int32_t v;

		v = item->u.side_basic.u.side_u32;
		if (dynamic_type_to_host_reverse_bo(item))
			v = side_bswap_32(v);
		tracer_print_dynamic_basic_type_header(item);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 32);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo32, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRId32, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx32, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_DYNAMIC_TYPE_S64:
	{
		int64_t v;

		v = item->u.side_basic.u.side_u64;
		if (dynamic_type_to_host_reverse_bo(item))
			v = side_bswap_64(v);
		tracer_print_dynamic_basic_type_header(item);
		switch (base) {
		case TRACER_DISPLAY_BASE_2:
			print_integer_binary(v, 64);
			break;
		case TRACER_DISPLAY_BASE_8:
			printf("0%" PRIo64, v);
			break;
		case TRACER_DISPLAY_BASE_10:
			printf("%" PRId64, v);
			break;
		case TRACER_DISPLAY_BASE_16:
			printf("0x%" PRIx64, v);
			break;
		default:
			abort();
		}
		break;
	}
	case SIDE_DYNAMIC_TYPE_BYTE:
		tracer_print_dynamic_basic_type_header(item);
		printf("0x%" PRIx8, item->u.side_basic.u.side_byte);
		break;
	case SIDE_DYNAMIC_TYPE_POINTER32:
	{
		uint32_t v;

		v = item->u.side_basic.u.side_u32;
		if (dynamic_type_to_host_reverse_bo(item))
			v = side_bswap_32(v);
		tracer_print_dynamic_basic_type_header(item);
		printf("0x%" PRIx32, v);
		break;
	}

	case SIDE_DYNAMIC_TYPE_POINTER64:
	{
		uint64_t v;

		v = item->u.side_basic.u.side_u64;
		if (dynamic_type_to_host_reverse_bo(item))
			v = side_bswap_64(v);
		tracer_print_dynamic_basic_type_header(item);
		printf("0x%" PRIx64, v);
		break;
	}

	case SIDE_DYNAMIC_TYPE_FLOAT_BINARY16:
	{
#if __HAVE_FLOAT16
		union {
			_Float16 f;
			uint16_t u;
		} float16 = {
			.f = item->u.side_basic.u.side_float_binary16,
		};

		if (dynamic_type_to_host_reverse_bo(item))
			float16.u = side_bswap_16(float16.u);
		tracer_print_dynamic_basic_type_header(item);
		printf("%g", (double) float16.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary16 float type\n");
		abort();
#endif
	}
	case SIDE_DYNAMIC_TYPE_FLOAT_BINARY32:
	{
#if __HAVE_FLOAT32
		union {
			_Float32 f;
			uint32_t u;
		} float32 = {
			.f = item->u.side_basic.u.side_float_binary32,
		};

		if (dynamic_type_to_host_reverse_bo(item))
			float32.u = side_bswap_32(float32.u);
		tracer_print_dynamic_basic_type_header(item);
		printf("%g", (double) float32.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary32 float type\n");
		abort();
#endif
	}
	case SIDE_DYNAMIC_TYPE_FLOAT_BINARY64:
	{
#if __HAVE_FLOAT64
		union {
			_Float64 f;
			uint64_t u;
		} float64 = {
			.f = item->u.side_basic.u.side_float_binary64,
		};

		if (dynamic_type_to_host_reverse_bo(item))
			float64.u = side_bswap_64(float64.u);
		tracer_print_dynamic_basic_type_header(item);
		printf("%g", (double) float64.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary64 float type\n");
		abort();
#endif
	}
	case SIDE_DYNAMIC_TYPE_FLOAT_BINARY128:
	{
#if __HAVE_FLOAT128
		union {
			_Float128 f;
			char arr[16];
		} float128 = {
			.f = item->u.side_basic.u.side_float_binary128,
		};

		if (dynamic_type_to_host_reverse_bo(item))
			side_bswap_128p(float128.arr);
		tracer_print_dynamic_basic_type_header(item);
		printf("%Lg", (long double) float128.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary128 float type\n");
		abort();
#endif
	}
	case SIDE_DYNAMIC_TYPE_STRING:
		tracer_print_dynamic_basic_type_header(item);
		printf("\"%s\"", (const char *)(uintptr_t) item->u.side_basic.u.string);
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
		fprintf(stderr, "<UNKNOWN TYPE>");
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
		fprintf(stderr, "ERROR: number of fields mismatch between description and arguments\n");
		abort();
	}
	print_attributes(", attr", ":", desc->attr, desc->nr_attr);
	printf("%s", side_sav_len ? ", fields: [ " : "");
	for (i = 0; i < side_sav_len; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_field(&desc->fields[i], &sav[i]);
	}
	if (nr_items)
		*nr_items = i;
	if (side_sav_len)
		printf(" ]");
}

void tracer_call(const struct side_event_description *desc,
		const struct side_arg_vec_description *sav_desc,
		void *priv __attribute__((unused)))
{
	int nr_fields = 0;

	tracer_print_static_fields(desc, sav_desc, &nr_fields);
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
		fprintf(stderr, "ERROR: unexpected non-variadic event description\n");
		abort();
	}
	print_attributes(", attr ", "::", var_struct->attr, var_struct->nr_attr);
	printf("%s", var_struct_len ? ", fields:: [ " : "");
	for (i = 0; i < var_struct_len; i++, nr_fields++) {
		printf("%s", i ? ", " : "");
		printf("%s:: ", var_struct->fields[i].field_name);
		tracer_print_dynamic(&var_struct->fields[i].elem);
	}
	if (i)
		printf(" ]");
	printf("\n");
}

void tracer_event_notification(enum side_tracer_notification notif,
		struct side_event_description **events, uint32_t nr_events, void *priv)
{
	uint32_t i;
	int ret;

	printf("----------------------------------------------------------\n");
	printf("Tracer notified of events %s\n",
		notif == SIDE_TRACER_NOTIFICATION_INSERT_EVENTS ? "inserted" : "removed");
	for (i = 0; i < nr_events; i++) {
		struct side_event_description *event = events[i];

		/* Skip NULL pointers */
		if (!event)
			continue;
		printf("provider: %s, event: %s\n",
			event->provider_name, event->event_name);
		if  (notif == SIDE_TRACER_NOTIFICATION_INSERT_EVENTS) {
			if (event->flags & SIDE_EVENT_FLAG_VARIADIC) {
				ret = side_tracer_callback_variadic_register(event, tracer_call_variadic, NULL);
				if (ret)
					abort();
			} else {
				ret = side_tracer_callback_register(event, tracer_call, NULL);
				if (ret)
					abort();
			}
		} else {
			if (event->flags & SIDE_EVENT_FLAG_VARIADIC) {
				ret = side_tracer_callback_variadic_unregister(event, tracer_call_variadic, NULL);
				if (ret)
					abort();
			} else {
				ret = side_tracer_callback_unregister(event, tracer_call, NULL);
				if (ret)
					abort();
			}
		}
	}
	printf("----------------------------------------------------------\n");
}

static __attribute__((constructor))
void tracer_init(void);
static
void tracer_init(void)
{
	tracer_handle = side_tracer_event_notification_register(tracer_event_notification, NULL);
	if (!tracer_handle)
		abort();
}

static __attribute__((destructor))
void tracer_exit(void);
static
void tracer_exit(void)
{
	side_tracer_event_notification_unregister(tracer_handle);
}
