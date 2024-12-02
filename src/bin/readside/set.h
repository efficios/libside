// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_SET_H
#define READSIDE_SET_H

typedef struct set *set_t;

/**
 * Make a set with (1 << LEN_POW) buckets.  If LEN_POW is zero, then a default
 * size is selected instead.
 */
extern set_t make_set(size_t len_pow);

/**
 * Drop a set.
 */
extern void drop_set(set_t set);

/**
 * Add PATH to SET.  If PATH is already in SET, then return false.  Otherwise,
 * return true.
 */
extern bool add_to_set(set_t set, const char *path);

#endif	/* READSIDE_SET_H */
