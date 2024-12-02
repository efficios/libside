// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_PANIC_H
#define READSIDE_PANIC_H

__attribute__((noreturn, format(printf, 4, 5)))
extern void do_panic(const char *file, int linum,
		const char *func_name,
		const char *fmt, ...);

#define panic(fmt, ...)							\
	do_panic(__FILE__, __LINE__, __PRETTY_FUNCTION__, fmt, ##__VA_ARGS__)

#endif	/* READSIDE_PANIC_H */
