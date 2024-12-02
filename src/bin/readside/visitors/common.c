// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include "visitors/common.h"

static void side_type_struct_attributes(void *ctx, const struct side_type_struct *type,
					const struct side_attr **attr, u32 *nr_attr)
{
	if (type) {
		*nr_attr = type->attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->attributes.elements) : NULL;
	} else {
		*attr = NULL;
		*nr_attr = 0;
	}
}

static void side_type_array_attributes(void *ctx, const struct side_type_array *type,
				const struct side_attr **attr, u32 *nr_attr)
{
	if (type) {
		*nr_attr = type->attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->attributes.elements) : NULL;
	} else {
		*attr = NULL;
		*nr_attr = 0;
	}
}

static void side_type_vla_attributes(void *ctx, const struct side_type_vla *type,
				const struct side_attr **attr, u32 *nr_attr)
{
	if (type) {
		*nr_attr = type->attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->attributes.elements) : NULL;
	} else {
		*attr = NULL;
		*nr_attr = 0;
	}
}

static void side_type_variant_attributes(void *ctx, const struct side_type_variant *type,
					const struct side_attr **attr, u32 *nr_attr)
{
	if (type) {
		*nr_attr = type->attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->attributes.elements) : NULL;
	} else {
		*attr = NULL;
		*nr_attr = 0;
	}
}


static void side_type_optional_attributes(void *ctx, const struct side_type_optional *type,
					const struct side_attr **attr, u32 *nr_attr)
{
	if (type) {
		*nr_attr = type->attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->attributes.elements) : NULL;
	} else {
		*attr = NULL;
		*nr_attr = 0;
	}
}


static void side_type_vla_visitor_attributes(void *ctx, const struct side_type_vla_visitor *type,
					const struct side_attr **attr, u32 *nr_attr)
{
	if (type) {
		*nr_attr = type->attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->attributes.elements) : NULL;
	} else {
		*attr = NULL;
		*nr_attr = 0;
	}
}

static void side_type_enum_attributes(void *ctx, const struct side_type_enum *type,
				const struct side_attr **attr, u32 *nr_attr)
{
	const struct side_enum_mappings *mappings;

	if (!type) {
		goto error;
	}

	mappings = visit_side_pointer(ctx, type->mappings);

	if (!mappings) {
		goto error;
	}

	*nr_attr = mappings->attributes.length;
	*attr = *nr_attr ? visit_side_pointer(ctx, mappings->attributes.elements) : NULL;

	return;
error:
	*attr = NULL;
	*nr_attr = 0;
}

static void side_type_enum_bitmap_attributes(void *ctx, const struct side_type_enum_bitmap *type,
				const struct side_attr **attr, u32 *nr_attr)
{
	const struct side_enum_bitmap_mappings *mappings;

	if (!type) {
		goto error;
	}

	mappings = visit_side_pointer(ctx, type->mappings);

	if (!mappings) {
		goto error;
	}

	*nr_attr = mappings->attributes.length;
	*attr = *nr_attr ? visit_side_pointer(ctx, mappings->attributes.elements) : NULL;

	return;
error:
	*attr = NULL;
	*nr_attr = 0;
}

