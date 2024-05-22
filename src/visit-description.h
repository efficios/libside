// SPDX-License-Identifier: MIT
/*
 * Copyright 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _VISIT_DESCRIPTION_H
#define _VISIT_DESCRIPTION_H

#include <side/trace.h>

enum side_description_visitor_location {
	SIDE_DESCRIPTION_VISITOR_BEFORE,
	SIDE_DESCRIPTION_VISITOR_AFTER,
};

enum side_description_visitor_vla_location {
	SIDE_DESCRIPTION_VISITOR_VLA_BEFORE,
	SIDE_DESCRIPTION_VISITOR_VLA_AFTER_LENGTH,
	SIDE_DESCRIPTION_VISITOR_VLA_AFTER_ELEMENT,
};

struct side_description_visitor {
	void (*event_func)(enum side_description_visitor_location loc,
			const struct side_event_description *desc,
			void *priv);

	void (*static_fields_func)(enum side_description_visitor_location loc,
			const struct side_event_description *desc,
			void *priv);

	/* Stack-copy basic types. */
	void (*field_func)(enum side_description_visitor_location loc, const struct side_event_field *item_desc, void *priv);
	void (*elem_func)(enum side_description_visitor_location loc, const struct side_type *type_desc, void *priv);
	void (*option_func)(enum side_description_visitor_location loc, const struct side_variant_option *option_desc, void *priv);

	void (*null_type_func)(const struct side_type *type_desc, void *priv);
	void (*bool_type_func)(const struct side_type *type_desc, void *priv);
	void (*integer_type_func)(const struct side_type *type_desc, void *priv);
	void (*byte_type_func)(const struct side_type *type_desc, void *priv);
	void (*pointer_type_func)(const struct side_type *type_desc, void *priv);
	void (*float_type_func)(const struct side_type *type_desc, void *priv);
	void (*string_type_func)(const struct side_type *type_desc, void *priv);

	/* Stack-copy compound types. */
	void (*struct_type_func)(enum side_description_visitor_location loc, const struct side_type_struct *side_struct, void *priv);
	void (*variant_type_func)(enum side_description_visitor_location loc, const struct side_type_variant *side_variant, void *priv);
	void (*array_type_func)(enum side_description_visitor_location loc, const struct side_type_array *side_array, void *priv);
	void (*vla_type_func)(enum side_description_visitor_vla_location loc, const struct side_type_vla *side_vla, void *priv);
	void (*vla_visitor_type_func)(enum side_description_visitor_vla_location loc, const struct side_type_vla_visitor *side_vla_visitor, void *priv);

	/* Stack-copy enumeration types. */
	void (*enum_type_func)(enum side_description_visitor_location loc, const struct side_type *type_desc, void *priv);
	void (*enum_bitmap_type_func)(enum side_description_visitor_location loc, const struct side_type *type_desc, void *priv);

	/* Gather basic types. */
	void (*gather_bool_type_func)(const struct side_type_gather_bool *type, void *priv);
	void (*gather_byte_type_func)(const struct side_type_gather_byte *type, void *priv);
	void (*gather_integer_type_func)(const struct side_type_gather_integer *type, void *priv);
	void (*gather_pointer_type_func)(const struct side_type_gather_integer *type, void *priv);
	void (*gather_float_type_func)(const struct side_type_gather_float *type, void *priv);
	void (*gather_string_type_func)(const struct side_type_gather_string *type, void *priv);

	/* Gather compound types. */
	void (*gather_struct_type_func)(enum side_description_visitor_location loc, const struct side_type_gather_struct *type, void *priv);
	void (*gather_array_type_func)(enum side_description_visitor_location loc, const struct side_type_gather_array *type, void *priv);
	void (*gather_vla_type_func)(enum side_description_visitor_vla_location loc, const struct side_type_gather_vla *type, void *priv);

	/* Gather enumeration types. */
	void (*gather_enum_type_func)(enum side_description_visitor_location loc, const struct side_type_gather_enum *type, void *priv);

	/* Dynamic types. */
	void (*dynamic_type_func)(const struct side_type *type_desc, void *priv);
};

void description_visitor_event(const struct side_description_visitor *description_visitor,
		const struct side_event_description *desc, void *priv);

#endif /* _VISIT_DESCRIPTION_H */
