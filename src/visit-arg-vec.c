// SPDX-License-Identifier: MIT
/*
 * Copyright 2022-2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <string.h>

#include "visit-arg-vec.h"

union int_value {
	uint64_t u[NR_SIDE_INTEGER128_SPLIT];
	int64_t s[NR_SIDE_INTEGER128_SPLIT];
};

enum context_type {
	CONTEXT_NAMESPACE,
	CONTEXT_FIELD,
	CONTEXT_ARRAY,
	CONTEXT_STRUCT,
	CONTEXT_OPTIONAL,
};

struct visit_context {
	const struct visit_context *parent;
	union {
		struct {
			const char *provider_name;
			const char *event_name;
		} namespace;
		const char *field_name;
		size_t array_index;
	};
	enum context_type type;
};

static
void visit_dynamic_type(const struct side_type_visitor *type_visitor, const struct side_arg *dynamic_item, void *priv);

static
void visit_dynamic_elem(const struct side_type_visitor *type_visitor, const struct side_arg *dynamic_item, void *priv);

static
uint32_t visit_gather_type(const struct side_type_visitor *type_visitor, const struct side_type *type_desc, const void *ptr, void *priv);

static
uint32_t visit_gather_elem(const struct side_type_visitor *type_visitor, const struct side_type *type_desc, const void *ptr, void *priv);

static
void side_visit_type(const struct side_type_visitor *type_visitor, const struct visit_context *ctx, const struct side_type *type_desc, const struct side_arg *item, void *priv);

static
void side_visit_elem(const struct side_type_visitor *type_visitor, const struct visit_context *ctx, const struct side_type *type_desc, const struct side_arg *item, void *priv);

static
uint32_t type_visitor_gather_enum(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv);

static
uint32_t type_visitor_gather_struct(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv);

static
uint32_t type_visitor_gather_array(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv);

static
uint32_t type_visitor_gather_vla(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr,
		const void *_length_ptr, void *priv);

static
union int_value tracer_load_integer_value(const struct side_type_integer *type_integer,
		const union side_integer_value *value,
		uint16_t offset_bits, uint16_t *_len_bits)
{
	union int_value v = {};
	uint16_t len_bits;
	bool reverse_bo;

	if (!type_integer->len_bits)
		len_bits = type_integer->integer_size * CHAR_BIT;
	else
		len_bits = type_integer->len_bits;
	if (len_bits + offset_bits > type_integer->integer_size * CHAR_BIT)
		abort();
	reverse_bo = side_enum_get(type_integer->byte_order) != SIDE_TYPE_BYTE_ORDER_HOST;
	switch (type_integer->integer_size) {
	case 1:
		if (type_integer->signedness)
			v.s[SIDE_INTEGER128_SPLIT_LOW] = value->side_s8;
		else
			v.u[SIDE_INTEGER128_SPLIT_LOW] = value->side_u8;
		break;
	case 2:
		if (type_integer->signedness) {
			int16_t side_s16;

			side_s16 = value->side_s16;
			if (reverse_bo)
				side_s16 = side_bswap_16(side_s16);
			v.s[SIDE_INTEGER128_SPLIT_LOW] = side_s16;
		} else {
			uint16_t side_u16;

			side_u16 = value->side_u16;
			if (reverse_bo)
				side_u16 = side_bswap_16(side_u16);
			v.u[SIDE_INTEGER128_SPLIT_LOW] = side_u16;
		}
		break;
	case 4:
		if (type_integer->signedness) {
			int32_t side_s32;

			side_s32 = value->side_s32;
			if (reverse_bo)
				side_s32 = side_bswap_32(side_s32);
			v.s[SIDE_INTEGER128_SPLIT_LOW] = side_s32;
		} else {
			uint32_t side_u32;

			side_u32 = value->side_u32;
			if (reverse_bo)
				side_u32 = side_bswap_32(side_u32);
			v.u[SIDE_INTEGER128_SPLIT_LOW] = side_u32;
		}
		break;
	case 8:
		if (type_integer->signedness) {
			int64_t side_s64;

			side_s64 = value->side_s64;
			if (reverse_bo)
				side_s64 = side_bswap_64(side_s64);
			v.s[SIDE_INTEGER128_SPLIT_LOW] = side_s64;
		} else {
			uint64_t side_u64;

			side_u64 = value->side_u64;
			if (reverse_bo)
				side_u64 = side_bswap_64(side_u64);
			v.u[SIDE_INTEGER128_SPLIT_LOW] = side_u64;
		}
		break;
	case 16:
		if (type_integer->signedness) {
			int64_t side_s64[NR_SIDE_INTEGER128_SPLIT];

			side_s64[SIDE_INTEGER128_SPLIT_LOW] = value->side_s128_split[SIDE_INTEGER128_SPLIT_LOW];
			side_s64[SIDE_INTEGER128_SPLIT_HIGH] = value->side_s128_split[SIDE_INTEGER128_SPLIT_HIGH];
			if (reverse_bo) {
				side_s64[SIDE_INTEGER128_SPLIT_LOW] = side_bswap_64(side_s64[SIDE_INTEGER128_SPLIT_LOW]);
				side_s64[SIDE_INTEGER128_SPLIT_HIGH] = side_bswap_64(side_s64[SIDE_INTEGER128_SPLIT_HIGH]);
				v.s[SIDE_INTEGER128_SPLIT_LOW] = side_s64[SIDE_INTEGER128_SPLIT_HIGH];
				v.s[SIDE_INTEGER128_SPLIT_HIGH] = side_s64[SIDE_INTEGER128_SPLIT_LOW];
			} else {
				v.s[SIDE_INTEGER128_SPLIT_LOW] = side_s64[SIDE_INTEGER128_SPLIT_LOW];
				v.s[SIDE_INTEGER128_SPLIT_HIGH] = side_s64[SIDE_INTEGER128_SPLIT_HIGH];
			}
		} else {
			uint64_t side_u64[NR_SIDE_INTEGER128_SPLIT];

			side_u64[SIDE_INTEGER128_SPLIT_LOW] = value->side_u128_split[SIDE_INTEGER128_SPLIT_LOW];
			side_u64[SIDE_INTEGER128_SPLIT_HIGH] = value->side_u128_split[SIDE_INTEGER128_SPLIT_HIGH];
			if (reverse_bo) {
				side_u64[SIDE_INTEGER128_SPLIT_LOW] = side_bswap_64(side_u64[SIDE_INTEGER128_SPLIT_LOW]);
				side_u64[SIDE_INTEGER128_SPLIT_HIGH] = side_bswap_64(side_u64[SIDE_INTEGER128_SPLIT_HIGH]);
				v.u[SIDE_INTEGER128_SPLIT_LOW] = side_u64[SIDE_INTEGER128_SPLIT_HIGH];
				v.u[SIDE_INTEGER128_SPLIT_HIGH] = side_u64[SIDE_INTEGER128_SPLIT_LOW];
			} else {
				v.u[SIDE_INTEGER128_SPLIT_LOW] = side_u64[SIDE_INTEGER128_SPLIT_LOW];
				v.u[SIDE_INTEGER128_SPLIT_HIGH] = side_u64[SIDE_INTEGER128_SPLIT_HIGH];
			}
		}
		break;
	default:
		abort();
	}
	if (type_integer->integer_size <= 8) {
		v.u[SIDE_INTEGER128_SPLIT_LOW] >>= offset_bits;
		if (len_bits < 64) {
			v.u[SIDE_INTEGER128_SPLIT_LOW] &= (1ULL << len_bits) - 1;
			if (type_integer->signedness) {
				/* Sign-extend. */
				if (v.u[SIDE_INTEGER128_SPLIT_LOW] & (1ULL << (len_bits - 1))) {
					v.u[SIDE_INTEGER128_SPLIT_LOW] |= ~((1ULL << len_bits) - 1);
					v.u[SIDE_INTEGER128_SPLIT_HIGH] = ~0ULL;
				}
			}
		}
	} else {
		//TODO: Implement 128-bit integer with len_bits != 128 or nonzero offset_bits
		if (len_bits < 128 || offset_bits != 0)
			abort();
	}
	if (_len_bits)
		*_len_bits = len_bits;
	return v;
}