void side_type_attributes(const struct side_type *type, void *ctx,
			const struct side_attr **attr, u32 *nr_attr)
{
	switch (side_enum_get(type->type)) {
	case SIDE_TYPE_NULL:
		*nr_attr = type->u.side_null.attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->u.side_null.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_BOOL:
		*nr_attr = type->u.side_bool.attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->u.side_bool.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_BYTE:
		*nr_attr = type->u.side_byte.attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->u.side_byte.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_U8:	/* fall through */
	case SIDE_TYPE_U16:	/* fall through */
	case SIDE_TYPE_U32:	/* fall through */
	case SIDE_TYPE_U64:	/* fall through */
	case SIDE_TYPE_U128:	/* fall through */
	case SIDE_TYPE_S8:	/* fall through */
	case SIDE_TYPE_S16:	/* fall through */
	case SIDE_TYPE_S32:	/* fall through */
	case SIDE_TYPE_S64:	/* fall through */
	case SIDE_TYPE_S128:	/* fall through */
	case SIDE_TYPE_POINTER:	/* fall through */
		*nr_attr = type->u.side_integer.attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->u.side_integer.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_FLOAT_BINARY16:	/* fall through */
	case SIDE_TYPE_FLOAT_BINARY32:	/* fall through */
	case SIDE_TYPE_FLOAT_BINARY64:	/* fall through */
	case SIDE_TYPE_FLOAT_BINARY128:	/* fall through */
		*nr_attr = type->u.side_float.attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->u.side_float.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_STRING_UTF8:	/* fall through */
	case SIDE_TYPE_STRING_UTF16:	/* fall through */
	case SIDE_TYPE_STRING_UTF32:	/* fall through */
		*nr_attr = type->u.side_string.attributes.length;
		*attr = *nr_attr ? visit_side_pointer(ctx, type->u.side_string.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_STRUCT:
		side_type_struct_attributes(ctx,
					visit_side_pointer(ctx, type->u.side_struct),
					attr, nr_attr);
		break;
	case SIDE_TYPE_ARRAY:
		side_type_array_attributes(ctx,
					visit_side_pointer(ctx, type->u.side_array),
					attr, nr_attr);
		break;
	case SIDE_TYPE_VLA:
		side_type_vla_attributes(ctx,
					visit_side_pointer(ctx, type->u.side_vla),
					attr, nr_attr);
		break;
	case SIDE_TYPE_VARIANT:
		side_type_variant_attributes(ctx,
					visit_side_pointer(ctx, type->u.side_variant),
					attr, nr_attr);
		break;
	case SIDE_TYPE_OPTIONAL:
		side_type_optional_attributes(ctx,
					visit_side_pointer(ctx, type->u.side_optional),
					attr, nr_attr);
		break;
	case SIDE_TYPE_VLA_VISITOR:
		side_type_vla_visitor_attributes(ctx,
						visit_side_pointer(ctx, type->u.side_vla_visitor),
						attr, nr_attr);
		break;
	case SIDE_TYPE_ENUM:
		side_type_enum_attributes(ctx, &type->u.side_enum,
					attr, nr_attr);
		break;
	case SIDE_TYPE_ENUM_BITMAP:
		side_type_enum_bitmap_attributes(ctx, &type->u.side_enum_bitmap,
						attr, nr_attr);
		break;
	default:
		*attr = NULL;
		*nr_attr = 0;
		break;
	}
}

const char *side_loglevel_to_string(enum side_loglevel loglevel)
{
	switch (loglevel) {
	case SIDE_LOGLEVEL_EMERG:
		return "EMERG";
	case SIDE_LOGLEVEL_ALERT:
		return "ALERT";
	case SIDE_LOGLEVEL_CRIT:
		return "CRIT";
	case SIDE_LOGLEVEL_ERR:
		return "ERR";
	case SIDE_LOGLEVEL_WARNING:
		return "WARNING";
	case SIDE_LOGLEVEL_NOTICE:
		return "NOTICE";
	case SIDE_LOGLEVEL_INFO:
		return "INFO";
	case SIDE_LOGLEVEL_DEBUG:
		return "DEBUG";
	default:
		return "<UNKNOWN>";
	}
}

const char *side_type_to_string(enum side_type_label label)
{
	switch (label) {
	case SIDE_TYPE_NULL:
		return "NULL";
	case SIDE_TYPE_BOOL:
		return "BOOL";
	case SIDE_TYPE_U8:
		return "U8";
	case SIDE_TYPE_U16:
		return "U16";
	case SIDE_TYPE_U32:
		return "U32";
	case SIDE_TYPE_U64:
		return "U64";
	case SIDE_TYPE_U128:
		return "U128";
	case SIDE_TYPE_S8:
		return "S8";
	case SIDE_TYPE_S16:
		return "S16";
	case SIDE_TYPE_S32:
		return "S32";
	case SIDE_TYPE_S64:
		return "S64";
	case SIDE_TYPE_S128:
		return "S128";
	case SIDE_TYPE_BYTE:
		return "BYTE";
	case SIDE_TYPE_POINTER:
		return "POINTER";
	case SIDE_TYPE_FLOAT_BINARY16:
		return "FLOAT_BINARY16";
	case SIDE_TYPE_FLOAT_BINARY32:
		return "FLOAT_BINARY32";
	case SIDE_TYPE_FLOAT_BINARY64:
		return "FLOAT_BINARY64";
	case SIDE_TYPE_FLOAT_BINARY128:
		return "FLOAT_BINARY128";
	case SIDE_TYPE_STRING_UTF8:
		return "STRING_UTF8";
	case SIDE_TYPE_STRING_UTF16:
		return "STRING_UTF16";
	case SIDE_TYPE_STRING_UTF32:
		return "STRING_UTF32";
	case SIDE_TYPE_STRUCT:
		return "STRUCT";
	case SIDE_TYPE_VARIANT:
		return "VARIANT";
	case SIDE_TYPE_OPTIONAL:
		return "OPTIONAL";
	case SIDE_TYPE_ARRAY:
		return "ARRAY";
	case SIDE_TYPE_VLA:
		return "VLA";
	case SIDE_TYPE_VLA_VISITOR:
		return "VLA_VISITOR";
	case SIDE_TYPE_ENUM:
		return "ENUM";
	case SIDE_TYPE_ENUM_BITMAP:
		return "ENUM_BITMAP";
	case SIDE_TYPE_DYNAMIC:
		return "DYNAMIC";
	case SIDE_TYPE_GATHER_BOOL:
		return "GATHER_BOOL";
	case SIDE_TYPE_GATHER_INTEGER:
		return "GATHER_INTEGER";
	case SIDE_TYPE_GATHER_BYTE:
		return "GATHER_BYTE";
	case SIDE_TYPE_GATHER_POINTER:
		return "GATHER_POINTER";
	case SIDE_TYPE_GATHER_FLOAT:
		return "GATHER_FLOAT";
	case SIDE_TYPE_GATHER_STRING:
		return "GATHER_STRING";
	case SIDE_TYPE_GATHER_STRUCT:
		return "GATHER_STRUCT";
	case SIDE_TYPE_GATHER_ARRAY:
		return "GATHER_ARRAY";
	case SIDE_TYPE_GATHER_VLA:
		return "GATHER_VLA";
	case SIDE_TYPE_GATHER_ENUM:
		return "GATHER_ENUM";
	case SIDE_TYPE_DYNAMIC_NULL:
		return "DYNAMIC_NULL";
	case SIDE_TYPE_DYNAMIC_BOOL:
		return "DYNAMIC_BOOL";
	case SIDE_TYPE_DYNAMIC_INTEGER:
		return "DYNAMIC_INTEGER";
	case SIDE_TYPE_DYNAMIC_BYTE:
		return "DYNAMIC_BYTE";
	case SIDE_TYPE_DYNAMIC_POINTER:
		return "DYNAMIC_POINTER";
	case SIDE_TYPE_DYNAMIC_FLOAT:
		return "DYNAMIC_FLOAT";
	case SIDE_TYPE_DYNAMIC_STRING:
		return "DYNAMIC_STRING";
	case SIDE_TYPE_DYNAMIC_STRUCT:
		return "DYNAMIC_STRUCT";
	case SIDE_TYPE_DYNAMIC_STRUCT_VISITOR:
		return "DYNAMIC_STRUCT_VISITOR";
	case SIDE_TYPE_DYNAMIC_VLA:
		return "DYNAMIC_VLA";
	case SIDE_TYPE_DYNAMIC_VLA_VISITOR:
		return "DYNAMIC_VLA_VISITOR";
	default:
		return "<UNKNOWN>";
	}
}
