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
#include <iconv.h>

#include <side/trace.h>

enum tracer_display_base {
	TRACER_DISPLAY_BASE_2,
	TRACER_DISPLAY_BASE_8,
	TRACER_DISPLAY_BASE_10,
	TRACER_DISPLAY_BASE_16,
};

union int64_value {
	uint64_t u;
	int64_t s;
};

static struct side_tracer_handle *tracer_handle;

static
void tracer_print_struct(const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec);
static
void tracer_print_variant(const struct side_type *type_desc, const struct side_arg_variant *side_arg_variant);
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
uint32_t tracer_print_gather_integer_type(const struct side_type_gather *type_gather, const void *_ptr,
		enum tracer_display_base default_base);
static
uint32_t tracer_print_gather_float_type(const struct side_type_gather *type_gather, const void *_ptr);
static
uint32_t tracer_print_gather_string_type(const struct side_type_gather *type_gather, const void *_ptr);
static
uint32_t tracer_print_gather_enum_type(const struct side_type_gather *type_gather, const void *_ptr);
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
void tracer_convert_string_to_utf8(const void *p, uint8_t unit_size, enum side_type_label_byte_order byte_order,
		size_t *strlen_with_null,
		char **output_str)
{
	size_t ret, inbytesleft = 0, outbytesleft, bufsize;
	const char *str = p, *fromcode;
	char *inbuf = (char *) p, *outbuf, *buf;
	iconv_t cd;

	switch (unit_size) {
	case 1:
		if (strlen_with_null)
			*strlen_with_null = strlen(str) + 1;
		*output_str = (char *) str;
		return;
	case 2:
	{
		const uint16_t *p16 = p;

		switch (byte_order) {
		case SIDE_TYPE_BYTE_ORDER_LE:
		{
			fromcode = "UTF-16LE";
			break;
		}
		case SIDE_TYPE_BYTE_ORDER_BE:
		{
			fromcode = "UTF-16BE";
			break;
		}
		default:
			fprintf(stderr, "Unknown byte order\n");
			abort();
		}
		for (; *p16; p16++)
			inbytesleft += 2;
		/*
		 * Worse case is U+FFFF UTF-16 (2 bytes) converting to
		 * { ef, bf, bf } UTF-8 (3 bytes).
		 */
		bufsize = inbytesleft / 2 * 3 + 1;
		break;
	}
	case 4:
	{
		const uint32_t *p32 = p;

		switch (byte_order) {
		case SIDE_TYPE_BYTE_ORDER_LE:
		{
			fromcode = "UTF-32LE";
			break;
		}
		case SIDE_TYPE_BYTE_ORDER_BE:
		{
			fromcode = "UTF-32BE";
			break;
		}
		default:
			fprintf(stderr, "Unknown byte order\n");
			abort();
		}
		for (; *p32; p32++)
			inbytesleft += 4;
		/*
		 * Each 4-byte UTF-32 character converts to at most a
		 * 4-byte UTF-8 character.
		 */
		bufsize = inbytesleft + 1;
		break;
	}
	default:
		fprintf(stderr, "Unknown string unit size %" PRIu8 "\n", unit_size);
		abort();
	}

	cd = iconv_open("UTF8", fromcode);
	if (cd == (iconv_t) -1) {
		perror("iconv_open");
		abort();
	}
	buf = malloc(bufsize);
	if (!buf) {
		abort();
	}
	outbuf = (char *) buf;
	outbytesleft = bufsize;
	ret = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	if (ret == (size_t) -1) {
		perror("iconv");
		abort();
	}
	if (inbytesleft) {
		fprintf(stderr, "Buffer too small to convert string input\n");
		abort();
	}
	(*outbuf++) = '\0';
	if (iconv_close(cd) == -1) {
		perror("iconv_close");
		abort();
	}
	if (strlen_with_null)
		*strlen_with_null = outbuf - buf;
	*output_str = buf;
}