static
void side_check_value_u64(union int_value v)
{
	if (v.u[SIDE_INTEGER128_SPLIT_HIGH]) {
		fprintf(stderr, "Unexpected integer value\n");
		abort();
	}
}

/*
 * return the size of the input string including the null terminator, in
 * bytes.
 */
static
size_t type_visitor_strlen(const void *p, uint8_t unit_size)
{
	size_t inbytesleft = 0;

	switch (unit_size) {
	case 1:
	{
		const char *str = p;

		return strlen(str) + 1;
	}
	case 2:
	{
		const uint16_t *p16 = p;

		for (; *p16; p16++)
			inbytesleft += 2;
		return inbytesleft + 2;		/* Include 2-byte null terminator. */
	}
	case 4:
	{
		const uint32_t *p32 = p;

		for (; *p32; p32++)
			inbytesleft += 4;
		return inbytesleft + 4;		/* Include 4-byte null terminator. */
	}
	default:
		fprintf(stderr, "Unknown string unit size %" PRIu8 "\n", unit_size);
		abort();
	}
}

static
void side_visit_elem(const struct side_type_visitor *type_visitor, const struct visit_context *ctx,
		const struct side_type *type_desc, const struct side_arg *item, void *priv)
{
	if (type_visitor->before_elem_func)
		type_visitor->before_elem_func(type_desc, priv);
	side_visit_type(type_visitor, ctx, type_desc, item, priv);
	if (type_visitor->after_elem_func)
		type_visitor->after_elem_func(type_desc, priv);
}

static
void side_visit_field(const struct side_type_visitor *type_visitor, const struct visit_context *ctx,
		const struct side_event_field *item_desc, const struct side_arg *item, void *priv)
{
	struct visit_context new_ctx = {
		.type = CONTEXT_FIELD,
		.field_name = side_ptr_get(item_desc->field_name),
		.parent = ctx,
	};
	if (type_visitor->before_field_func)
		type_visitor->before_field_func(item_desc, priv);
	side_visit_type(type_visitor, &new_ctx, &item_desc->side_type, item, priv);
	if (type_visitor->after_field_func)
		type_visitor->after_field_func(item_desc, priv);
}

static
void type_visitor_struct(const struct side_type_visitor *type_visitor, const struct visit_context *ctx,
			const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec, void *priv)
{
	const struct side_arg *sav = side_ptr_get(side_arg_vec->sav);
	const struct side_type_struct *side_struct = side_ptr_get(type_desc->u.side_struct);
	uint32_t i, side_sav_len = side_arg_vec->len;

	if (side_struct->nr_fields != side_sav_len) {
		fprintf(stderr, "ERROR: number of fields mismatch between description and arguments of structure\n");
		abort();
	}
	if (type_visitor->before_struct_type_func)
		type_visitor->before_struct_type_func(side_struct, side_arg_vec, priv);

	for (i = 0; i < side_sav_len; i++) {
		struct visit_context new_ctx = {
			.type = CONTEXT_STRUCT,
			.parent = ctx
		};
		side_visit_field(type_visitor, &new_ctx, &side_ptr_get(side_struct->fields)[i], &sav[i], priv);
	}
	if (type_visitor->after_struct_type_func)
		type_visitor->after_struct_type_func(side_struct, side_arg_vec, priv);
}

static
void type_visitor_variant(const struct side_type_visitor *type_visitor, const struct visit_context *ctx,
			const struct side_type *type_desc, const struct side_arg_variant *side_arg_variant, void *priv)
{
	const struct side_type_variant *side_type_variant = side_ptr_get(type_desc->u.side_variant);
	const struct side_type *selector_type = &side_type_variant->selector;
	union int_value v;
	uint32_t i;

	if (side_enum_get(selector_type->type) != side_enum_get(side_arg_variant->selector.type)) {
		fprintf(stderr, "ERROR: Unexpected variant selector type\n");
		abort();
	}
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
	v = tracer_load_integer_value(&selector_type->u.side_integer,
			&side_arg_variant->selector.u.side_static.integer_value, 0, NULL);
	side_check_value_u64(v);
	for (i = 0; i < side_type_variant->nr_options; i++) {
		const struct side_variant_option *option = &side_ptr_get(side_type_variant->options)[i];

		if (v.s[SIDE_INTEGER128_SPLIT_LOW] >= option->range_begin && v.s[SIDE_INTEGER128_SPLIT_LOW] <= option->range_end) {
			side_visit_type(type_visitor, ctx, &option->side_type, &side_arg_variant->option, priv);
			return;
		}
	}
	fprintf(stderr, "ERROR: Variant selector value unknown %" PRId64 "\n", v.s[SIDE_INTEGER128_SPLIT_LOW]);
	abort();
}

static
void type_visitor_optional(const struct side_type_visitor *type_visitor, const struct visit_context *ctx,
			const struct side_type *type_desc, const struct side_arg_optional *side_arg_optional, void *priv)
{
	const struct side_type *type;
	const struct side_arg *arg;
	struct visit_context new_ctx = {
		.type = CONTEXT_OPTIONAL,
		.parent = ctx,
	};

	if (side_arg_optional->selector == SIDE_OPTIONAL_DISABLED)
		return;

	type = side_ptr_get(side_ptr_get(type_desc->u.side_optional)->elem_type);
	arg = &side_arg_optional->side_static;

	side_visit_type(type_visitor, &new_ctx, type, arg, priv);
}

static
void type_visitor_array(const struct side_type_visitor *type_visitor, const struct visit_context *ctx,
			const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec, void *priv)
{
	const struct side_arg *sav = side_ptr_get(side_arg_vec->sav);
	uint32_t i, side_sav_len = side_arg_vec->len;

	if (side_ptr_get(type_desc->u.side_array)->length != side_sav_len) {
		fprintf(stderr, "ERROR: length mismatch between description and arguments of array\n");
		abort();
	}
	if (type_visitor->before_array_type_func)
		type_visitor->before_array_type_func(side_ptr_get(type_desc->u.side_array), side_arg_vec, priv);
	for (i = 0; i < side_sav_len; i++) {
		struct visit_context new_ctx = {
			.type = CONTEXT_ARRAY,
			.array_index = i,
			.parent = ctx
		};
		side_visit_elem(type_visitor, &new_ctx, side_ptr_get(side_ptr_get(type_desc->u.side_array)->elem_type), &sav[i], priv);
	}
	if (type_visitor->after_array_type_func)
		type_visitor->after_array_type_func(side_ptr_get(type_desc->u.side_array), side_arg_vec, priv);
}

