// SPDX-License-Identifier: MIT
/*
 * Copyright 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef LIBSIDE_TOOLS_VISIT_EVENT_DESCRIPTION_H
#define LIBSIDE_TOOLS_VISIT_EVENT_DESCRIPTION_H

#include <side/abi/event-description.h>
#include <side/abi/type-description.h>

struct side_description_visitor_callbacks {
	void (*before_event_func)(const struct side_event_description *desc, void *priv);
	void (*after_event_func)(const struct side_event_description *desc, void *priv);

	void (*before_static_fields_func)(const struct side_event_description *desc, void *priv);
	void (*after_static_fields_func)(const struct side_event_description *desc, void *priv);

	/* Stack-copy basic types. */
	void (*before_field_func)(const struct side_event_field *item_desc, void *priv);
	void (*after_field_func)(const struct side_event_field *item_desc, void *priv);
	void (*before_elem_func)(const struct side_type *type_desc, void *priv);
	void (*after_elem_func)(const struct side_type *type_desc, void *priv);
	void (*before_option_func)(const struct side_variant_option *option_desc, void *priv);
	void (*after_option_func)(const struct side_variant_option *option_desc, void *priv);

	void (*null_type_func)(const struct side_type_null *type_desc, void *priv);
	void (*bool_type_func)(const struct side_type_bool *type_desc, void *priv);
	void (*integer_type_func)(const struct side_type_integer *type_desc, void *priv);
	void (*byte_type_func)(const struct side_type_byte *type_desc, void *priv);
	void (*pointer_type_func)(const struct side_type_integer *type_desc, void *priv);
	void (*float_type_func)(const struct side_type_float *type_desc, void *priv);
	void (*string_type_func)(const struct side_type_string *type_desc, void *priv);

	/* Stack-copy compound types. */
	void (*before_struct_type_func)(const struct side_type_struct *side_struct, void *priv);
	void (*after_struct_type_func)(const struct side_type_struct *side_struct, void *priv);
	void (*before_variant_type_func)(const struct side_type_variant *side_variant, void *priv);
	void (*after_variant_selector_type_func)(const struct side_type *side_type, void *priv);
	void (*after_variant_type_func)(const struct side_type_variant *side_variant, void *priv);
	void (*before_array_type_func)(const struct side_type_array *side_array, void *priv);
	void (*after_array_type_func)(const struct side_type_array *side_array, void *priv);
	void (*before_vla_type_func)(const struct side_type_vla *side_vla, void *priv);
	void (*after_length_vla_type_func)(const struct side_type_vla *side_vla, void *priv);
	void (*after_element_vla_type_func)(const struct side_type_vla *side_vla, void *priv);
	void (*before_optional_type_func)(const struct side_type_optional *optional, void *priv);
	void (*after_optional_type_func)(const struct side_type_optional *optional, void *priv);

	/* Stack-copy enumeration types. */
	void (*before_enum_type_func)(const struct side_type_enum *type_desc, void *priv);
	void (*after_enum_type_func)(const struct side_type_enum *type_desc, void *priv);
	void (*before_enum_bitmap_type_func)(const struct side_type_enum_bitmap *type_desc, void *priv);
	void (*after_enum_bitmap_type_func)(const struct side_type_enum_bitmap *type_desc, void *priv);

	/* Gather basic types. */
	void (*gather_bool_type_func)(const struct side_type_gather_bool *type, void *priv);
	void (*gather_byte_type_func)(const struct side_type_gather_byte *type, void *priv);
	void (*gather_integer_type_func)(const struct side_type_gather_integer *type, void *priv);
	void (*gather_pointer_type_func)(const struct side_type_gather_integer *type, void *priv);
	void (*gather_float_type_func)(const struct side_type_gather_float *type, void *priv);
	void (*gather_string_type_func)(const struct side_type_gather_string *type, void *priv);

	/* Gather compound types. */
	void (*before_gather_struct_type_func)(const struct side_type_gather_struct *type, void *priv);
	void (*after_gather_struct_type_func)(const struct side_type_gather_struct *type, void *priv);
	void (*before_gather_array_type_func)(const struct side_type_gather_array *type, void *priv);
	void (*after_gather_array_type_func)(const struct side_type_gather_array *type, void *priv);
	void (*before_gather_vla_type_func)(const struct side_type_gather_vla *type, void *priv);
	void (*after_length_gather_vla_type_func)(const struct side_type_gather_vla *type, void *priv);
	void (*after_element_gather_vla_type_func)(const struct side_type_gather_vla *type, void *priv);

	/* Gather enumeration types. */
	void (*before_gather_enum_type_func)(const struct side_type_gather_enum *type, void *priv);
	void (*after_gather_enum_type_func)(const struct side_type_gather_enum *type, void *priv);

	/* Dynamic types. */
	void (*dynamic_type_func)(const struct side_type *type_desc, void *priv);
};

struct side_description_visitor {
	const struct side_description_visitor_callbacks *callbacks;
	void *priv;
	/*
	 * How to reach an address which a description holds, given priv.
	 * A reference to a type defined in another translation unit is
	 * an address rather than a distance, and is the only thing this
	 * walk has to resolve; everything else it follows is measured
	 * from the field holding it. Null where the description is read
	 * in the process which wrote it, resolving being the identity
	 * there. See side_ptr_sel_t.
	 */
	void *(*resolve)(void *ptr, void *priv);
};

void visit_event_description(const struct side_description_visitor *visitor,
			const struct side_event_description *desc);

#endif /* LIBSIDE_TOOLS_VISIT_EVENT_DESCRIPTION_H */