static
void tracer_print_string(const void *p, uint8_t unit_size, enum side_type_label_byte_order byte_order,
		size_t *strlen_with_null)
{
	char *output_str = NULL;

	tracer_convert_string_to_utf8(p, unit_size, byte_order, strlen_with_null, &output_str);
	printf("\"%s\"", output_str);
	if (output_str != p)
		free(output_str);
}

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
		char *utf8_str = NULL;
		bool cmp;

		tracer_convert_string_to_utf8(attr->key.p, attr->key.unit_size,
			attr->key.byte_order, NULL, &utf8_str);
		cmp = strcmp(utf8_str, "std.integer.base");
		if (utf8_str != attr->key.p)
			free(utf8_str);
		if (!cmp) {
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
void tracer_print_attr_type(const char *separator, const struct side_attr *attr)
{
	char *utf8_str = NULL;

	tracer_convert_string_to_utf8(attr->key.p, attr->key.unit_size,
		attr->key.byte_order, NULL, &utf8_str);
	printf("{ key%s \"%s\", value%s ", separator, utf8_str, separator);
	if (utf8_str != attr->key.p)
		free(utf8_str);
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
		tracer_print_string(attr->value.u.string_value.p,
				attr->value.u.string_value.unit_size,
				attr->value.u.string_value.byte_order, NULL);
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
union int64_value tracer_load_integer_value(const struct side_type_integer *type_integer,
		const union side_integer_value *value,
		uint16_t offset_bits, uint16_t *_len_bits)
{
	union int64_value v64;
	uint16_t len_bits;
	bool reverse_bo;

	if (!type_integer->len_bits)
		len_bits = type_integer->integer_size * CHAR_BIT;
	else
		len_bits = type_integer->len_bits;
	if (len_bits + offset_bits > type_integer->integer_size * CHAR_BIT)
		abort();
	reverse_bo = type_integer->byte_order != SIDE_TYPE_BYTE_ORDER_HOST;
	switch (type_integer->integer_size) {
	case 1:
		if (type_integer->signedness)
			v64.s = value->side_s8;
		else
			v64.u = value->side_u8;
		break;
	case 2:
		if (type_integer->signedness) {
			int16_t side_s16;

			side_s16 = value->side_s16;
			if (reverse_bo)
				side_s16 = side_bswap_16(side_s16);
			v64.s = side_s16;
		} else {
			uint16_t side_u16;

			side_u16 = value->side_u16;
			if (reverse_bo)
				side_u16 = side_bswap_16(side_u16);
			v64.u = side_u16;
		}
		break;
	case 4:
		if (type_integer->signedness) {
			int32_t side_s32;

			side_s32 = value->side_s32;
			if (reverse_bo)
				side_s32 = side_bswap_32(side_s32);
			v64.s = side_s32;
		} else {
			uint32_t side_u32;

			side_u32 = value->side_u32;
			if (reverse_bo)
				side_u32 = side_bswap_32(side_u32);
			v64.u = side_u32;
		}
		break;
	case 8:
		if (type_integer->signedness) {
			int64_t side_s64;

			side_s64 = value->side_s64;
			if (reverse_bo)
				side_s64 = side_bswap_64(side_s64);
			v64.s = side_s64;
		} else {
			uint64_t side_u64;

			side_u64 = value->side_u64;
			if (reverse_bo)
				side_u64 = side_bswap_64(side_u64);
			v64.u = side_u64;
		}
		break;
	default:
		abort();
	}
	v64.u >>= offset_bits;
	if (len_bits < 64) {
		v64.u &= (1ULL << len_bits) - 1;
		if (type_integer->signedness) {
			/* Sign-extend. */
			if (v64.u & (1ULL << (len_bits - 1)))
				v64.u |= ~((1ULL << len_bits) - 1);
		}
	}
	if (_len_bits)
		*_len_bits = len_bits;
	return v64;
}

static
void print_enum_labels(const struct side_enum_mappings *mappings, union int64_value v64)
{
	uint32_t i, print_count = 0;

	printf(", labels: [ ");
	for (i = 0; i < mappings->nr_mappings; i++) {
		const struct side_enum_mapping *mapping = &mappings->mappings[i];

		if (mapping->range_end < mapping->range_begin) {
			fprintf(stderr, "ERROR: Unexpected enum range: %" PRIu64 "-%" PRIu64 "\n",
				mapping->range_begin, mapping->range_end);
			abort();
		}
		if (v64.s >= mapping->range_begin && v64.s <= mapping->range_end) {
			printf("%s", print_count++ ? ", " : "");
			tracer_print_string(mapping->label.p, mapping->label.unit_size, mapping->label.byte_order, NULL);
		}
	}
	if (!print_count)
		printf("<NO LABEL>");
	printf(" ]");
}

static
void tracer_print_enum(const struct side_type *type_desc, const struct side_arg *item)
{
	const struct side_enum_mappings *mappings = type_desc->u.side_enum.mappings;
	const struct side_type *elem_type = type_desc->u.side_enum.elem_type;
	union int64_value v64;

	if (elem_type->type != item->type) {
		fprintf(stderr, "ERROR: Unexpected enum element type\n");
		abort();
	}
	v64 = tracer_load_integer_value(&elem_type->u.side_integer,
			&item->u.side_static.integer_value, 0, NULL);
	print_attributes("attr", ":", mappings->attr, mappings->nr_attr);
	printf("%s", mappings->nr_attr ? ", " : "");
	tracer_print_type(elem_type, item);
	print_enum_labels(mappings, v64);
}

static
uint32_t elem_type_to_stride(const struct side_type *elem_type)
{
	uint32_t stride_bit;

	switch (elem_type->type) {
	case SIDE_TYPE_BYTE:
		stride_bit = 8;
		break;

	case SIDE_TYPE_U8:
	case SIDE_TYPE_U16:
	case SIDE_TYPE_U32:
	case SIDE_TYPE_U64:
	case SIDE_TYPE_S8:
	case SIDE_TYPE_S16:
	case SIDE_TYPE_S32:
	case SIDE_TYPE_S64:
		return elem_type->u.side_integer.integer_size * CHAR_BIT;
	default:
		fprintf(stderr, "ERROR: Unexpected enum bitmap element type\n");
		abort();
	}
	return stride_bit;
}

static
void tracer_print_enum_bitmap(const struct side_type *type_desc,
		const struct side_arg *item)
{
	const struct side_enum_bitmap_mappings *side_enum_mappings = type_desc->u.side_enum_bitmap.mappings;
	const struct side_type *enum_elem_type = type_desc->u.side_enum_bitmap.elem_type, *elem_type;
	uint32_t i, print_count = 0, stride_bit, nr_items;
	const struct side_arg *array_item;

	switch (enum_elem_type->type) {
	case SIDE_TYPE_U8:		/* Fall-through */
	case SIDE_TYPE_BYTE:		/* Fall-through */
	case SIDE_TYPE_U16:		/* Fall-through */
	case SIDE_TYPE_U32:		/* Fall-through */
	case SIDE_TYPE_U64:		/* Fall-through */
	case SIDE_TYPE_S8:		/* Fall-through */
	case SIDE_TYPE_S16:		/* Fall-through */
	case SIDE_TYPE_S32:		/* Fall-through */
	case SIDE_TYPE_S64:
		elem_type = enum_elem_type;
		array_item = item;
		nr_items = 1;
		break;
	case SIDE_TYPE_ARRAY:
		elem_type = enum_elem_type->u.side_array.elem_type;
		array_item = item->u.side_static.side_array->sav;
		nr_items = type_desc->u.side_array.length;
		break;
	case SIDE_TYPE_VLA:
		elem_type = enum_elem_type->u.side_vla.elem_type;
		array_item = item->u.side_static.side_vla->sav;
		nr_items = item->u.side_static.side_vla->len;
		break;
	default:
		fprintf(stderr, "ERROR: Unexpected enum element type\n");
		abort();
	}
	stride_bit = elem_type_to_stride(elem_type);

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
			if (elem_type->type == SIDE_TYPE_BYTE) {
				uint8_t v = array_item[bit / 8].u.side_static.byte_value;
				if (v & (1ULL << (bit % 8))) {
					match = true;
					goto match;
				}
			} else {
				union int64_value v64;

				v64 = tracer_load_integer_value(&elem_type->u.side_integer,
						&array_item[bit / stride_bit].u.side_static.integer_value,
						0, NULL);
				if (v64.u & (1ULL << (bit % stride_bit))) {
					match = true;
					goto match;
				}
			}
		}
match:
		if (match) {
			printf("%s", print_count++ ? ", " : "");
			tracer_print_string(mapping->label.p, mapping->label.unit_size, mapping->label.byte_order, NULL);
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
	union int64_value v64;
	uint16_t len_bits;

	v64 = tracer_load_integer_value(type_integer, value, offset_bits, &len_bits);
	tracer_print_type_header(separator, type_integer->attr, type_integer->nr_attr);
	base = get_attr_display_base(type_integer->attr, type_integer->nr_attr, default_base);
	switch (base) {
	case TRACER_DISPLAY_BASE_2:
		print_integer_binary(v64.u, len_bits);
		break;
	case TRACER_DISPLAY_BASE_8:
		/* Clear sign bits beyond len_bits */
		if (len_bits < 64)
			v64.u &= (1ULL << len_bits) - 1;
		printf("0%" PRIo64, v64.u);
		break;
	case TRACER_DISPLAY_BASE_10:
		if (type_integer->signedness)
			printf("%" PRId64, v64.s);
		else
			printf("%" PRIu64, v64.u);
		break;
	case TRACER_DISPLAY_BASE_16:
		/* Clear sign bits beyond len_bits */
		if (len_bits < 64)
			v64.u &= (1ULL << len_bits) - 1;
		printf("0x%" PRIx64, v64.u);
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

	tracer_print_type_header(separator, type_float->attr, type_float->nr_attr);
	reverse_bo = type_float->byte_order != SIDE_TYPE_FLOAT_WORD_ORDER_HOST;
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

	case SIDE_TYPE_GATHER_ENUM:
		switch (item->type) {
		case SIDE_TYPE_GATHER_INTEGER:
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
		case SIDE_TYPE_DYNAMIC_INTEGER:
		case SIDE_TYPE_DYNAMIC_BYTE:
		case SIDE_TYPE_DYNAMIC_POINTER:
		case SIDE_TYPE_DYNAMIC_FLOAT:
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

	if (type_desc->type == SIDE_TYPE_ENUM || type_desc->type == SIDE_TYPE_ENUM_BITMAP ||
			type_desc->type == SIDE_TYPE_GATHER_ENUM)
		type = (enum side_type_label) type_desc->type;
	else
		type = (enum side_type_label) item->type;

	printf("{ ");
	switch (type) {
		/* Stack-copy basic types */
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

	case SIDE_TYPE_BYTE:
		tracer_print_type_header(":", type_desc->u.side_byte.attr, type_desc->u.side_byte.nr_attr);
		printf("0x%" PRIx8, item->u.side_static.byte_value);
		break;

	case SIDE_TYPE_POINTER:
		tracer_print_type_integer(":", &type_desc->u.side_integer, &item->u.side_static.integer_value, 0,
				TRACER_DISPLAY_BASE_16);
		break;

	case SIDE_TYPE_FLOAT_BINARY16:
	case SIDE_TYPE_FLOAT_BINARY32:
	case SIDE_TYPE_FLOAT_BINARY64:
	case SIDE_TYPE_FLOAT_BINARY128:
		tracer_print_type_float(":", &type_desc->u.side_float, &item->u.side_static.float_value);
		break;

	case SIDE_TYPE_STRING_UTF8:
	case SIDE_TYPE_STRING_UTF16:
	case SIDE_TYPE_STRING_UTF32:
		tracer_print_type_header(":", type_desc->u.side_string.attr, type_desc->u.side_string.nr_attr);
		tracer_print_string((const void *)(uintptr_t) item->u.side_static.string_value,
				type_desc->u.side_string.unit_size, type_desc->u.side_string.byte_order, NULL);
		break;

		/* Stack-copy compound types */
	case SIDE_TYPE_STRUCT:
		tracer_print_struct(type_desc, item->u.side_static.side_struct);
		break;
	case SIDE_TYPE_VARIANT:
		tracer_print_variant(type_desc, item->u.side_static.side_variant);
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

		/* Stack-copy enumeration types */
	case SIDE_TYPE_ENUM:
		tracer_print_enum(type_desc, item);
		break;
	case SIDE_TYPE_ENUM_BITMAP:
		tracer_print_enum_bitmap(type_desc, item);
		break;

		/* Gather basic types */
	case SIDE_TYPE_GATHER_BOOL:
		(void) tracer_print_gather_bool_type(&type_desc->u.side_gather, item->u.side_static.side_bool_gather_ptr);
		break;
	case SIDE_TYPE_GATHER_INTEGER:
		(void) tracer_print_gather_integer_type(&type_desc->u.side_gather, item->u.side_static.side_integer_gather_ptr,
					TRACER_DISPLAY_BASE_10);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		(void) tracer_print_gather_byte_type(&type_desc->u.side_gather, item->u.side_static.side_byte_gather_ptr);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		(void) tracer_print_gather_integer_type(&type_desc->u.side_gather, item->u.side_static.side_integer_gather_ptr,
					TRACER_DISPLAY_BASE_16);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		(void) tracer_print_gather_float_type(&type_desc->u.side_gather, item->u.side_static.side_float_gather_ptr);
		break;
	case SIDE_TYPE_GATHER_STRING:
		(void) tracer_print_gather_string_type(&type_desc->u.side_gather, item->u.side_static.side_string_gather_ptr);
		break;

		/* Gather compound type */
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

		/* Gather enumeration types */
	case SIDE_TYPE_GATHER_ENUM:
		(void) tracer_print_gather_enum_type(&type_desc->u.side_gather, item->u.side_static.side_integer_gather_ptr);
		break;

	/* Dynamic basic types */
	case SIDE_TYPE_DYNAMIC_NULL:
	case SIDE_TYPE_DYNAMIC_BOOL:
	case SIDE_TYPE_DYNAMIC_INTEGER:
	case SIDE_TYPE_DYNAMIC_BYTE:
	case SIDE_TYPE_DYNAMIC_POINTER:
	case SIDE_TYPE_DYNAMIC_FLOAT:
	case SIDE_TYPE_DYNAMIC_STRING:

	/* Dynamic compound types */
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
void tracer_print_variant(const struct side_type *type_desc, const struct side_arg_variant *side_arg_variant)
{
	const struct side_type_variant *side_type_variant = type_desc->u.side_variant;
	const struct side_type *selector_type = &side_type_variant->selector;
	union int64_value v64;
	uint32_t i;

	if (selector_type->type != side_arg_variant->selector.type) {
		fprintf(stderr, "ERROR: Unexpected variant selector type\n");
		abort();
	}
	switch (selector_type->type) {
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
		fprintf(stderr, "ERROR: Expecting integer variant selector type\n");
		abort();
	}
	v64 = tracer_load_integer_value(&selector_type->u.side_integer,
			&side_arg_variant->selector.u.side_static.integer_value, 0, NULL);
	for (i = 0; i < side_type_variant->nr_options; i++) {
		const struct side_variant_option *option = &side_type_variant->options[i];

		if (v64.s >= option->range_begin && v64.s <= option->range_end) {
			tracer_print_type(&option->side_type, &side_arg_variant->option);
			return;
		}
	}
	fprintf(stderr, "ERROR: Variant selector value unknown %" PRId64 "\n", v64.s);
	abort();
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
union int64_value tracer_load_gather_integer_value(const struct side_type_gather_integer *side_integer,
		const void *_ptr)
{
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) side_integer->access_mode;
	uint32_t integer_size_bytes = side_integer->type.integer_size;
	const char *ptr = (const char *) _ptr;
	union side_integer_value value;

	ptr = tracer_gather_access(access_mode, ptr + side_integer->offset);
	memcpy(&value, ptr, integer_size_bytes);
	return tracer_load_integer_value(&side_integer->type, &value,
			side_integer->offset_bits, NULL);
}

static
uint32_t tracer_print_gather_bool_type(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) type_gather->u.side_bool.access_mode;
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
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) type_gather->u.side_byte.access_mode;
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
uint32_t tracer_print_gather_integer_type(const struct side_type_gather *type_gather, const void *_ptr,
		enum tracer_display_base default_base)
{
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) type_gather->u.side_integer.access_mode;
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
			type_gather->u.side_integer.offset_bits, default_base);
	return tracer_gather_size(access_mode, integer_size_bytes);
}

static
uint32_t tracer_print_gather_float_type(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) type_gather->u.side_float.access_mode;
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
uint32_t tracer_print_gather_string_type(const struct side_type_gather *type_gather, const void *_ptr)
{
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) type_gather->u.side_string.access_mode;
	const char *ptr = (const char *) _ptr;
	size_t string_len;

	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_string.offset);
	tracer_print_type_header(":", type_gather->u.side_string.type.attr,
			type_gather->u.side_string.type.nr_attr);
	if (ptr) {
		tracer_print_string(ptr, type_gather->u.side_string.type.unit_size,
				type_gather->u.side_string.type.byte_order, &string_len);
	} else {
		printf("<NULL>");
		string_len = type_gather->u.side_string.type.unit_size;
	}
	return tracer_gather_size(access_mode, string_len);
}

static
uint32_t tracer_print_gather_type(const struct side_type *type_desc, const void *ptr)
{
	uint32_t len;

	printf("{ ");
	switch (type_desc->type) {
		/* Gather basic types */
	case SIDE_TYPE_GATHER_BOOL:
		len = tracer_print_gather_bool_type(&type_desc->u.side_gather, ptr);
		break;
	case SIDE_TYPE_GATHER_INTEGER:
		len = tracer_print_gather_integer_type(&type_desc->u.side_gather, ptr,
				TRACER_DISPLAY_BASE_10);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		len = tracer_print_gather_byte_type(&type_desc->u.side_gather, ptr);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		len = tracer_print_gather_integer_type(&type_desc->u.side_gather, ptr,
				TRACER_DISPLAY_BASE_16);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		len = tracer_print_gather_float_type(&type_desc->u.side_gather, ptr);
		break;
	case SIDE_TYPE_GATHER_STRING:
		len = tracer_print_gather_string_type(&type_desc->u.side_gather, ptr);
		break;

		/* Gather enum types */
	case SIDE_TYPE_GATHER_ENUM:
		len = tracer_print_gather_enum_type(&type_desc->u.side_gather, ptr);
		break;

		/* Gather compound types */
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
uint32_t tracer_print_gather_enum_type(const struct side_type_gather *type_gather, const void *_ptr)
{
	const struct side_enum_mappings *mappings = type_gather->u.side_enum.mappings;
	const struct side_type *enum_elem_type = type_gather->u.side_enum.elem_type;
	const struct side_type_gather_integer *side_integer = &enum_elem_type->u.side_gather.u.side_integer;
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) side_integer->access_mode;
	uint32_t integer_size_bytes = side_integer->type.integer_size;
	const char *ptr = (const char *) _ptr;
	union side_integer_value value;
	union int64_value v64;

	switch (integer_size_bytes) {
	case 1:
	case 2:
	case 4:
	case 8:
		break;
	default:
		abort();
	}
	ptr = tracer_gather_access(access_mode, ptr + side_integer->offset);
	memcpy(&value, ptr, integer_size_bytes);
	v64 = tracer_load_gather_integer_value(side_integer, &value);
	print_attributes("attr", ":", mappings->attr, mappings->nr_attr);
	printf("%s", mappings->nr_attr ? ", " : "");
	tracer_print_gather_type(enum_elem_type, ptr);
	print_enum_labels(mappings, v64);
	return tracer_gather_size(access_mode, integer_size_bytes);
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
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) type_gather->u.side_struct.access_mode;
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
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) type_gather->u.side_array.access_mode;
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
	enum side_type_gather_access_mode access_mode =
		(enum side_type_gather_access_mode) type_gather->u.side_vla.access_mode;
	const char *ptr = (const char *) _ptr, *orig_ptr;
	const char *length_ptr = (const char *) _length_ptr;
	union int64_value v64;
	uint32_t i, length;

	/* Access length */
	switch (type_gather->u.side_vla.length_type->type) {
	case SIDE_TYPE_GATHER_INTEGER:
		break;
	default:
		fprintf(stderr, "<gather VLA expects integer gather length type>\n");
		abort();
	}
	v64 = tracer_load_gather_integer_value(&type_gather->u.side_vla.length_type->u.side_gather.u.side_integer,
					length_ptr);
	length = (uint32_t) v64.u;
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
		/* Dynamic basic types */
	case SIDE_TYPE_DYNAMIC_NULL:
		tracer_print_type_header("::", item->u.side_dynamic.side_null.attr, item->u.side_dynamic.side_null.nr_attr);
		printf("<NULL TYPE>");
		break;
	case SIDE_TYPE_DYNAMIC_BOOL:
		tracer_print_type_bool("::", &item->u.side_dynamic.side_bool.type, &item->u.side_dynamic.side_bool.value, 0);
		break;
	case SIDE_TYPE_DYNAMIC_INTEGER:
		tracer_print_type_integer("::", &item->u.side_dynamic.side_integer.type, &item->u.side_dynamic.side_integer.value, 0,
				TRACER_DISPLAY_BASE_10);
		break;
	case SIDE_TYPE_DYNAMIC_BYTE:
		tracer_print_type_header("::", item->u.side_dynamic.side_byte.type.attr, item->u.side_dynamic.side_byte.type.nr_attr);
		printf("0x%" PRIx8, item->u.side_dynamic.side_byte.value);
		break;
	case SIDE_TYPE_DYNAMIC_POINTER:
		tracer_print_type_integer("::", &item->u.side_dynamic.side_integer.type, &item->u.side_dynamic.side_integer.value, 0,
				TRACER_DISPLAY_BASE_16);
		break;
	case SIDE_TYPE_DYNAMIC_FLOAT:
		tracer_print_type_float("::", &item->u.side_dynamic.side_float.type,
					&item->u.side_dynamic.side_float.value);
		break;
	case SIDE_TYPE_DYNAMIC_STRING:
		tracer_print_type_header("::", item->u.side_dynamic.side_string.type.attr, item->u.side_dynamic.side_string.type.nr_attr);
		tracer_print_string((const char *)(uintptr_t) item->u.side_dynamic.side_string.value,
				item->u.side_dynamic.side_string.type.unit_size,
				item->u.side_dynamic.side_string.type.byte_order, NULL);
		break;

		/* Dynamic compound types */
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

static
void tracer_call(const struct side_event_description *desc,
		const struct side_arg_vec *side_arg_vec,
		void *priv __attribute__((unused)))
{
	uint32_t nr_fields = 0;

	tracer_print_static_fields(desc, side_arg_vec, &nr_fields);
	printf("\n");
}

static
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

static
void tracer_event_notification(enum side_tracer_notification notif,
		struct side_event_description **events, uint32_t nr_events,
		void *priv __attribute__((unused)))
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