static
void type_visitor_vla(const struct side_type_visitor *type_visitor, const struct visit_context *ctx,
		const struct side_type *type_desc, const struct side_arg_vec *side_arg_vec, void *priv)
{
	const struct side_arg *sav = side_ptr_get(side_arg_vec->sav);
	uint32_t i, side_sav_len = side_arg_vec->len;

	if (type_visitor->before_vla_type_func)
		type_visitor->before_vla_type_func(side_ptr_get(type_desc->u.side_vla), side_arg_vec, priv);
	for (i = 0; i < side_sav_len; i++) {
		struct visit_context new_ctx = {
			.type = CONTEXT_ARRAY,
			.array_index = i,
			.parent = ctx
		};
		side_visit_elem(type_visitor, &new_ctx, side_ptr_get(side_ptr_get(type_desc->u.side_vla)->elem_type), &sav[i], priv);
	}
	if (type_visitor->after_vla_type_func)
		type_visitor->after_vla_type_func(side_ptr_get(type_desc->u.side_vla), side_arg_vec, priv);
}

struct tracer_visitor_priv {
	const struct side_type_visitor *type_visitor;
	const struct visit_context *ctx;
	void *priv;
	const struct side_type *elem_type;
	int i;
};

static
enum side_visitor_status tracer_write_elem_cb(const struct side_tracer_visitor_ctx *tracer_ctx,
					const struct side_arg *elem)
{
	struct tracer_visitor_priv *tracer_priv = (struct tracer_visitor_priv *) tracer_ctx->priv;

	side_visit_elem(tracer_priv->type_visitor, tracer_priv->ctx, tracer_priv->elem_type, elem, tracer_priv->priv);
	return SIDE_VISITOR_STATUS_OK;
}

static
void type_visitor_vla_visitor(const struct side_type_visitor *type_visitor, const struct visit_context *ctx,
			const struct side_type *type_desc, struct side_arg_vla_visitor *vla_visitor, void *priv)
{
	struct tracer_visitor_priv tracer_priv = {
		.type_visitor = type_visitor,
		.priv = priv,
		.elem_type = side_ptr_get(side_ptr_get(type_desc->u.side_vla_visitor)->elem_type),
		.i = 0,
		.ctx = ctx,
	};
	const struct side_tracer_visitor_ctx tracer_ctx = {
		.write_elem = tracer_write_elem_cb,
		.priv = &tracer_priv,
	};
	enum side_visitor_status status;
	side_visitor_func func;
	void *app_ctx;

	if (!vla_visitor)
		abort();
	if (type_visitor->before_vla_visitor_type_func)
		type_visitor->before_vla_visitor_type_func(side_ptr_get(type_desc->u.side_vla_visitor), vla_visitor, priv);
	app_ctx = side_ptr_get(vla_visitor->app_ctx);
	func = side_ptr_get(side_ptr_get(type_desc->u.side_vla_visitor)->visitor);
	status = func(&tracer_ctx, app_ctx);
	switch (status) {
	case SIDE_VISITOR_STATUS_OK:
		break;
	case SIDE_VISITOR_STATUS_ERROR:
		fprintf(stderr, "ERROR: Visitor error\n");
		abort();
	}
	if (type_visitor->after_vla_visitor_type_func)
		type_visitor->after_vla_visitor_type_func(side_ptr_get(type_desc->u.side_vla_visitor), vla_visitor, priv);
}

static
const char *tracer_gather_access(enum side_type_gather_access_mode access_mode, const char *ptr)
{
	switch (access_mode) {
	case SIDE_TYPE_GATHER_ACCESS_DIRECT:
		return ptr;
	case SIDE_TYPE_GATHER_ACCESS_POINTER:
		/* Dereference pointer */
		memcpy(&ptr, ptr, sizeof(const char *));
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
union int_value tracer_load_gather_integer_value(const struct side_type_gather_integer *side_integer,
		const void *_ptr)
{
	enum side_type_gather_access_mode access_mode = side_enum_get(side_integer->access_mode);
	uint32_t integer_size_bytes = side_integer->type.integer_size;
	const char *ptr = (const char *) _ptr;
	union side_integer_value value;

	ptr = tracer_gather_access(access_mode, ptr + side_integer->offset);
	memcpy(&value, ptr, integer_size_bytes);
	return tracer_load_integer_value(&side_integer->type, &value,
			side_integer->offset_bits, NULL);
}

static
void visit_gather_field(const struct side_type_visitor *type_visitor, const struct side_event_field *field, const void *ptr, void *priv)
{
	if (type_visitor->before_field_func)
		type_visitor->before_field_func(field, priv);
	(void) visit_gather_type(type_visitor, &field->side_type, ptr, priv);
	if (type_visitor->after_field_func)
		type_visitor->after_field_func(field, priv);
}

static
uint32_t type_visitor_gather_struct(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv)
{
	enum side_type_gather_access_mode access_mode = side_enum_get(type_gather->u.side_struct.access_mode);
	const struct side_type_struct *side_struct = side_ptr_get(type_gather->u.side_struct.type);
	const char *ptr = (const char *) _ptr;
	uint32_t i;

	if (type_visitor->before_gather_struct_type_func)
		type_visitor->before_gather_struct_type_func(side_struct, priv);
	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_struct.offset);
	for (i = 0; i < side_struct->nr_fields; i++)
		visit_gather_field(type_visitor, &side_ptr_get(side_struct->fields)[i], ptr, priv);
	if (type_visitor->after_gather_struct_type_func)
		type_visitor->after_gather_struct_type_func(side_struct, priv);
	return tracer_gather_size(access_mode, type_gather->u.side_struct.size);
}

static
uint32_t type_visitor_gather_array(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv)
{
	enum side_type_gather_access_mode access_mode = side_enum_get(type_gather->u.side_array.access_mode);
	const struct side_type_array *side_array = &type_gather->u.side_array.type;
	const char *ptr = (const char *) _ptr, *orig_ptr;
	uint32_t i;

	if (type_visitor->before_gather_array_type_func)
		type_visitor->before_gather_array_type_func(side_array, priv);
	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_array.offset);
	orig_ptr = ptr;
	for (i = 0; i < side_array->length; i++) {
		const struct side_type *elem_type = side_ptr_get(side_array->elem_type);

		switch (side_enum_get(elem_type->type)) {
		case SIDE_TYPE_GATHER_VLA:
			fprintf(stderr, "<gather VLA only supported within gather structures>\n");
			abort();
		default:
			break;
		}
		ptr += visit_gather_elem(type_visitor, elem_type, ptr, priv);
	}
	if (type_visitor->after_gather_array_type_func)
		type_visitor->after_gather_array_type_func(side_array, priv);
	return tracer_gather_size(access_mode, ptr - orig_ptr);
}

