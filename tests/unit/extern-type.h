// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

#ifndef SIDE_TESTS_EXTERN_TYPE_H
#define SIDE_TESTS_EXTERN_TYPE_H

#include <side/trace.h>

/*
 * Types defined in extern-type-tu2.c, which any unit referring to them
 * declares here. A reference to one of them is an address rather than a
 * distance -- the assembler folds a distance only within the unit it is
 * assembling -- and side_extern() at the reference is what says so.
 *
 * The fields are written out again here, the way a C header writes out
 * the shape of a structure: it is what the static checker works from to
 * go on checking arguments against the type. They cannot be hidden
 * behind a macro of their own: the field list is dispatched on by
 * pasting, and a macro name pasted onto reaches nothing, which yields a
 * structure with no field at all and says nothing about it.
 */
side_declare_struct(shared_struct,
	side_field_list(
		side_field_u32("there_x"),
		side_field_s64("there_y"),
	)
);

side_declare_enum(shared_enum);

#endif /* SIDE_TESTS_EXTERN_TYPE_H */
