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
void tracer_print_struct(const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec);
static
void tracer_print_array(const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec);
static
void tracer_print_vla(const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec);
static
void tracer_print_vla_visitor(const struct side_type *type_desc, void *app_ctx);
static
void tracer_print_dynamic(const struct side_arg *dynamic_item);
static
uint32_t tracer_print_gather_bool_type(const struct side_type_gather *type_gather, const void *_ptr);
static
uint32_t tracer_print_gather_byte_type(const struct side_type_gather *type_gather, const void *_ptr);
static
uint32_t tracer_print_gather_integer_type(const struct side_type_gather *type_gather, const void *_ptr);
static
uint32_t tracer_print_gather_float_type(const struct side_type_gather *type_gather, const void *_ptr);
static
uint32_t tracer_print_gather_struct(const struct side_type_gather *type_gather, const void *_ptr);
static
uint32_t tracer_print_gather_array(const struct side_type_gather *type_gather, const void *_ptr);
static
uint32_t tracer_print_gather_vla(const struct side_type_gather *type_gather, const void *_ptr,
		const void *_length_ptr);
static
void tracer_print_type(const struct side_type *type_desc, const struct side_arg *item);

static
int64_t get_attr_integer_value(const struct side_attr *attr)
{
	int64_t val;

	switch (attr->value.type) {
	case SIDE_ATTR_TYPE_U8:
		val = attr->value.u.integer_value.side_u8;
		break;
	case SIDE_ATTR_TYPE_U16:
		val = attr->value.u.integer_value.side_u16;
		break;
	case SIDE_ATTR_TYPE_U32:
		val = attr->value.u.integer_value.side_u32;
		break;
	case SIDE_ATTR_TYPE_U64:
		val = attr->value.u.integer_value.side_u64;
		break;
	case SIDE_ATTR_TYPE_S8:
		val = attr->value.u.integer_value.side_s8;
		break;
	case SIDE_ATTR_TYPE_S16:
		val = attr->value.u.integer_value.side_s16;
		break;
	case SIDE_ATTR_TYPE_S32:
		val = attr->value.u.integer_value.side_s32;
		break;
	case SIDE_ATTR_TYPE_S64:
		val = attr->value.u.integer_value.side_s64;
		break;
	default:
		fprintf(stderr, "Unexpected attribute type\n");
		abort();
	}
	return val;
}

static
enum tracer_display_base get_attr_display_base(const struct side_attr *_attr, uint32_t nr_attr,
				enum tracer_display_base default_base)
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
	return default_base;	/* Default */
}

static
bool type_to_host_reverse_bo(const struct side_type *type_desc)
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
		if (type_desc->u.side_integer.byte_order != SIDE_TYPE_BYTE_ORDER_HOST)
			return true;
		else
			return false;
		break;
        case SIDE_TYPE_FLOAT_BINARY16:
        case SIDE_TYPE_FLOAT_BINARY32:
        case SIDE_TYPE_FLOAT_BINARY64:
        case SIDE_TYPE_FLOAT_BINARY128:
		if (type_desc->u.side_float.byte_order != SIDE_TYPE_FLOAT_WORD_ORDER_HOST)
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
		printf("%s", attr->value.u.bool_value ? "true" : "false");
		break;
	case SIDE_ATTR_TYPE_U8:
		printf("%" PRIu8, attr->value.u.integer_value.side_u8);
		break;
	case SIDE_ATTR_TYPE_U16:
		printf("%" PRIu16, attr->value.u.integer_value.side_u16);
		break;
	case SIDE_ATTR_TYPE_U32:
		printf("%" PRIu32, attr->value.u.integer_value.side_u32);
		break;
	case SIDE_ATTR_TYPE_U64:
		printf("%" PRIu64, attr->value.u.integer_value.side_u64);
		break;
	case SIDE_ATTR_TYPE_S8:
		printf("%" PRId8, attr->value.u.integer_value.side_s8);
		break;
	case SIDE_ATTR_TYPE_S16:
		printf("%" PRId16, attr->value.u.integer_value.side_s16);
		break;
	case SIDE_ATTR_TYPE_S32:
		printf("%" PRId32, attr->value.u.integer_value.side_s32);
		break;
	case SIDE_ATTR_TYPE_S64:
		printf("%" PRId64, attr->value.u.integer_value.side_s64);
		break;
	case SIDE_ATTR_TYPE_POINTER32:
		printf("0x%" PRIx32, attr->value.u.integer_value.side_u32);
		break;
	case SIDE_ATTR_TYPE_POINTER64:
		printf("0x%" PRIx64, attr->value.u.integer_value.side_u64);
		break;
	case SIDE_ATTR_TYPE_FLOAT_BINARY16:
#if __HAVE_FLOAT16
		printf("%g", (double) attr->value.u.float_value.side_float_binary16);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary16 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_FLOAT_BINARY32:
#if __HAVE_FLOAT32
		printf("%g", (double) attr->value.u.float_value.side_float_binary32);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary32 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_FLOAT_BINARY64:
#if __HAVE_FLOAT64
		printf("%g", (double) attr->value.u.float_value.side_float_binary64);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary64 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_FLOAT_BINARY128:
#if __HAVE_FLOAT128
		printf("%Lg", (long double) attr->value.u.float_value.side_float_binary128);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary128 float type\n");
		abort();
#endif
	case SIDE_ATTR_TYPE_STRING:
		printf("\"%s\"", (const char *)(uintptr_t) attr->value.u.string_value);
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
	uint32_t i;

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
void print_enum(const struct side_type *type_desc, const struct side_arg *item)
{
	const struct side_enum_mappings *mappings = type_desc->u.side_enum.mappings;
	const struct side_type *elem_type = type_desc->u.side_enum.elem_type;
	uint32_t i, print_count = 0;
	int64_t value;

	if (elem_type->type != item->type) {
		fprintf(stderr, "ERROR: Unexpected enum element type\n");
		abort();
	}
	switch (item->type) {
	case SIDE_TYPE_U8:
		value = (int64_t) item->u.side_static.integer_value.side_u8;
		break;
	case SIDE_TYPE_U16:
	{
		uint16_t v;

		v = item->u.side_static.integer_value.side_u16;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_16(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_U32:
	{
		uint32_t v;

		v = item->u.side_static.integer_value.side_u32;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_32(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_U64:
	{
		uint64_t v;

		v = item->u.side_static.integer_value.side_u64;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_64(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_S8:
		value = (int64_t) item->u.side_static.integer_value.side_s8;
		break;
	case SIDE_TYPE_S16:
	{
		int16_t v;

		v = item->u.side_static.integer_value.side_s16;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_16(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_S32:
	{
		int32_t v;

		v = item->u.side_static.integer_value.side_s32;
		if (type_to_host_reverse_bo(elem_type))
			v = side_bswap_32(v);
		value = (int64_t) v;
		break;
	}
	case SIDE_TYPE_S64:
	{
		int64_t v;

		v = item->u.side_static.integer_value.side_s64;
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
uint32_t enum_elem_type_to_stride(const struct side_type *elem_type)
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
void print_enum_bitmap(const struct side_type *type_desc,
		const struct side_arg *item)
{
	const struct side_type *elem_type = type_desc->u.side_enum_bitmap.elem_type;
	const struct side_enum_bitmap_mappings *side_enum_mappings = type_desc->u.side_enum_bitmap.mappings;
	uint32_t i, print_count = 0, stride_bit, nr_items;
	bool reverse_byte_order = false;
	const struct side_arg *array_item;

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
		array_item = item->u.side_static.side_array->sav;
		nr_items = type_desc->u.side_array.length;
		break;
	case SIDE_TYPE_VLA:
		stride_bit = enum_elem_type_to_stride(elem_type->u.side_vla.elem_type);
		reverse_byte_order = type_to_host_reverse_bo(elem_type->u.side_vla.elem_type);
		array_item = item->u.side_static.side_vla->sav;
		nr_items = item->u.side_static.side_vla->len;
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
				uint8_t v = array_item[bit / 8].u.side_static.integer_value.side_u8;
				if (v & (1ULL << (bit % 8))) {
					match = true;
					goto match;
				}
				break;
			}
			case 16:
			{
				uint16_t v = array_item[bit / 16].u.side_static.integer_value.side_u16;
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
				uint32_t v = array_item[bit / 32].u.side_static.integer_value.side_u32;
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
				uint64_t v = array_item[bit / 64].u.side_static.integer_value.side_u64;
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
void tracer_print_type_header(const char *separator,
		const struct side_attr *attr, uint32_t nr_attr)
{
	print_attributes("attr", separator, attr, nr_attr);
	printf("%s", nr_attr ? ", " : "");
	printf("value%s ", separator);
}

static
void tracer_print_type_bool(const char *separator,
		const struct side_type_bool *type_bool,
		const union side_bool_value *value,
		uint16_t offset_bits)
{
	uint32_t len_bits;
	bool reverse_bo;
	uint64_t v;

	if (!type_bool->len_bits)
		len_bits = type_bool->bool_size * CHAR_BIT;
	else
		len_bits = type_bool->len_bits;
	if (len_bits + offset_bits > type_bool->bool_size * CHAR_BIT)
		abort();
	reverse_bo = type_bool->byte_order != SIDE_TYPE_BYTE_ORDER_HOST;
	switch (type_bool->bool_size) {
	case 1:
		v = value->side_bool8;
		break;
	case 2:
	{
		uint16_t side_u16;

		side_u16 = value->side_bool16;
		if (reverse_bo)
			side_u16 = side_bswap_16(side_u16);
		v = side_u16;
		break;
	}
	case 4:
	{
		uint32_t side_u32;

		side_u32 = value->side_bool32;
		if (reverse_bo)
			side_u32 = side_bswap_32(side_u32);
		v = side_u32;
		break;
	}
	case 8:
	{
		uint64_t side_u64;

		side_u64 = value->side_bool64;
		if (reverse_bo)
			side_u64 = side_bswap_64(side_u64);
		v = side_u64;
		break;
	}
	default:
		abort();
	}
	v >>= offset_bits;
	if (len_bits < 64)
		v &= (1ULL << len_bits) - 1;
	tracer_print_type_header(separator, type_bool->attr, type_bool->nr_attr);
	printf("%s", v ? "true" : "false");
}

static
void tracer_print_type_integer(const char *separator,
		const struct side_type_integer *type_integer,
		const union side_integer_value *value,
		uint16_t offset_bits,
		enum tracer_display_base default_base)
{
	enum tracer_display_base base;
	uint32_t len_bits;
	bool reverse_bo;
	union {
		uint64_t v_unsigned;
		int64_t v_signed;
	} v;

	if (!type_integer->len_bits)
		len_bits = type_integer->integer_size * CHAR_BIT;
	else
		len_bits = type_integer->len_bits;
	if (len_bits + offset_bits > type_integer->integer_size * CHAR_BIT)
		abort();
	reverse_bo = type_integer->byte_order != SIDE_TYPE_BYTE_ORDER_HOST;
	base = get_attr_display_base(type_integer->attr,
			type_integer->nr_attr,
			default_base);
	switch (type_integer->integer_size) {
	case 1:
		if (type_integer->signedness)
			v.v_signed = value->side_s8;
		else
			v.v_unsigned = value->side_u8;
		break;
	case 2:
		if (type_integer->signedness) {
			int16_t side_s16;

			side_s16 = value->side_s16;
			if (reverse_bo)
				side_s16 = side_bswap_16(side_s16);
			v.v_signed = side_s16;
		} else {
			uint16_t side_u16;

			side_u16 = value->side_u16;
			if (reverse_bo)
				side_u16 = side_bswap_16(side_u16);
			v.v_unsigned = side_u16;
		}
		break;
	case 4:
		if (type_integer->signedness) {
			int32_t side_s32;

			side_s32 = value->side_s32;
			if (reverse_bo)
				side_s32 = side_bswap_32(side_s32);
			v.v_signed = side_s32;
		} else {
			uint32_t side_u32;

			side_u32 = value->side_u32;
			if (reverse_bo)
				side_u32 = side_bswap_32(side_u32);
			v.v_unsigned = side_u32;
		}
		break;
	case 8:
		if (type_integer->signedness) {
			int64_t side_s64;

			side_s64 = value->side_s64;
			if (reverse_bo)
				side_s64 = side_bswap_64(side_s64);
			v.v_signed = side_s64;
		} else {
			uint64_t side_u64;

			side_u64 = value->side_u64;
			if (reverse_bo)
				side_u64 = side_bswap_64(side_u64);
			v.v_unsigned = side_u64;
		}
		break;
	default:
		abort();
	}
	v.v_unsigned >>= offset_bits;
	if (len_bits < 64)
		v.v_unsigned &= (1ULL << len_bits) - 1;
	tracer_print_type_header(separator, type_integer->attr, type_integer->nr_attr);
	switch (base) {
	case TRACER_DISPLAY_BASE_2:
		print_integer_binary(v.v_unsigned, len_bits);
		break;
	case TRACER_DISPLAY_BASE_8:
		printf("0%" PRIo64, v.v_unsigned);
		break;
	case TRACER_DISPLAY_BASE_10:
		if (type_integer->signedness) {
			/* Sign-extend. */
			if (len_bits < 64) {
				if (v.v_unsigned & (1ULL << (len_bits - 1)))
					v.v_unsigned |= ~((1ULL << len_bits) - 1);
			}
			printf("%" PRId64, v.v_signed);
		} else {
			printf("%" PRIu64, v.v_unsigned);
		}
		break;
	case TRACER_DISPLAY_BASE_16:
		printf("0x%" PRIx64, v.v_unsigned);
		break;
	default:
		abort();
	}
}

static
void tracer_print_type_float(const char *separator,
		const struct side_type_float *type_float,
		const union side_float_value *value)
{
	bool reverse_bo;

	reverse_bo = type_float->byte_order != SIDE_TYPE_FLOAT_WORD_ORDER_HOST;
	tracer_print_type_header(separator, type_float->attr, type_float->nr_attr);
	switch (type_float->float_size) {
	case 2:
	{
#if __HAVE_FLOAT16
		union {
			_Float16 f;
			uint16_t u;
		} float16 = {
			.f = value->side_float_binary16,
		};

		if (reverse_bo)
			float16.u = side_bswap_16(float16.u);
		tracer_print_type_header(":", type_desc->u.side_float.attr, type_desc->u.side_float.nr_attr);
		printf("%g", (double) float16.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary16 float type\n");
		abort();
#endif
	}
	case 4:
	{
#if __HAVE_FLOAT32
		union {
			_Float32 f;
			uint32_t u;
		} float32 = {
			.f = value->side_float_binary32,
		};

		if (reverse_bo)
			float32.u = side_bswap_32(float32.u);
		printf("%g", (double) float32.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary32 float type\n");
		abort();
#endif
	}
	case 8:
	{
#if __HAVE_FLOAT64
		union {
			_Float64 f;
			uint64_t u;
		} float64 = {
			.f = value->side_float_binary64,
		};

		if (reverse_bo)
			float64.u = side_bswap_64(float64.u);
		printf("%g", (double) float64.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary64 float type\n");
		abort();
#endif
	}
	case 16:
	{
#if __HAVE_FLOAT128
		union {
			_Float128 f;
			char arr[16];
		} float128 = {
			.f = value->side_float_binary128,
		};

		if (reverse_bo)
			side_bswap_128p(float128.arr);
		printf("%Lg", (long double) float128.f);
		break;
#else
		fprintf(stderr, "ERROR: Unsupported binary128 float type\n");
		abort();
#endif
	}
	default:
		fprintf(stderr, "ERROR: Unknown float size\n");
		abort();
	}
}

static
void tracer_print_type(const struct side_type *type_desc, const struct side_arg *item)
{
	enum side_type_label type;

	switch (type_desc->type) {
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

	case SIDE_TYPE_DYNAMIC:
		switch (item->type) {
		case SIDE_TYPE_DYNAMIC_NULL:
		case SIDE_TYPE_DYNAMIC_BOOL:
		case SIDE_TYPE_DYNAMIC_U8:
		case SIDE_TYPE_DYNAMIC_U16:
		case SIDE_TYPE_DYNAMIC_U32:
		case SIDE_TYPE_DYNAMIC_U64:
		case SIDE_TYPE_DYNAMIC_S8:
		case SIDE_TYPE_DYNAMIC_S16:
		case SIDE_TYPE_DYNAMIC_S32:
		case SIDE_TYPE_DYNAMIC_S64:
		case SIDE_TYPE_DYNAMIC_BYTE:
		case SIDE_TYPE_DYNAMIC_POINTER32:
		case SIDE_TYPE_DYNAMIC_POINTER64:
		case SIDE_TYPE_DYNAMIC_FLOAT_BINARY16:
		case SIDE_TYPE_DYNAMIC_FLOAT_BINARY32:
		case SIDE_TYPE_DYNAMIC_FLOAT_BINARY64:
		case SIDE_TYPE_DYNAMIC_FLOAT_BINARY128:
		case SIDE_TYPE_DYNAMIC_STRING:
		case SIDE_TYPE_DYNAMIC_STRUCT:
		case SIDE_TYPE_DYNAMIC_STRUCT_VISITOR:
		case SIDE_TYPE_DYNAMIC_VLA:
		case SIDE_TYPE_DYNAMIC_VLA_VISITOR:
			break;
		default:
			fprintf(stderr, "ERROR: Unexpected dynamic type\n");
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
		type = (enum side_type_label) type_desc->type;
	else
		type = (enum side_type_label) item->type;

	printf("{ ");
	switch (type) {
	case SIDE_TYPE_NULL:
		tracer_print_type_header(":", type_desc->u.side_null.attr, type_desc->u.side_null.nr_attr);
		printf("<NULL TYPE>");
		break;

	case SIDE_TYPE_BOOL:
		tracer_print_type_bool(":", &type_desc->u.side_bool, &item->u.side_static.bool_value, 0);
		break;

	case SIDE_TYPE_U8:
	case SIDE_TYPE_U16:
	case SIDE_TYPE_U32:
	case SIDE_TYPE_U64:
	case SIDE_TYPE_S8:
	case SIDE_TYPE_S16:
	case SIDE_TYPE_S32:
	case SIDE_TYPE_S64:
		tracer_print_type_integer(":", &type_desc->u.side_integer, &item->u.side_static.integer_value, 0,
				TRACER_DISPLAY_BASE_10);
		break;

	case SIDE_TYPE_POINTER32:
	case SIDE_TYPE_POINTER64:
		tracer_print_type_integer(":", &type_desc->u.side_integer, &item->u.side_static.integer_value, 0,
				TRACER_DISPLAY_BASE_16);
		break;

	case SIDE_TYPE_BYTE:
		tracer_print_type_header(":", type_desc->u.side_byte.attr, type_desc->u.side_byte.nr_attr);
		printf("0x%" PRIx8, item->u.side_static.byte_value);
		break;

	case SIDE_TYPE_ENUM:
		print_enum(type_desc, item);
		break;

	case SIDE_TYPE_ENUM_BITMAP:
		print_enum_bitmap(type_desc, item);
		break;

	case SIDE_TYPE_FLOAT_BINARY16:
	case SIDE_TYPE_FLOAT_BINARY32:
	case SIDE_TYPE_FLOAT_BINARY64:
	case SIDE_TYPE_FLOAT_BINARY128:
		tracer_print_type_float(":", &type_desc->u.side_float, &item->u.side_static.float_value);
		break;

	case SIDE_TYPE_STRING:
		tracer_print_type_header(":", type_desc->u.side_string.attr, type_desc->u.side_string.nr_attr);
		printf("\"%s\"", (const char *)(uintptr_t) item->u.side_static.string_value);
		break;
	case SIDE_TYPE_STRUCT:
		tracer_print_struct(type_desc, item->u.side_static.side_struct);
		break;
	case SIDE_TYPE_GATHER_STRUCT:
		(void) tracer_print_gather_struct(&type_desc->u.side_gather, item->u.side_static.side_struct_gather_ptr);
		break;
	case SIDE_TYPE_GATHER_ARRAY:
		(void) tracer_print_gather_array(&type_desc->u.side_gather, item->u.side_static.side_array_gather_ptr);
		break;
	case SIDE_TYPE_GATHER_VLA:
		(void) tracer_print_gather_vla(&type_desc->u.side_gather, item->u.side_static.side_vla_gather.ptr,
				item->u.side_static.side_vla_gather.length_ptr);
		break;
	case SIDE_TYPE_GATHER_BOOL:
		(void) tracer_print_gather_bool_type(&type_desc->u.side_gather, item->u.side_static.side_bool_gather_ptr);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		(void) tracer_print_gather_byte_type(&type_desc->u.side_gather, item->u.side_static.side_byte_gather_ptr);
		break;
	case SIDE_TYPE_GATHER_UNSIGNED_INT:
	case SIDE_TYPE_GATHER_SIGNED_INT:
		(void) tracer_print_gather_integer_type(&type_desc->u.side_gather, item->u.side_static.side_integer_gather_ptr);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		(void) tracer_print_gather_float_type(&type_desc->u.side_gather, item->u.side_static.side_float_gather_ptr);
		break;
	case SIDE_TYPE_ARRAY:
		tracer_print_array(type_desc, item->u.side_static.side_array);
		break;
	case SIDE_TYPE_VLA:
		tracer_print_vla(type_desc, item->u.side_static.side_vla);
		break;
	case SIDE_TYPE_VLA_VISITOR:
		tracer_print_vla_visitor(type_desc, item->u.side_static.side_vla_app_visitor_ctx);
		break;

	/* Dynamic types */
	case SIDE_TYPE_DYNAMIC_NULL:
	case SIDE_TYPE_DYNAMIC_BOOL:
	case SIDE_TYPE_DYNAMIC_U8:
	case SIDE_TYPE_DYNAMIC_U16:
	case SIDE_TYPE_DYNAMIC_U32:
	case SIDE_TYPE_DYNAMIC_U64:
	case SIDE_TYPE_DYNAMIC_S8:
	case SIDE_TYPE_DYNAMIC_S16:
	case SIDE_TYPE_DYNAMIC_S32:
	case SIDE_TYPE_DYNAMIC_S64:
	case SIDE_TYPE_DYNAMIC_BYTE:
	case SIDE_TYPE_DYNAMIC_POINTER32:
	case SIDE_TYPE_DYNAMIC_POINTER64:
	case SIDE_TYPE_DYNAMIC_FLOAT_BINARY16:
	case SIDE_TYPE_DYNAMIC_FLOAT_BINARY32:
	case SIDE_TYPE_DYNAMIC_FLOAT_BINARY64:
	case SIDE_TYPE_DYNAMIC_FLOAT_BINARY128:
	case SIDE_TYPE_DYNAMIC_STRING:
	case SIDE_TYPE_DYNAMIC_STRUCT:
	case SIDE_TYPE_DYNAMIC_STRUCT_VISITOR:
	case SIDE_TYPE_DYNAMIC_VLA:
	case SIDE_TYPE_DYNAMIC_VLA_VISITOR:
		tracer_print_dynamic(item);
		break;
	default:
		fprintf(stderr, "<UNKNOWN TYPE>\n");
		abort();
	}
	printf(" }");
}

static
void tracer_print_field(const struct side_event_field *item_desc, const struct side_arg *item)
{
	printf("%s: ", item_desc->field_name);
	tracer_print_type(&item_desc->side_type, item);
}

static
void tracer_print_struct(const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec)
{
	const struct side_arg *sav = side_arg_vec->sav;
	uint32_t i, side_sav_len = side_arg_vec->len;

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
void tracer_print_array(const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec)
{
	const struct side_arg *sav = side_arg_vec->sav;
	uint32_t i, side_sav_len = side_arg_vec->len;

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
void tracer_print_vla(const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec)
{
	const struct side_arg *sav = side_arg_vec->sav;
	uint32_t i, side_sav_len = side_arg_vec->len;

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

static
const char *tracer_gather_access(enum side_type_gather_access_mode access_mode, const char *ptr)
{
	switch (access_mode) {
	case SIDE_TYPE_GATHER_ACCESS_DIRECT:
		return ptr;
	case SIDE_TYPE_GATHER_ACCESS_POINTER:
		/* Dereference pointer */
		memcpy(&ptr, ptr, sizeof(ptr));
		return ptr;
	default:
		abort();
	}
}

static
uint32_t tracer_gather_size(enum side_type_gather_access_mode access_mode, uint32_t len)
{
	switch (access_mode) {
	case SIDE_TYPE_GATHER_ACCESS_DIRECT:
		return len;
	case SIDE_TYPE_GATHER_ACCESS_POINTER:
		return sizeof(void *);
	default:
		abort();
	}
}

static
uint64_t tracer_load_gather_integer_type(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode = type_gather->u.side_integer.access_mode;
	uint32_t integer_size_bytes = type_gather->u.side_integer.type.integer_size;
	const char *ptr = (const char *) _ptr;
	union side_integer_value value;

	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_integer.offset);
	memcpy(&value, ptr, integer_size_bytes);
	switch (type_gather->u.side_integer.type.integer_size) {
	case 1:
		return (uint64_t) value.side_u8;
	case 2:
		return (uint64_t) value.side_u16;
	case 4:
		return (uint64_t) value.side_u32;
	case 8:
		return (uint64_t) value.side_u64;
	default:
		abort();
	}
}

static
uint32_t tracer_print_gather_bool_type(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode = type_gather->u.side_bool.access_mode;
	uint32_t bool_size_bytes = type_gather->u.side_bool.type.bool_size;
	const char *ptr = (const char *) _ptr;
	union side_bool_value value;

	switch (bool_size_bytes) {
	case 1:
	case 2:
	case 4:
	case 8:
		break;
	default:
		abort();
	}
	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_bool.offset);
	memcpy(&value, ptr, bool_size_bytes);
	tracer_print_type_bool(":", &type_gather->u.side_bool.type, &value,
			type_gather->u.side_bool.offset_bits);
	return tracer_gather_size(access_mode, bool_size_bytes);
}

static
uint32_t tracer_print_gather_byte_type(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode = type_gather->u.side_byte.access_mode;
	const char *ptr = (const char *) _ptr;
	uint8_t value;

	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_byte.offset);
	memcpy(&value, ptr, 1);
	tracer_print_type_header(":", type_gather->u.side_byte.type.attr,
			type_gather->u.side_byte.type.nr_attr);
	printf("0x%" PRIx8, value);
	return tracer_gather_size(access_mode, 1);
}

static
uint32_t tracer_print_gather_integer_type(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode = type_gather->u.side_integer.access_mode;
	uint32_t integer_size_bytes = type_gather->u.side_integer.type.integer_size;
	const char *ptr = (const char *) _ptr;
	union side_integer_value value;

	switch (integer_size_bytes) {
	case 1:
	case 2:
	case 4:
	case 8:
		break;
	default:
		abort();
	}
	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_integer.offset);
	memcpy(&value, ptr, integer_size_bytes);
	tracer_print_type_integer(":", &type_gather->u.side_integer.type, &value,
			type_gather->u.side_integer.offset_bits, TRACER_DISPLAY_BASE_10);
	return tracer_gather_size(access_mode, integer_size_bytes);
}

static
uint32_t tracer_print_gather_float_type(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode = type_gather->u.side_float.access_mode;
	uint32_t float_size_bytes = type_gather->u.side_float.type.float_size;
	const char *ptr = (const char *) _ptr;
	union side_float_value value;

	switch (float_size_bytes) {
	case 2:
	case 4:
	case 8:
	case 16:
		break;
	default:
		abort();
	}
	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_float.offset);
	memcpy(&value, ptr, float_size_bytes);
	tracer_print_type_float(":", &type_gather->u.side_float.type, &value);
	return tracer_gather_size(access_mode, float_size_bytes);
}

static
uint32_t tracer_print_gather_type(const struct side_type *type_desc, const void *ptr)
{
	uint32_t len;

	printf("{ ");
	switch (type_desc->type) {
	case SIDE_TYPE_GATHER_BOOL:
		len = tracer_print_gather_bool_type(&type_desc->u.side_gather, ptr);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		len = tracer_print_gather_byte_type(&type_desc->u.side_gather, ptr);
		break;
	case SIDE_TYPE_GATHER_UNSIGNED_INT:
	case SIDE_TYPE_GATHER_SIGNED_INT:
		len = tracer_print_gather_integer_type(&type_desc->u.side_gather, ptr);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		len = tracer_print_gather_float_type(&type_desc->u.side_gather, ptr);
		break;
	case SIDE_TYPE_GATHER_STRUCT:
		len = tracer_print_gather_struct(&type_desc->u.side_gather, ptr);
		break;
	case SIDE_TYPE_GATHER_ARRAY:
		len = tracer_print_gather_array(&type_desc->u.side_gather, ptr);
		break;
	case SIDE_TYPE_GATHER_VLA:
		len = tracer_print_gather_vla(&type_desc->u.side_gather, ptr, ptr);
		break;
	default:
		fprintf(stderr, "<UNKNOWN GATHER TYPE>");
		abort();
	}
	printf(" }");
	return len;
}

static
void tracer_print_gather_field(const struct side_event_field *field, const void *ptr)
{
	printf("%s: ", field->field_name);
	(void) tracer_print_gather_type(&field->side_type, ptr);
}

static
uint32_t tracer_print_gather_struct(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode = type_gather->u.side_struct.access_mode;
	const char *ptr = (const char *) _ptr;
	uint32_t i;

	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_struct.offset);
	print_attributes("attr", ":", type_gather->u.side_struct.type->attr, type_gather->u.side_struct.type->nr_attr);
	printf("%s", type_gather->u.side_struct.type->nr_attr ? ", " : "");
	printf("fields: { ");
	for (i = 0; i < type_gather->u.side_struct.type->nr_fields; i++) {
		printf("%s", i ? ", " : "");
		tracer_print_gather_field(&type_gather->u.side_struct.type->fields[i], ptr);
	}
	printf(" }");
	return tracer_gather_size(access_mode, type_gather->u.side_struct.size);
}

static
uint32_t tracer_print_gather_array(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode = type_gather->u.side_array.access_mode;
	const char *ptr = (const char *) _ptr, *orig_ptr;
	uint32_t i;

	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_array.offset);
	orig_ptr = ptr;
	print_attributes("attr", ":", type_gather->u.side_array.type.attr, type_gather->u.side_array.type.nr_attr);
	printf("%s", type_gather->u.side_array.type.nr_attr ? ", " : "");
	printf("elements: ");
	printf("[ ");
	for (i = 0; i < type_gather->u.side_array.type.length; i++) {
		switch (type_gather->u.side_array.type.elem_type->type) {
		case SIDE_TYPE_GATHER_VLA:
			fprintf(stderr, "<gather VLA only supported within gather structures>\n");
			abort();
		default:
			break;
		}
		printf("%s", i ? ", " : "");
		ptr += tracer_print_gather_type(type_gather->u.side_array.type.elem_type, ptr);
	}
	printf(" ]");
	return tracer_gather_size(access_mode, ptr - orig_ptr);
}

static
uint32_t tracer_print_gather_vla(const struct side_type_gather *type_gather, const void *_ptr,
		const void *_length_ptr)
{
	enum side_type_gather_access_mode access_mode = type_gather->u.side_vla.access_mode;
	const char *ptr = (const char *) _ptr, *orig_ptr;
	const char *length_ptr = (const char *) _length_ptr;
	uint32_t i, length;

	/* Access length */
	switch (type_gather->u.side_vla.length_type->type) {
	case SIDE_TYPE_GATHER_UNSIGNED_INT:
	case SIDE_TYPE_GATHER_SIGNED_INT:
		break;
	default:
		fprintf(stderr, "<gather VLA expects integer gather length type>\n");
		abort();
	}
	length = (uint32_t) tracer_load_gather_integer_type(&type_gather->u.side_vla.length_type->u.side_gather, length_ptr);
	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_vla.offset);
	orig_ptr = ptr;
	print_attributes("attr", ":", type_gather->u.side_vla.type.attr, type_gather->u.side_vla.type.nr_attr);
	printf("%s", type_gather->u.side_vla.type.nr_attr ? ", " : "");
	printf("elements: ");
	printf("[ ");
	for (i = 0; i < length; i++) {
		switch (type_gather->u.side_vla.type.elem_type->type) {
		case SIDE_TYPE_GATHER_VLA:
			fprintf(stderr, "<gather VLA only supported within gather structures>\n");
			abort();
		default:
			break;
		}
		printf("%s", i ? ", " : "");
		ptr += tracer_print_gather_type(type_gather->u.side_vla.type.elem_type, ptr);
	}
	printf(" ]");
	return tracer_gather_size(access_mode, ptr - orig_ptr);
}

struct tracer_visitor_priv {
	const struct side_type *elem_type;
	int i;
};

static
enum side_visitor_status tracer_write_elem_cb(const struct side_tracer_visitor_ctx *tracer_ctx,
			const struct side_arg *elem)
{
	struct tracer_visitor_priv *tracer_priv = (struct tracer_visitor_priv *) tracer_ctx->priv;

	printf("%s", tracer_priv->i++ ? ", " : "");
	tracer_print_type(tracer_priv->elem_type, elem);
	return SIDE_VISITOR_STATUS_OK;
}

static
void tracer_print_vla_visitor(const struct side_type *type_desc, void *app_ctx)
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

static
void tracer_print_dynamic_struct(const struct side_arg_dynamic_struct *dynamic_struct)
{
	const struct side_arg_dynamic_field *fields = dynamic_struct->fields;
	uint32_t i, len = dynamic_struct->len;

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
			const struct side_arg_dynamic_field *dynamic_field)
{
	struct tracer_dynamic_struct_visitor_priv *tracer_priv =
		(struct tracer_dynamic_struct_visitor_priv *) tracer_ctx->priv;

	printf("%s", tracer_priv->i++ ? ", " : "");
	printf("%s:: ", dynamic_field->field_name);
	tracer_print_dynamic(&dynamic_field->elem);
	return SIDE_VISITOR_STATUS_OK;
}

static
void tracer_print_dynamic_struct_visitor(const struct side_arg *item)
{
	enum side_visitor_status status;
	struct tracer_dynamic_struct_visitor_priv tracer_priv = {
		.i = 0,
	};
	const struct side_tracer_dynamic_struct_visitor_ctx tracer_ctx = {
		.write_field = tracer_dynamic_struct_write_elem_cb,
		.priv = &tracer_priv,
	};
	void *app_ctx = item->u.side_dynamic.side_dynamic_struct_visitor.app_ctx;

	print_attributes("attr", "::", item->u.side_dynamic.side_dynamic_struct_visitor.attr, item->u.side_dynamic.side_dynamic_struct_visitor.nr_attr);
	printf("%s", item->u.side_dynamic.side_dynamic_struct_visitor.nr_attr ? ", " : "");
	printf("fields:: ");
	printf("[ ");
	status = item->u.side_dynamic.side_dynamic_struct_visitor.visitor(&tracer_ctx, app_ctx);
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
void tracer_print_dynamic_vla(const struct side_arg_dynamic_vla *vla)
{
	const struct side_arg *sav = vla->sav;
	uint32_t i, side_sav_len = vla->len;

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
			const struct side_tracer_visitor_ctx *tracer_ctx,
			const struct side_arg *elem)
{
	struct tracer_dynamic_vla_visitor_priv *tracer_priv =
		(struct tracer_dynamic_vla_visitor_priv *) tracer_ctx->priv;

	printf("%s", tracer_priv->i++ ? ", " : "");
	tracer_print_dynamic(elem);
	return SIDE_VISITOR_STATUS_OK;
}

static
void tracer_print_dynamic_vla_visitor(const struct side_arg *item)
{
	enum side_visitor_status status;
	struct tracer_dynamic_vla_visitor_priv tracer_priv = {
		.i = 0,
	};
	const struct side_tracer_visitor_ctx tracer_ctx = {
		.write_elem = tracer_dynamic_vla_write_elem_cb,
		.priv = &tracer_priv,
	};
	void *app_ctx = item->u.side_dynamic.side_dynamic_vla_visitor.app_ctx;

	print_attributes("attr", "::", item->u.side_dynamic.side_dynamic_vla_visitor.attr, item->u.side_dynamic.side_dynamic_vla_visitor.nr_attr);
	printf("%s", item->u.side_dynamic.side_dynamic_vla_visitor.nr_attr ? ", " : "");
	printf("elements:: ");
	printf("[ ");
	status = item->u.side_dynamic.side_dynamic_vla_visitor.visitor(&tracer_ctx, app_ctx);
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
void tracer_print_dynamic(const struct side_arg *item)
{
	printf("{ ");
	switch (item->type) {
	case SIDE_TYPE_DYNAMIC_NULL:
		tracer_print_type_header("::", item->u.side_dynamic.side_null.attr, item->u.side_dynamic.side_null.nr_attr);
		printf("<NULL TYPE>");
		break;

	case SIDE_TYPE_DYNAMIC_BOOL:
		tracer_print_type_bool("::", &item->u.side_dynamic.side_bool.type, &item->u.side_dynamic.side_bool.value, 0);
		break;

	case SIDE_TYPE_DYNAMIC_U8:
	case SIDE_TYPE_DYNAMIC_U16:
	case SIDE_TYPE_DYNAMIC_U32:
	case SIDE_TYPE_DYNAMIC_U64:
	case SIDE_TYPE_DYNAMIC_S8:
	case SIDE_TYPE_DYNAMIC_S16:
	case SIDE_TYPE_DYNAMIC_S32:
	case SIDE_TYPE_DYNAMIC_S64:
		tracer_print_type_integer("::", &item->u.side_dynamic.side_integer.type, &item->u.side_dynamic.side_integer.value, 0,
				TRACER_DISPLAY_BASE_10);
		break;
	case SIDE_TYPE_DYNAMIC_BYTE:
		tracer_print_type_header("::", item->u.side_dynamic.side_byte.type.attr, item->u.side_dynamic.side_byte.type.nr_attr);
		printf("0x%" PRIx8, item->u.side_dynamic.side_byte.value);
		break;

	case SIDE_TYPE_DYNAMIC_POINTER32:
	case SIDE_TYPE_DYNAMIC_POINTER64:
		tracer_print_type_integer("::", &item->u.side_dynamic.side_integer.type, &item->u.side_dynamic.side_integer.value, 0,
				TRACER_DISPLAY_BASE_16);
		break;

	case SIDE_TYPE_DYNAMIC_FLOAT_BINARY16:
	case SIDE_TYPE_DYNAMIC_FLOAT_BINARY32:
	case SIDE_TYPE_DYNAMIC_FLOAT_BINARY64:
	case SIDE_TYPE_DYNAMIC_FLOAT_BINARY128:
		tracer_print_type_float("::", &item->u.side_dynamic.side_float.type,
					&item->u.side_dynamic.side_float.value);
		break;

	case SIDE_TYPE_DYNAMIC_STRING:
		tracer_print_type_header("::", item->u.side_dynamic.side_string.type.attr, item->u.side_dynamic.side_string.type.nr_attr);
		printf("\"%s\"", (const char *)(uintptr_t) item->u.side_dynamic.side_string.value);
		break;
	case SIDE_TYPE_DYNAMIC_STRUCT:
		tracer_print_dynamic_struct(item->u.side_dynamic.side_dynamic_struct);
		break;
	case SIDE_TYPE_DYNAMIC_STRUCT_VISITOR:
		tracer_print_dynamic_struct_visitor(item);
		break;
	case SIDE_TYPE_DYNAMIC_VLA:
		tracer_print_dynamic_vla(item->u.side_dynamic.side_dynamic_vla);
		break;
	case SIDE_TYPE_DYNAMIC_VLA_VISITOR:
		tracer_print_dynamic_vla_visitor(item);
		break;
	default:
		fprintf(stderr, "<UNKNOWN TYPE>\n");
		abort();
	}
	printf(" }");
}

static
void tracer_print_static_fields(const struct side_event_description *desc,
		const struct side_arg_vec *side_arg_vec,
		uint32_t *nr_items)
{
	const struct side_arg *sav = side_arg_vec->sav;
	uint32_t i, side_sav_len = side_arg_vec->len;

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
		const struct side_arg_vec *side_arg_vec,
		void *priv __attribute__((unused)))
{
	uint32_t nr_fields = 0;

	tracer_print_static_fields(desc, side_arg_vec, &nr_fields);
	printf("\n");
}

void tracer_call_variadic(const struct side_event_description *desc,
		const struct side_arg_vec *side_arg_vec,
		const struct side_arg_dynamic_struct *var_struct,
		void *priv __attribute__((unused)))
{
	uint32_t nr_fields = 0, i, var_struct_len = var_struct->len;

	tracer_print_static_fields(desc, side_arg_vec, &nr_fields);

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