static
uint32_t type_visitor_gather_vla(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, const void *_length_ptr, void *priv)
{
	enum side_type_gather_access_mode access_mode = side_enum_get(type_gather->u.side_vla.access_mode);
	const struct side_type_vla *side_vla = &type_gather->u.side_vla.type;
	const struct side_type *length_type = side_ptr_get(type_gather->u.side_vla.type.length_type);
	const char *ptr = (const char *) _ptr, *orig_ptr;
	const char *length_ptr = (const char *) _length_ptr;
	union int_value v = {};
	uint32_t i, length;

	/* Access length */
	switch (side_enum_get(length_type->type)) {
	case SIDE_TYPE_GATHER_INTEGER:
		break;
	default:
		fprintf(stderr, "<gather VLA expects integer gather length type>\n");
		abort();
	}
	v = tracer_load_gather_integer_value(&length_type->u.side_gather.u.side_integer,
					length_ptr);
	if (v.u[SIDE_INTEGER128_SPLIT_HIGH] || v.u[SIDE_INTEGER128_SPLIT_LOW] > UINT32_MAX) {
		fprintf(stderr, "Unexpected vla length value\n");
		abort();
	}
	length = (uint32_t) v.u[SIDE_INTEGER128_SPLIT_LOW];
	if (type_visitor->before_gather_vla_type_func)
		type_visitor->before_gather_vla_type_func(side_vla, length, priv);
	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_vla.offset);
	orig_ptr = ptr;
	for (i = 0; i < length; i++) {
		const struct side_type *elem_type = side_ptr_get(side_vla->elem_type);

		switch (side_enum_get(elem_type->type)) {
		case SIDE_TYPE_GATHER_VLA:
			fprintf(stderr, "<gather VLA only supported within gather structures>\n");
			abort();
		default:
			break;
		}
		ptr += visit_gather_elem(type_visitor, elem_type, ptr, priv);
	}
	if (type_visitor->after_gather_vla_type_func)
		type_visitor->after_gather_vla_type_func(side_vla, length, priv);
	return tracer_gather_size(access_mode, ptr - orig_ptr);
}

static
uint32_t type_visitor_gather_bool(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv)
{
	enum side_type_gather_access_mode access_mode = side_enum_get(type_gather->u.side_bool.access_mode);
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
	if (type_visitor->gather_bool_type_func)
		type_visitor->gather_bool_type_func(&type_gather->u.side_bool, &value, priv);
	return tracer_gather_size(access_mode, bool_size_bytes);
}

static
uint32_t type_visitor_gather_byte(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv)
{
	enum side_type_gather_access_mode access_mode = side_enum_get(type_gather->u.side_byte.access_mode);
	const char *ptr = (const char *) _ptr;
	uint8_t value;

	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_byte.offset);
	memcpy(&value, ptr, 1);
	if (type_visitor->gather_byte_type_func)
		type_visitor->gather_byte_type_func(&type_gather->u.side_byte, &value, priv);
	return tracer_gather_size(access_mode, 1);
}

static
uint32_t type_visitor_gather_integer(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr,
		enum side_type_label integer_type, void *priv)
{
	enum side_type_gather_access_mode access_mode = side_enum_get(type_gather->u.side_integer.access_mode);
	uint32_t integer_size_bytes = type_gather->u.side_integer.type.integer_size;
	const char *ptr = (const char *) _ptr;
	union side_integer_value value;

	switch (integer_size_bytes) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
		break;
	default:
		abort();
	}
	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_integer.offset);
	memcpy(&value, ptr, integer_size_bytes);
	switch (integer_type) {
	case SIDE_TYPE_GATHER_INTEGER:
		if (type_visitor->gather_integer_type_func)
			type_visitor->gather_integer_type_func(&type_gather->u.side_integer, &value, priv);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		if (type_visitor->gather_pointer_type_func)
			type_visitor->gather_pointer_type_func(&type_gather->u.side_integer, &value, priv);
		break;
	default:
		fprintf(stderr, "Unexpected integer type\n");
		abort();
	}
	return tracer_gather_size(access_mode, integer_size_bytes);
}

static
uint32_t type_visitor_gather_float(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv)
{
	enum side_type_gather_access_mode access_mode = side_enum_get(type_gather->u.side_float.access_mode);
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
	if (type_visitor->gather_float_type_func)
		type_visitor->gather_float_type_func(&type_gather->u.side_float, &value, priv);
	return tracer_gather_size(access_mode, float_size_bytes);
}

static
uint32_t type_visitor_gather_string(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv)
{

	enum side_type_gather_access_mode access_mode = side_enum_get(type_gather->u.side_string.access_mode);
	enum side_type_label_byte_order byte_order = side_enum_get(type_gather->u.side_string.type.byte_order);
	uint8_t unit_size = type_gather->u.side_string.type.unit_size;
	const char *ptr = (const char *) _ptr;
	size_t string_len = 0;

	ptr = tracer_gather_access(access_mode, ptr + type_gather->u.side_string.offset);
	if (ptr)
		string_len = type_visitor_strlen(ptr, unit_size);
	if (type_visitor->gather_string_type_func)
		type_visitor->gather_string_type_func(&type_gather->u.side_string, ptr, unit_size,
				byte_order, string_len, priv);
	return tracer_gather_size(access_mode, string_len);
}

static
uint32_t visit_gather_type(const struct side_type_visitor *type_visitor, const struct side_type *type_desc, const void *ptr, void *priv)
{
	uint32_t len;

	switch (side_enum_get(type_desc->type)) {
		/* Gather basic types */
	case SIDE_TYPE_GATHER_BOOL:
		len = type_visitor_gather_bool(type_visitor, &type_desc->u.side_gather, ptr, priv);
		break;
	case SIDE_TYPE_GATHER_INTEGER:
		len = type_visitor_gather_integer(type_visitor, &type_desc->u.side_gather, ptr, SIDE_TYPE_GATHER_INTEGER, priv);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		len = type_visitor_gather_byte(type_visitor, &type_desc->u.side_gather, ptr, priv);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		len = type_visitor_gather_integer(type_visitor, &type_desc->u.side_gather, ptr, SIDE_TYPE_GATHER_POINTER, priv);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		len = type_visitor_gather_float(type_visitor, &type_desc->u.side_gather, ptr, priv);
		break;
	case SIDE_TYPE_GATHER_STRING:
		len = type_visitor_gather_string(type_visitor, &type_desc->u.side_gather, ptr, priv);
		break;

		/* Gather enumeration types */
	case SIDE_TYPE_GATHER_ENUM:
		len = type_visitor_gather_enum(type_visitor, &type_desc->u.side_gather, ptr, priv);
		break;

		/* Gather compound types */
	case SIDE_TYPE_GATHER_STRUCT:
		len = type_visitor_gather_struct(type_visitor, &type_desc->u.side_gather, ptr, priv);
		break;
	case SIDE_TYPE_GATHER_ARRAY:
		len = type_visitor_gather_array(type_visitor, &type_desc->u.side_gather, ptr, priv);
		break;
	case SIDE_TYPE_GATHER_VLA:
		len = type_visitor_gather_vla(type_visitor, &type_desc->u.side_gather, ptr, ptr, priv);
		break;
	default:
		fprintf(stderr, "<UNKNOWN GATHER TYPE>");
		abort();
	}
	return len;
}

