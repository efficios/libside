// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _SIDE_COMPILER_H
#define _SIDE_COMPILER_H

#if defined(__x86_64__) || defined(__i386__)
static inline
void side_cpu_relax(void)
{
	asm volatile ("rep; nop" : : : "memory");
}
#else
//TODO: implement per-architecture busy loop "pause"/"rep; nop;" instruction.
static inline
void side_cpu_relax(void)
{
	asm volatile ("" : : : "memory");
}
#endif

#endif /* _SIDE_COMPILER_H */
