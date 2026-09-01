// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

/*
 * A second translation unit taking part, so that the test covers what
 * happens when more than one of them defines distances: the symbols
 * naming those distances are local to the unit which defines them.
 */

#include "rel-ptr.h"

#define TEST_DESC __attribute__((section("side_test_desc"), aligned(8), used))

TEST_DESC static char tu2_provider[] = "provider_two";
TEST_DESC static char tu2_f0_name[] = "alpha";
TEST_DESC static char tu2_f1_name[] = "beta";

extern struct test_field tu2_fields[2];
extern struct test_event tu2_event_desc;

SIDE_PTR_REL_DEFINE(tu2_off_provider, tu2_event_desc, struct test_event,
	provider, tu2_provider)
SIDE_PTR_REL_DEFINE(tu2_off_fields, tu2_event_desc, struct test_event,
	fields, tu2_fields)
SIDE_PTR_REL_DEFINE_AT(tu2_off_f0, tu2_fields, TEST_FIELD_NAME_AT(0), tu2_f0_name)
SIDE_PTR_REL_DEFINE_AT(tu2_off_f1, tu2_fields, TEST_FIELD_NAME_AT(1), tu2_f1_name)

TEST_DESC struct test_field tu2_fields[2] = {
	{ .name = SIDE_PTR_REL_INIT(tu2_off_f0), .type = 10 },
	{ .name = SIDE_PTR_REL_INIT(tu2_off_f1), .type = 20 },
};

TEST_DESC struct test_event tu2_event_desc = {
	.provider = SIDE_PTR_REL_INIT(tu2_off_provider),
	.fields = SIDE_PTR_REL_INIT(tu2_off_fields),
	.nr_fields = 2,
};

const struct test_event *tu2_event(void)
{
	return &tu2_event_desc;
}