static
uint32_t visit_gather_elem(const struct side_type_visitor *type_visitor, const struct side_type *type_desc, const void *ptr, void *priv)
{
	uint32_t len;

	if (type_visitor->before_elem_func)
		type_visitor->before_elem_func(type_desc, priv);
	len = visit_gather_type(type_visitor, type_desc, ptr, priv);
	if (type_visitor->after_elem_func)
		type_visitor->after_elem_func(type_desc, priv);
	return len;
}

static
uint32_t type_visitor_gather_enum(const struct side_type_visitor *type_visitor, const struct side_type_gather *type_gather, const void *_ptr, void *priv)
{
	const struct side_type *enum_elem_type = side_ptr_get(type_gather->u.side_enum.elem_type);
	const struct side_type_gather_integer *side_integer = &enum_elem_type->u.side_gather.u.side_integer;
	enum side_type_gather_access_mode access_mode = side_enum_get(side_integer->access_mode);
	uint32_t integer_size_bytes = side_integer->type.integer_size;
	const char *ptr = (const char *) _ptr;
	union side_integer_value value;

	switch (integer_size_bytes) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
		break;
	default:
		abort();
	}
	ptr = tracer_gather_access(access_mode, ptr + side_integer->offset);
	memcpy(&value, ptr, integer_size_bytes);
	if (type_visitor->gather_enum_type_func)
		type_visitor->gather_enum_type_func(&type_gather->u.side_enum, &value, priv);
	return tracer_gather_size(access_mode, integer_size_bytes);
}

static
void visit_dynamic_field(const struct side_type_visitor *type_visitor, const struct side_arg_dynamic_field *field, void *priv)
{
	if (type_visitor->before_dynamic_field_func)
		type_visitor->before_dynamic_field_func(field, priv);
	visit_dynamic_type(type_visitor, &field->elem, priv);
	if (type_visitor->after_dynamic_field_func)
		type_visitor->after_dynamic_field_func(field, priv);
}

static
void type_visitor_dynamic_struct(const struct side_type_visitor *type_visitor, const struct side_arg_dynamic_struct *dynamic_struct, void *priv)
{
	const struct side_arg_dynamic_field *fields = side_ptr_get(dynamic_struct->fields);
	uint32_t i, len = dynamic_struct->len;

	if (type_visitor->before_dynamic_struct_func)
		type_visitor->before_dynamic_struct_func(dynamic_struct, priv);
	for (i = 0; i < len; i++)
		visit_dynamic_field(type_visitor, &fields[i], priv);
	if (type_visitor->after_dynamic_struct_func)
		type_visitor->after_dynamic_struct_func(dynamic_struct, priv);
}

struct tracer_dynamic_struct_visitor_priv {
	const struct side_type_visitor *type_visitor;
	void *priv;
	int i;
};

static
enum side_visitor_status tracer_dynamic_struct_write_elem_cb(
			const struct side_tracer_dynamic_struct_visitor_ctx *tracer_ctx,
			const struct side_arg_dynamic_field *dynamic_field)
{
	struct tracer_dynamic_struct_visitor_priv *tracer_priv =
		(struct tracer_dynamic_struct_visitor_priv *) tracer_ctx->priv;

	visit_dynamic_field(tracer_priv->type_visitor, dynamic_field, tracer_priv->priv);
	return SIDE_VISITOR_STATUS_OK;
}

static
void type_visitor_dynamic_struct_visitor(const struct side_type_visitor *type_visitor, const struct side_arg *item, void *priv)
{
	struct side_arg_dynamic_struct_visitor *dynamic_struct_visitor;
	struct tracer_dynamic_struct_visitor_priv tracer_priv = {
		.type_visitor = type_visitor,
		.priv = priv,
		.i = 0,
	};
	const struct side_tracer_dynamic_struct_visitor_ctx tracer_ctx = {
		.write_field = tracer_dynamic_struct_write_elem_cb,
		.priv = &tracer_priv,
	};
	enum side_visitor_status status;
	void *app_ctx;

	if (type_visitor->before_dynamic_struct_visitor_func)
		type_visitor->before_dynamic_struct_visitor_func(item, priv);
	dynamic_struct_visitor = side_ptr_get(item->u.side_dynamic.side_dynamic_struct_visitor);
	if (!dynamic_struct_visitor)
		abort();
	app_ctx = side_ptr_get(dynamic_struct_visitor->app_ctx);
	status = side_ptr_get(dynamic_struct_visitor->visitor)(&tracer_ctx, app_ctx);
	switch (status) {
	case SIDE_VISITOR_STATUS_OK:
		break;
	case SIDE_VISITOR_STATUS_ERROR:
		fprintf(stderr, "ERROR: Visitor error\n");
		abort();
	}
	if (type_visitor->after_dynamic_struct_visitor_func)
		type_visitor->after_dynamic_struct_visitor_func(item, priv);
}

static
void type_visitor_dynamic_vla(const struct side_type_visitor *type_visitor, const struct side_arg_dynamic_vla *vla, void *priv)
{
	const struct side_arg *sav = side_ptr_get(vla->sav);
	uint32_t i, side_sav_len = vla->len;

	if (type_visitor->before_dynamic_vla_func)
		type_visitor->before_dynamic_vla_func(vla, priv);
	for (i = 0; i < side_sav_len; i++)
		visit_dynamic_elem(type_visitor, &sav[i], priv);
	if (type_visitor->after_dynamic_vla_func)
		type_visitor->after_dynamic_vla_func(vla, priv);
}

struct tracer_dynamic_vla_visitor_priv {
	const struct side_type_visitor *type_visitor;
	void *priv;
	int i;
};

static
enum side_visitor_status tracer_dynamic_vla_write_elem_cb(
			const struct side_tracer_visitor_ctx *tracer_ctx,
			const struct side_arg *elem)
{
	struct tracer_dynamic_vla_visitor_priv *tracer_priv =
		(struct tracer_dynamic_vla_visitor_priv *) tracer_ctx->priv;

	visit_dynamic_elem(tracer_priv->type_visitor, elem, tracer_priv->priv);
	return SIDE_VISITOR_STATUS_OK;
}

