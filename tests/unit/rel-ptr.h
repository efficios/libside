// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

#ifndef SIDE_TESTS_REL_PTR_H
#define SIDE_TESTS_REL_PTR_H

#include <side/macros.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Something a description and a dynamic argument both write: the one
 * stores a distance, the other an address. See side_ptr_sel_t.
 */
struct test_attr {
	side_ptr_sel_t(const char) key;
	int32_t value;
};

/* A description shaped like a side one: an event pointing at its fields. */
struct test_field {
	side_ptr_rel_t(char) name;
	int32_t type;
	int32_t padding;
};

struct test_event {
	side_ptr_rel_t(char) provider;
	side_ptr_rel_t(struct test_field) fields;
	side_array_sel_t(const struct test_attr) attrs;
	int32_t nr_fields;
	int32_t padding;
};

/* Where the name of element _i of a field array sits, in bytes. */
#define TEST_FIELD_NAME_AT(_i)						\
	((_i) * sizeof(struct test_field) + offsetof(struct test_field, name))

/* Where the key of element _i of an attribute array sits, in bytes. */
#define TEST_ATTR_KEY_AT(_i)						\
	((_i) * sizeof(struct test_attr) + offsetof(struct test_attr, key))

/* Defined by rel-ptr-tu2.c. */
const struct test_event *tu2_event(void);

#endif /* SIDE_TESTS_REL_PTR_H */
