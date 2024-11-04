// SPDX-License-Identifier: MIT
/*
 * Copyright 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _VISIT_ARG_VEC_H
#define _VISIT_ARG_VEC_H

#include <side/trace.h>

struct side_type_visitor {
	void (*before_event_func)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_struct *var_struct,
			void *caller_addr, void *priv);
	void (*after_event_func)(const struct side_event_description *desc,
			const struct side_arg_vec *side_arg_vec,
			const struct side_arg_dynamic_struct *var_struct,
			void *caller_addr, void *priv);

	void (*before_static_fields_func)(const struct side_arg_vec *side_arg_vec, void *priv);
	void (*after_static_fields_func)(const struct side_arg_vec *side_arg_vec, void *priv);

	void (*before_variadic_fields_func)(const struct side_arg_dynamic_struct *var_struct, void *priv);
	void (*after_variadic_fields_func)(const struct side_arg_dynamic_struct *var_struct, void *priv);

	/* Stack-copy basic types. */
	void (*before_field_func)(const struct side_event_field *item_desc, void *priv);
	void (*after_field_func)(const struct side_event_field *item_desc, void *priv);
	void (*before_elem_func)(const struct side_type *type_desc, void *priv);
	void (*after_elem_func)(const struct side_type *type_desc, void *priv);

	void (*null_type_func)(const struct side_type *type_desc, const struct side_arg *item, void *priv);
	void (*bool_type_func)(const struct side_type *type_desc, const struct side_arg *item, void *priv);
	void (*integer_type_func)(const struct side_type *type_desc, const struct side_arg *item, void *priv);
	void (*byte_type_func)(const struct side_type *type_desc, const struct side_arg *item, void *priv);
	void (*pointer_type_func)(const struct side_type *type_desc, const struct side_arg *item, void *priv);
	void (*float_type_func)(const struct side_type *type_desc, const struct side_arg *item, void *priv);
	void (*string_type_func)(const struct side_type *type_desc, const struct side_arg *item, void *priv);

	/* Stack-copy compound types. */
	void (*before_struct_type_func)(const struct side_type_struct *side_struct, const struct side_arg_vec *side_arg_vec, void *priv);
	void (*after_struct_type_func)(const struct side_type_struct *side_struct, const struct side_arg_vec *side_arg_vec, void *priv);
	void (*before_array_type_func)(const struct side_type_array *side_array, const struct side_arg_vec *side_arg_vec, void *priv);
	void (*after_array_type_func)(const struct side_type_array *side_array, const struct side_arg_vec *side_arg_vec, void *priv);
	void (*before_vla_type_func)(const struct side_type_vla *side_vla, const struct side_arg_vec *side_arg_vec, void *priv);
	void (*after_vla_type_func)(const struct side_type_vla *side_vla, const struct side_arg_vec *side_arg_vec, void *priv);
	void (*before_vla_visitor_type_func)(const struct side_type_vla_visitor *side_vla_visitor, const struct side_arg_vla_visitor *side_arg_vla_visitor, void *priv);
	void (*after_vla_visitor_type_func)(const struct side_type_vla_visitor *side_vla_visitor, const struct side_arg_vla_visitor *side_arg_vla_visitor, void *priv);

	/* Stack-copy enumeration types. */
	void (*enum_type_func)(const struct side_type *type_desc, const struct side_arg *item, void *priv);
	void (*enum_bitmap_type_func)(const struct side_type *type_desc, const struct side_arg *item, void *priv);

	/* Gather basic types. */
	void (*gather_bool_type_func)(const struct side_type_gather_bool *type, const union side_bool_value *value, void *priv);
	void (*gather_byte_type_func)(const struct side_type_gather_byte *type, const uint8_t *_ptr, void *priv);
	void (*gather_integer_type_func)(const struct side_type_gather_integer *type, const union side_integer_value *value, void *priv);
	void (*gather_pointer_type_func)(const struct side_type_gather_integer *type, const union side_integer_value *value, void *priv);
	void (*gather_float_type_func)(const struct side_type_gather_float *type, const union side_float_value *value, void *priv);
	void (*gather_string_type_func)(const struct side_type_gather_string *type, const void *p, uint8_t unit_size,
				enum side_type_label_byte_order byte_order, size_t strlen_with_null, void *priv);

	/* Gather compound types. */
	void (*before_gather_struct_type_func)(const struct side_type_struct *type, void *priv);
	void (*after_gather_struct_type_func)(const struct side_type_struct *type, void *priv);
	void (*before_gather_array_type_func)(const struct side_type_array *type, void *priv);
	void (*after_gather_array_type_func)(const struct side_type_array *type, void *priv);
	void (*before_gather_vla_type_func)(const struct side_type_vla *type, uint32_t length, void *priv);
	void (*after_gather_vla_type_func)(const struct side_type_vla *type, uint32_t length, void *priv);

	/* Gather enumeration types. */
	void (*gather_enum_type_func)(const struct side_type_gather_enum *type, const union side_integer_value *value, void *priv);

	/* Dynamic basic types. */
	void (*before_dynamic_field_func)(const struct side_arg_dynamic_field *field, void *priv);
	void (*after_dynamic_field_func)(const struct side_arg_dynamic_field *field, void *priv);
	void (*before_dynamic_elem_func)(const struct side_arg *dynamic_item, void *priv);
	void (*after_dynamic_elem_func)(const struct side_arg *dynamic_item, void *priv);

	void (*dynamic_null_func)(const struct side_arg *item, void *priv);
	void (*dynamic_bool_func)(const struct side_arg *item, void *priv);
	void (*dynamic_integer_func)(const struct side_arg *item, void *priv);
	void (*dynamic_byte_func)(const struct side_arg *item, void *priv);
	void (*dynamic_pointer_func)(const struct side_arg *item, void *priv);
	void (*dynamic_float_func)(const struct side_arg *item, void *priv);
	void (*dynamic_string_func)(const struct side_arg *item, void *priv);

	/* Dynamic compound types. */
	void (*before_dynamic_struct_func)(const struct side_arg_dynamic_struct *dynamic_struct, void *priv);
	void (*after_dynamic_struct_func)(const struct side_arg_dynamic_struct *dynamic_struct, void *priv);
	void (*before_dynamic_struct_visitor_func)(const struct side_arg *item, void *priv);
	void (*after_dynamic_struct_visitor_func)(const struct side_arg *item, void *priv);
	void (*before_dynamic_vla_func)(const struct side_arg_dynamic_vla *vla, void *priv);
	void (*after_dynamic_vla_func)(const struct side_arg_dynamic_vla *vla, void *priv);
	void (*before_dynamic_vla_visitor_func)(const struct side_arg *item, void *priv);
	void (*after_dynamic_vla_visitor_func)(const struct side_arg *item, void *priv);
};

void type_visitor_event(const struct side_type_visitor *type_visitor,
		const struct side_event_description *desc,
		const struct side_arg_vec *side_arg_vec,
		const struct side_arg_dynamic_struct *var_struct,
		void *caller_addr, void *priv);

#endif /* _VISIT_ARG_VEC_H */