static
void type_visitor_dynamic_vla_visitor(const struct side_type_visitor *type_visitor, const struct side_arg *item, void *priv)
{
	struct side_arg_dynamic_vla_visitor *dynamic_vla_visitor;
	struct tracer_dynamic_vla_visitor_priv tracer_priv = {
		.type_visitor = type_visitor,
		.priv = priv,
		.i = 0,
	};
	const struct side_tracer_visitor_ctx tracer_ctx = {
		.write_elem = tracer_dynamic_vla_write_elem_cb,
		.priv = &tracer_priv,
	};
	enum side_visitor_status status;
	void *app_ctx;

	if (type_visitor->before_dynamic_vla_visitor_func)
		type_visitor->before_dynamic_vla_visitor_func(item, priv);
	dynamic_vla_visitor = side_ptr_get(item->u.side_dynamic.side_dynamic_vla_visitor);
	if (!dynamic_vla_visitor)
		abort();
	app_ctx = side_ptr_get(dynamic_vla_visitor->app_ctx);
	status = side_ptr_get(dynamic_vla_visitor->visitor)(&tracer_ctx, app_ctx);
	switch (status) {
	case SIDE_VISITOR_STATUS_OK:
		break;
	case SIDE_VISITOR_STATUS_ERROR:
		fprintf(stderr, "ERROR: Visitor error\n");
		abort();
	}
	if (type_visitor->after_dynamic_vla_visitor_func)
		type_visitor->after_dynamic_vla_visitor_func(item, priv);
}

static
void visit_dynamic_type(const struct side_type_visitor *type_visitor, const struct side_arg *dynamic_item, void *priv)
{
	switch (side_enum_get(dynamic_item->type)) {
	/* Dynamic basic types */
	case SIDE_TYPE_DYNAMIC_NULL:
		if (type_visitor->dynamic_null_func)
			type_visitor->dynamic_null_func(dynamic_item, priv);
		break;
	case SIDE_TYPE_DYNAMIC_BOOL:
		if (type_visitor->dynamic_bool_func)
			type_visitor->dynamic_bool_func(dynamic_item, priv);
		break;
	case SIDE_TYPE_DYNAMIC_INTEGER:
		if (type_visitor->dynamic_integer_func)
			type_visitor->dynamic_integer_func(dynamic_item, priv);
		break;
	case SIDE_TYPE_DYNAMIC_BYTE:
		if (type_visitor->dynamic_byte_func)
			type_visitor->dynamic_byte_func(dynamic_item, priv);
		break;
	case SIDE_TYPE_DYNAMIC_POINTER:
		if (type_visitor->dynamic_pointer_func)
			type_visitor->dynamic_pointer_func(dynamic_item, priv);
		break;
	case SIDE_TYPE_DYNAMIC_FLOAT:
		if (type_visitor->dynamic_float_func)
			type_visitor->dynamic_float_func(dynamic_item, priv);
		break;
	case SIDE_TYPE_DYNAMIC_STRING:
		if (type_visitor->dynamic_string_func)
			type_visitor->dynamic_string_func(dynamic_item, priv);
		break;

	/* Dynamic compound types */
	case SIDE_TYPE_DYNAMIC_STRUCT:
		type_visitor_dynamic_struct(type_visitor, side_ptr_get(dynamic_item->u.side_dynamic.side_dynamic_struct), priv);
		break;
	case SIDE_TYPE_DYNAMIC_STRUCT_VISITOR:
		type_visitor_dynamic_struct_visitor(type_visitor, dynamic_item, priv);
		break;
	case SIDE_TYPE_DYNAMIC_VLA:
		type_visitor_dynamic_vla(type_visitor, side_ptr_get(dynamic_item->u.side_dynamic.side_dynamic_vla), priv);
		break;
	case SIDE_TYPE_DYNAMIC_VLA_VISITOR:
		type_visitor_dynamic_vla_visitor(type_visitor, dynamic_item, priv);
		break;
	default:
		fprintf(stderr, "<UNKNOWN TYPE>\n");
		abort();
	}
}

static
void visit_dynamic_elem(const struct side_type_visitor *type_visitor, const struct side_arg *dynamic_item, void *priv)
{
	if (type_visitor->before_dynamic_elem_func)
		type_visitor->before_dynamic_elem_func(dynamic_item, priv);
	visit_dynamic_type(type_visitor, dynamic_item, priv);
	if (type_visitor->after_dynamic_elem_func)
		type_visitor->after_dynamic_elem_func(dynamic_item, priv);
}

static size_t unwind_context(const struct visit_context *ctx, size_t indent)
{
	if (CONTEXT_NAMESPACE == ctx->type) {
		fprintf(stderr,
			"%s:%s",
			ctx->namespace.provider_name,
			ctx->namespace.event_name);
		goto out;
	}

	indent = unwind_context(ctx->parent, indent);

	for (size_t k = 0; k < indent; ++k) {
		fputc('\t', stderr);
	}

	switch (ctx->type) {
	case CONTEXT_FIELD:
		fprintf(stderr, "field: \"%s\"", ctx->field_name);
		break;
	case CONTEXT_STRUCT:
		fprintf(stderr, "struct:");
		break;
	case CONTEXT_ARRAY:
		fprintf(stderr, "index: %zu", ctx->array_index);
		break;
	case CONTEXT_OPTIONAL:
		fprintf(stderr, "optional");
		break;
	case CONTEXT_NAMESPACE:	/* Fallthrough */
	default:
		abort();
		break;
	}
out:
	fputc('\n', stderr);
	return indent + 1;
}

