// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

/*
 * Referring to a type which has a name of its own, from here and from
 * another translation unit.
 *
 * A reference to a named type is the distance from the reference to the
 * type, which the assembler folds and which costs neither a relocation
 * nor a private dirty page. That only works within the unit being
 * assembled, so a reference to a type defined elsewhere says so with
 * side_extern() and holds the type's address instead. Both forms are
 * the same member, which says which of the two it holds; see
 * side_ptr_sel_t.
 *
 * What is checked is that the two forms really are different -- one a
 * distance and the other an address -- and that both reach the type
 * they name. A test which only followed them would pass with either
 * form used for both.
 *
 * extern-type-tu2.c defines the types which are elsewhere.
 */

#include <stdint.h>
#include <string.h>

#include <tap.h>

#include "extern-type.h"

/* Defined here, so references to these are distances. */
side_static_define_struct(local_struct,
	side_field_list(
		side_field_u32("here_x"),
	)
);

side_static_define_enum(local_enum,
	side_enum_mapping_list(
		side_enum_mapping_range("here-one-ten", 1, 10),
	)
);

side_static_event(my_event, "extern_type", "myevent", SIDE_LOGLEVEL_DEBUG,
	side_field_list(
		side_field_struct("here", local_struct),
		side_field_struct("there", side_extern(shared_struct)),
		side_field_enum("here_enum", local_enum, side_elem(side_type_u32())),
		side_field_enum("there_enum", side_extern(shared_enum), side_elem(side_type_u32())),
	)
);

static
const struct side_event_field *field_at(uint32_t i)
{
	const struct side_event_description *desc =
		side_event_state_description(&side_event_state__my_event.parent);

	return &side_array_rel_elements(&desc->fields)[i];
}

static
const char *field_name_at(uint32_t i)
{
	return side_ptr_rel_get(field_at(i)->field_name);
}

int main(void)
{
	const struct side_type *here, *there, *here_enum, *there_enum;

	plan_tests(14);

	here = &field_at(0)->side_type;
	there = &field_at(1)->side_type;
	here_enum = &field_at(2)->side_type;
	there_enum = &field_at(3)->side_type;

	ok(strcmp(field_name_at(0), "here") == 0, "the first field is the local structure");
	ok(strcmp(field_name_at(1), "there") == 0, "the second field is the foreign structure");
	ok(strcmp(field_name_at(2), "here_enum") == 0, "the third field is the local enumeration");
	ok(strcmp(field_name_at(3), "there_enum") == 0, "the fourth field is the foreign enumeration");

	/* A type defined here is reached by a distance. */
	ok(here->u.side_struct.is_offset,
		"a structure defined in this unit is reached by a distance");
	ok(side_ptr_sel_get(here->u.side_struct) == &local_struct,
		"that distance reaches the structure it names");
	ok(here_enum->u.side_enum.mappings.is_offset,
		"an enumeration defined in this unit is reached by a distance");
	ok(side_ptr_sel_get(here_enum->u.side_enum.mappings) == &local_enum,
		"that distance reaches the mappings it names");

	/*
	 * A type defined in another unit is reached by an address: the
	 * assembler cannot subtract across units, so there is no
	 * distance to hold.
	 */
	ok(!there->u.side_struct.is_offset,
		"a structure defined in another unit is reached by an address");
	ok(side_ptr_sel_get(there->u.side_struct) == &shared_struct,
		"that address reaches the structure it names");
	ok(!there_enum->u.side_enum.mappings.is_offset,
		"an enumeration defined in another unit is reached by an address");
	ok(side_ptr_sel_get(there_enum->u.side_enum.mappings) == &shared_enum,
		"that address reaches the mappings it names");

	/* Both are the type they claim to be, read through either form. */
	ok(side_array_rel_elements(&side_ptr_sel_get(here->u.side_struct)->fields)[0].field_name.off != 0
		&& strcmp(side_ptr_rel_get(side_array_rel_elements(
			&side_ptr_sel_get(here->u.side_struct)->fields)[0].field_name), "here_x") == 0,
		"the local structure holds the field it was defined with");
	ok(strcmp(side_ptr_rel_get(side_array_rel_elements(
			&side_ptr_sel_get(there->u.side_struct)->fields)[0].field_name), "there_x") == 0,
		"the foreign structure holds the field it was defined with");

	return exit_status();
}
