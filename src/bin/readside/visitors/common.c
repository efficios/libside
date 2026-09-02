// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include "visitors/common.h"

static void side_type_struct_attributes(void *ctx, const struct side_type_struct *type,
					const struct side_attr **attr, u32 *nr_attr)
{
	if (type) {
		*nr_attr = type->attributes.length;
		*attr = *nr_attr ? visit_side_rel_pointer(ctx, type->attributes.elements) : NULL;
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
		*attr = *nr_attr ? visit_side_rel_pointer(ctx, type->attributes.elements) : NULL;
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
		*attr = *nr_attr ? visit_side_rel_pointer(ctx, type->attributes.elements) : NULL;
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
		*attr = *nr_attr ? visit_side_rel_pointer(ctx, type->attributes.elements) : NULL;
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
		*attr = *nr_attr ? visit_side_rel_pointer(ctx, type->attributes.elements) : NULL;
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

	mappings = visit_side_rel_pointer(ctx, type->mappings);

	if (!mappings) {
		goto error;
	}

	*nr_attr = mappings->attributes.length;
	*attr = *nr_attr ? visit_side_rel_pointer(ctx, mappings->attributes.elements) : NULL;

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

	mappings = visit_side_rel_pointer(ctx, type->mappings);

	if (!mappings) {
		goto error;
	}

	*nr_attr = mappings->attributes.length;
	*attr = *nr_attr ? visit_side_rel_pointer(ctx, mappings->attributes.elements) : NULL;

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
		*attr = *nr_attr ? visit_side_sel_pointer(ctx, type->u.side_null.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_BOOL:
		*nr_attr = type->u.side_bool.attributes.length;
		*attr = *nr_attr ? visit_side_sel_pointer(ctx, type->u.side_bool.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_BYTE:
		*nr_attr = type->u.side_byte.attributes.length;
		*attr = *nr_attr ? visit_side_sel_pointer(ctx, type->u.side_byte.attributes.elements) : NULL;
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
		*attr = *nr_attr ? visit_side_sel_pointer(ctx, type->u.side_integer.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_FLOAT_BINARY16:	/* fall through */
	case SIDE_TYPE_FLOAT_BINARY32:	/* fall through */
	case SIDE_TYPE_FLOAT_BINARY64:	/* fall through */
	case SIDE_TYPE_FLOAT_BINARY128:	/* fall through */
		*nr_attr = type->u.side_float.attributes.length;
		*attr = *nr_attr ? visit_side_sel_pointer(ctx, type->u.side_float.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_STRING_UTF8:	/* fall through */
	case SIDE_TYPE_STRING_UTF16:	/* fall through */
	case SIDE_TYPE_STRING_UTF32:	/* fall through */
		*nr_attr = type->u.side_string.attributes.length;
		*attr = *nr_attr ? visit_side_sel_pointer(ctx, type->u.side_string.attributes.elements) : NULL;
		break;
	case SIDE_TYPE_STRUCT:
		side_type_struct_attributes(ctx,
					visit_side_rel_pointer(ctx, type->u.side_struct),
					attr, nr_attr);
		break;
	case SIDE_TYPE_ARRAY:
		side_type_array_attributes(ctx,
					visit_side_rel_pointer(ctx, type->u.side_array),
					attr, nr_attr);
		break;
	case SIDE_TYPE_VLA:
		side_type_vla_attributes(ctx,
					visit_side_rel_pointer(ctx, type->u.side_vla),
					attr, nr_attr);
		break;
	case SIDE_TYPE_VARIANT:
		side_type_variant_attributes(ctx,
					visit_side_rel_pointer(ctx, type->u.side_variant),
					attr, nr_attr);
		break;
	case SIDE_TYPE_OPTIONAL:
		side_type_optional_attributes(ctx,
					visit_side_rel_pointer(ctx, type->u.side_optional),
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