static
const char *side_type_label_to_string(enum side_type_label label)
{
	switch (label) {
	case SIDE_TYPE_NULL: return "SIDE_TYPE_NULL";
	case SIDE_TYPE_BOOL: return "SIDE_TYPE_BOOL";
	case SIDE_TYPE_U8: return "SIDE_TYPE_U8";
	case SIDE_TYPE_U16: return "SIDE_TYPE_U16";
	case SIDE_TYPE_U32: return "SIDE_TYPE_U32";
	case SIDE_TYPE_U64: return "SIDE_TYPE_U64";
	case SIDE_TYPE_U128: return "SIDE_TYPE_U128";
	case SIDE_TYPE_S8: return "SIDE_TYPE_S8";
	case SIDE_TYPE_S16: return "SIDE_TYPE_S16";
	case SIDE_TYPE_S32: return "SIDE_TYPE_S32";
	case SIDE_TYPE_S64: return "SIDE_TYPE_S64";
	case SIDE_TYPE_S128: return "SIDE_TYPE_S128";
	case SIDE_TYPE_BYTE: return "SIDE_TYPE_BYTE";
	case SIDE_TYPE_POINTER: return "SIDE_TYPE_POINTER";
	case SIDE_TYPE_FLOAT_BINARY16: return "SIDE_TYPE_FLOAT_BINARY16";
	case SIDE_TYPE_FLOAT_BINARY32: return "SIDE_TYPE_FLOAT_BINARY32";
	case SIDE_TYPE_FLOAT_BINARY64: return "SIDE_TYPE_FLOAT_BINARY64";
	case SIDE_TYPE_FLOAT_BINARY128: return "SIDE_TYPE_FLOAT_BINARY128";
	case SIDE_TYPE_STRING_UTF8: return "SIDE_TYPE_STRING_UTF8";
	case SIDE_TYPE_STRING_UTF16: return "SIDE_TYPE_STRING_UTF16";
	case SIDE_TYPE_STRING_UTF32: return "SIDE_TYPE_STRING_UTF32";
	case SIDE_TYPE_STRUCT: return "SIDE_TYPE_STRUCT";
	case SIDE_TYPE_VARIANT: return "SIDE_TYPE_VARIANT";
	case SIDE_TYPE_OPTIONAL: return "SIDE_TYPE_OPTIONAL";
	case SIDE_TYPE_ARRAY: return "SIDE_TYPE_ARRAY";
	case SIDE_TYPE_VLA: return "SIDE_TYPE_VLA";
	case SIDE_TYPE_VLA_VISITOR: return "SIDE_TYPE_VLA_VISITOR";
	case SIDE_TYPE_ENUM: return "SIDE_TYPE_ENUM";
	case SIDE_TYPE_ENUM_BITMAP: return "SIDE_TYPE_ENUM_BITMAP";
	case SIDE_TYPE_DYNAMIC: return "SIDE_TYPE_DYNAMIC";
	case SIDE_TYPE_GATHER_BOOL: return "SIDE_TYPE_GATHER_BOOL";
	case SIDE_TYPE_GATHER_INTEGER: return "SIDE_TYPE_GATHER_INTEGER";
	case SIDE_TYPE_GATHER_BYTE: return "SIDE_TYPE_GATHER_BYTE";
	case SIDE_TYPE_GATHER_POINTER: return "SIDE_TYPE_GATHER_POINTER";
	case SIDE_TYPE_GATHER_FLOAT: return "SIDE_TYPE_GATHER_FLOAT";
	case SIDE_TYPE_GATHER_STRING: return "SIDE_TYPE_GATHER_STRING";
	case SIDE_TYPE_GATHER_STRUCT: return "SIDE_TYPE_GATHER_STRUCT";
	case SIDE_TYPE_GATHER_ARRAY: return "SIDE_TYPE_GATHER_ARRAY";
	case SIDE_TYPE_GATHER_VLA: return "SIDE_TYPE_GATHER_VLA";
	case SIDE_TYPE_GATHER_ENUM: return "SIDE_TYPE_GATHER_ENUM";
	case SIDE_TYPE_DYNAMIC_NULL: return "SIDE_TYPE_DYNAMIC_NULL";
	case SIDE_TYPE_DYNAMIC_BOOL: return "SIDE_TYPE_DYNAMIC_BOOL";
	case SIDE_TYPE_DYNAMIC_INTEGER: return "SIDE_TYPE_DYNAMIC_INTEGER";
	case SIDE_TYPE_DYNAMIC_BYTE: return "SIDE_TYPE_DYNAMIC_BYTE";
	case SIDE_TYPE_DYNAMIC_POINTER: return "SIDE_TYPE_DYNAMIC_POINTER";
	case SIDE_TYPE_DYNAMIC_FLOAT: return "SIDE_TYPE_DYNAMIC_FLOAT";
	case SIDE_TYPE_DYNAMIC_STRING: return "SIDE_TYPE_DYNAMIC_STRING";
	case SIDE_TYPE_DYNAMIC_STRUCT: return "SIDE_TYPE_DYNAMIC_STRUCT";
	case SIDE_TYPE_DYNAMIC_STRUCT_VISITOR: return "SIDE_TYPE_DYNAMIC_STRUCT_VISITOR";
	case SIDE_TYPE_DYNAMIC_VLA: return "SIDE_TYPE_DYNAMIC_VLA";
	case SIDE_TYPE_DYNAMIC_VLA_VISITOR: return "SIDE_TYPE_DYNAMIC_VLA_VISITOR";
	default:
		return "<UNKNOWN>";
	}
}

__attribute__((noreturn))
static
void type_mismatch(const struct visit_context *ctx,
		enum side_type_label expected,
		enum side_type_label got)
{
	fprintf(stderr,
		"================================================================================\n");
	fprintf(stderr, "                                 ERROR!                                 \n");
	fprintf(stderr, "Type mismatch between description and arguments\n");
	fprintf(stderr, "Expecting `%s' but got `%s' in:\n\n",
		side_type_label_to_string(expected),
		side_type_label_to_string(got));
	unwind_context(ctx, 0);
	fprintf(stderr,
		"================================================================================\n");
	abort();
}

static void ensure_types_compatible(const struct visit_context *ctx,
				const struct side_type *type_desc,
				const struct side_arg *item)
{
	enum side_type_label want, got;

	want = side_enum_get(type_desc->type);
	got = side_enum_get(item->type);

	switch (want) {
	case SIDE_TYPE_ENUM:
		switch (got) {
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
			type_mismatch(ctx, want, got);
			break;
		}
		break;

	case SIDE_TYPE_ENUM_BITMAP:
		switch (got) {
		case SIDE_TYPE_U8:
		case SIDE_TYPE_BYTE:
		case SIDE_TYPE_U16:
		case SIDE_TYPE_U32:
		case SIDE_TYPE_U64:
		case SIDE_TYPE_U128:
		case SIDE_TYPE_ARRAY:
		case SIDE_TYPE_VLA:
			break;
		default:
			type_mismatch(ctx, want, got);
			break;
		}
		break;

	case SIDE_TYPE_GATHER_ENUM:
		switch (got) {
		case SIDE_TYPE_GATHER_INTEGER:
			break;
		default:
			type_mismatch(ctx, want, got);
			break;
		}
		break;

	case SIDE_TYPE_DYNAMIC:
		switch (got) {
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
			type_mismatch(ctx,
				side_enum_get(type_desc->type),
				side_enum_get(item->type));
			break;
		}
		break;
	default:
		if (want != got) {
			type_mismatch(ctx, want, got);
		}
		break;
	}
}

