// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

#ifndef SIDE_TESTS_REL_PTR_H
#define SIDE_TESTS_REL_PTR_H

#include <side/macros.h>
#include <stddef.h>
#include <stdint.h>

/* A description shaped like a side one: an event pointing at its fields. */
struct test_field {
	side_ptr_rel_t(char) name;
	int32_t type;
	int32_t padding;
};

struct test_event {
	side_ptr_rel_t(char) provider;
	side_ptr_rel_t(struct test_field) fields;
	int32_t nr_fields;
	int32_t padding;
};

/* Where the name of element _i of a field array sits, in bytes. */
#define TEST_FIELD_NAME_AT(_i)						\
	((_i) * sizeof(struct test_field) + offsetof(struct test_field, name))

/* Defined by rel-ptr-tu2.c. */
const struct test_event *tu2_event(void);

#endif /* SIDE_TESTS_REL_PTR_H */
