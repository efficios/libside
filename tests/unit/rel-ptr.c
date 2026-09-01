// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

/*
 * Self-relative pointers: side_ptr_rel_t and the macros which emit the
 * distances it holds.
 *
 * What is checked is both halves of the contract. That the distances
 * resolve to what they were built from is the obvious half. The other
 * is that they really are distances: a description which held addresses
 * would resolve just as correctly, because the loader would have
 * written them, and would cost a relocation and a private dirty page
 * for each one. So the test also requires that what is stored is not
 * the address it resolves to, and that it is small enough to be a
 * distance within the section rather than an address.
 *
 * rel-ptr-tu2.c takes part as a second translation unit: the symbols
 * naming the distances are local to the unit which defines them, and
 * two units defining events must not collide.
 */

#include <side/macros.h>
#include <stdint.h>
#include <string.h>

#include <tap.h>

#include "rel-ptr.h"

/* Everything taking part shares one section, and none of it is const. */
#define TEST_DESC __attribute__((section("side_test_desc"), aligned(8), used))

TEST_DESC static char tu1_provider[] = "provider_one";
TEST_DESC static char tu1_f0_name[] = "first_field";
TEST_DESC static char tu1_f1_name[] = "second_field";

extern struct test_field tu1_fields[2];
extern struct test_event tu1_event;

SIDE_PTR_REL_DEFINE(tu1_off_provider, tu1_event, struct test_event, provider,
	tu1_provider)
SIDE_PTR_REL_DEFINE(tu1_off_fields, tu1_event, struct test_event, fields,
	tu1_fields)
SIDE_PTR_REL_DEFINE_AT(tu1_off_f0, tu1_fields, TEST_FIELD_NAME_AT(0), tu1_f0_name)
SIDE_PTR_REL_DEFINE_AT(tu1_off_f1, tu1_fields, TEST_FIELD_NAME_AT(1), tu1_f1_name)

TEST_DESC struct test_field tu1_fields[2] = {
	{ .name = SIDE_PTR_REL_INIT(tu1_off_f0), .type = 10 },
	{ .name = SIDE_PTR_REL_INIT(tu1_off_f1), .type = 20 },
};

TEST_DESC struct test_event tu1_event = {
	.provider = SIDE_PTR_REL_INIT(tu1_off_provider),
	.fields = SIDE_PTR_REL_INIT(tu1_off_fields),
	.nr_fields = 2,
};

/*
 * A distance, not an address: what is stored must differ from the
 * address it resolves to, and must be small enough to be an offset
 * within a section rather than a pointer.
 */
static void check_is_offset(const char *what, intptr_t stored, const void *resolved)
{
	ok(stored != (intptr_t) resolved,
		"%s: stored value is not the address it resolves to", what);
	ok(stored > -(intptr_t) (1 << 30) && stored < (intptr_t) (1 << 30),
		"%s: stored value is a distance, not an address (%ld)", what,
		(long) stored);
}

static void check_event(const struct test_event *ev, const char *provider,
		const char *f0, const char *f1, const char *what)
{
	const struct test_field *fields = side_ptr_rel_get(ev->fields);

	ok(strcmp((const char *) side_ptr_rel_get(ev->provider), provider) == 0,
		"%s: provider resolves to \"%s\"", what, provider);
	ok(ev->nr_fields == 2, "%s: carries 2 fields", what);
	ok(strcmp((const char *) side_ptr_rel_get(fields[0].name), f0) == 0,
		"%s: first field resolves to \"%s\"", what, f0);
	ok(strcmp((const char *) side_ptr_rel_get(fields[1].name), f1) == 0,
		"%s: second field resolves to \"%s\"", what, f1);
	ok(fields[0].type == 10 && fields[1].type == 20,
		"%s: the fields around the distances are intact", what);
}

int main(void)
{
	plan_tests(16);

	check_event(&tu1_event, "provider_one", "first_field", "second_field",
		"same translation unit");
	check_is_offset("event to provider", (intptr_t) tu1_event.provider.off,
		side_ptr_rel_get(tu1_event.provider));
	check_is_offset("field to name", (intptr_t) tu1_fields[0].name.off,
		side_ptr_rel_get(tu1_fields[0].name));

	/*
	 * A second translation unit: its distances are its own, and the
	 * symbols naming them do not collide with the ones above.
	 */
	check_event(tu2_event(), "provider_two", "alpha", "beta",
		"other translation unit");

	ok(side_ptr_rel_get(tu1_event.provider) != side_ptr_rel_get(tu2_event()->provider),
		"the two translation units resolve to their own data");

	/* The distance is relative to the field, so a copy does not carry. */
	ok((char *) side_ptr_rel_get(tu1_fields[1].name) ==
			(char *) &tu1_fields[1].name.off + (intptr_t) tu1_fields[1].name.off,
		"a distance is measured from the field which holds it");

	return exit_status();
}