void side_visit_type(const struct side_type_visitor *type_visitor,
		const struct visit_context *ctx,
		const struct side_type *type_desc, const struct side_arg *item, void *priv)
{
	enum side_type_label type;

	ensure_types_compatible(ctx, type_desc, item);

	if (side_enum_get(type_desc->type) == SIDE_TYPE_ENUM || side_enum_get(type_desc->type) == SIDE_TYPE_ENUM_BITMAP || side_enum_get(type_desc->type) == SIDE_TYPE_GATHER_ENUM)
		type = side_enum_get(type_desc->type);
	else
		type = side_enum_get(item->type);

	switch (type) {
		/* Stack-copy basic types */
	case SIDE_TYPE_NULL:
		if (type_visitor->null_type_func)
			type_visitor->null_type_func(type_desc, item, priv);
		break;
	case SIDE_TYPE_BOOL:
		if (type_visitor->bool_type_func)
			type_visitor->bool_type_func(type_desc, item, priv);
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
		if (type_visitor->integer_type_func)
			type_visitor->integer_type_func(type_desc, item, priv);
		break;
	case SIDE_TYPE_BYTE:
		if (type_visitor->byte_type_func)
			type_visitor->byte_type_func(type_desc, item, priv);
		break;
	case SIDE_TYPE_POINTER:
		if (type_visitor->pointer_type_func)
			type_visitor->pointer_type_func(type_desc, item, priv);
		break;
	case SIDE_TYPE_FLOAT_BINARY16:	/* Fallthrough */
	case SIDE_TYPE_FLOAT_BINARY32:	/* Fallthrough */
	case SIDE_TYPE_FLOAT_BINARY64:	/* Fallthrough */
	case SIDE_TYPE_FLOAT_BINARY128:
		if (type_visitor->float_type_func)
			type_visitor->float_type_func(type_desc, item, priv);
		break;
	case SIDE_TYPE_STRING_UTF8:	/* Fallthrough */
	case SIDE_TYPE_STRING_UTF16:	/* Fallthrough */
	case SIDE_TYPE_STRING_UTF32:
		if (type_visitor->string_type_func)
			type_visitor->string_type_func(type_desc, item, priv);
		break;
	case SIDE_TYPE_ENUM:
		if (type_visitor->enum_type_func)
			type_visitor->enum_type_func(type_desc, item, priv);
		break;
	case SIDE_TYPE_ENUM_BITMAP:
		if (type_visitor->enum_bitmap_type_func)
			type_visitor->enum_bitmap_type_func(type_desc, item, priv);
		break;

		/* Stack-copy compound types */
	case SIDE_TYPE_STRUCT:
		type_visitor_struct(type_visitor, ctx, type_desc, side_ptr_get(item->u.side_static.side_struct), priv);
		break;
	case SIDE_TYPE_VARIANT:
		type_visitor_variant(type_visitor, ctx, type_desc, side_ptr_get(item->u.side_static.side_variant), priv);
		break;
	case SIDE_TYPE_ARRAY:
		type_visitor_array(type_visitor, ctx, type_desc, side_ptr_get(item->u.side_static.side_array), priv);
		break;
	case SIDE_TYPE_VLA:
		type_visitor_vla(type_visitor, ctx, type_desc, side_ptr_get(item->u.side_static.side_vla), priv);
		break;
	case SIDE_TYPE_VLA_VISITOR:
		type_visitor_vla_visitor(type_visitor, ctx, type_desc, side_ptr_get(item->u.side_static.side_vla_visitor), priv);
		break;

		/* Gather basic types */
	case SIDE_TYPE_GATHER_BOOL:
		(void) type_visitor_gather_bool(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_bool_gather_ptr), priv);
		break;
	case SIDE_TYPE_GATHER_INTEGER:
		(void) type_visitor_gather_integer(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_integer_gather_ptr), SIDE_TYPE_GATHER_INTEGER, priv);
		break;
	case SIDE_TYPE_GATHER_BYTE:
		(void) type_visitor_gather_byte(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_byte_gather_ptr), priv);
		break;
	case SIDE_TYPE_GATHER_POINTER:
		(void) type_visitor_gather_integer(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_integer_gather_ptr), SIDE_TYPE_GATHER_POINTER, priv);
		break;
	case SIDE_TYPE_GATHER_FLOAT:
		(void) type_visitor_gather_float(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_float_gather_ptr), priv);
		break;
	case SIDE_TYPE_GATHER_STRING:
		(void) type_visitor_gather_string(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_string_gather_ptr), priv);
		break;

		/* Gather compound type */
	case SIDE_TYPE_GATHER_STRUCT:
		(void) type_visitor_gather_struct(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_struct_gather_ptr), priv);
		break;
	case SIDE_TYPE_GATHER_ARRAY:
		(void) type_visitor_gather_array(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_array_gather_ptr), priv);
		break;
	case SIDE_TYPE_GATHER_VLA:
		(void) type_visitor_gather_vla(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_array_gather_ptr),
				side_ptr_get(item->u.side_static.side_vla_gather.length_ptr), priv);
		break;

		/* Gather enumeration types */
	case SIDE_TYPE_GATHER_ENUM:
		(void) type_visitor_gather_enum(type_visitor, &type_desc->u.side_gather, side_ptr_get(item->u.side_static.side_integer_gather_ptr), priv);
		break;

	/* Dynamic basic types */
	case SIDE_TYPE_DYNAMIC_NULL:		/* Fallthrough */
	case SIDE_TYPE_DYNAMIC_BOOL:		/* Fallthrough */
	case SIDE_TYPE_DYNAMIC_INTEGER:		/* Fallthrough */
	case SIDE_TYPE_DYNAMIC_BYTE:		/* Fallthrough */
	case SIDE_TYPE_DYNAMIC_POINTER:		/* Fallthrough */
	case SIDE_TYPE_DYNAMIC_FLOAT:		/* Fallthrough */
	case SIDE_TYPE_DYNAMIC_STRING:		/* Fallthrough */

	/* Dynamic compound types */
	case SIDE_TYPE_DYNAMIC_STRUCT:		/* Fallthrough */
	case SIDE_TYPE_DYNAMIC_STRUCT_VISITOR:	/* Fallthrough */
	case SIDE_TYPE_DYNAMIC_VLA:		/* Fallthrough */
	case SIDE_TYPE_DYNAMIC_VLA_VISITOR:
		visit_dynamic_type(type_visitor, item, priv);
		break;

	case SIDE_TYPE_OPTIONAL:
		type_visitor_optional(type_visitor, ctx, type_desc,
				side_ptr_get(item->u.side_static.side_optional),
				priv);
		break;

	default:
		fprintf(stderr, "<UNKNOWN TYPE>\n");
		abort();
	}
}

void type_visitor_event(const struct side_type_visitor *type_visitor,
		const struct side_event_description *desc,
		const struct side_arg_vec *side_arg_vec,
		const struct side_arg_dynamic_struct *var_struct,
		void *caller_addr, void *priv)
{
	const struct side_arg *sav = side_ptr_get(side_arg_vec->sav);
	uint32_t i, side_sav_len = side_arg_vec->len;
	const struct visit_context ctx = {
		.type = CONTEXT_NAMESPACE,
		.namespace = {
			.provider_name = side_ptr_get(desc->provider_name),
			.event_name = side_ptr_get(desc->event_name),
		},
	};

	if (desc->nr_fields != side_sav_len) {
		fprintf(stderr, "ERROR: number of fields mismatch between description and arguments\n");
		abort();
	}
	if (type_visitor->before_event_func)
		type_visitor->before_event_func(desc, side_arg_vec, var_struct, caller_addr, priv);
	if (side_sav_len) {
		if (type_visitor->before_static_fields_func)
			type_visitor->before_static_fields_func(side_arg_vec, priv);
		for (i = 0; i < side_sav_len; i++)
			side_visit_field(type_visitor, &ctx, &side_ptr_get(desc->fields)[i], &sav[i], priv);
		if (type_visitor->after_static_fields_func)
			type_visitor->after_static_fields_func(side_arg_vec, priv);
	}
	if (var_struct) {
		uint32_t var_struct_len = var_struct->len;

		if (type_visitor->before_variadic_fields_func)
			type_visitor->before_variadic_fields_func(var_struct, priv);
		for (i = 0; i < var_struct_len; i++)
			visit_dynamic_field(type_visitor, &side_ptr_get(var_struct->fields)[i], priv);
		if (type_visitor->after_variadic_fields_func)
			type_visitor->after_variadic_fields_func(var_struct, priv);
	}
	if (type_visitor->after_event_func)
		type_visitor->after_event_func(desc, side_arg_vec, var_struct, caller_addr, priv);
}
