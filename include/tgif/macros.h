// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _TGIF_MACROS_H
#define _TGIF_MACROS_H

#include <stddef.h>
#include <limits.h>

/* Helper macros */

#define TGIF_ARRAY_SIZE(arr)	(sizeof(arr) / sizeof((arr)[0]))

/*
 * Compound literals with static storage are needed by TGIF
 * instrumentation.
 * Compound literals are part of the C99 and C11 standards, but not
 * part of the C++ standards. They are supported by most C++ compilers
 * though.
 *
 * Example use:
 * static struct mystruct *var = LTTNG_UST_COMPOUND_LITERAL(struct mystruct, { 1, 2, 3 });
 */
#define TGIF_COMPOUND_LITERAL(type, ...)   (type[]) { __VA_ARGS__ }

#define tgif_likely(x)		__builtin_expect(!!(x), 1)
#define tgif_unlikely(x)	__builtin_expect(!!(x), 0)

#define TGIF_PARAM(...)	__VA_ARGS__

/*
 * tgif_container_of - Get the address of an object containing a field.
 *
 * @ptr: pointer to the field.
 * @type: type of the object.
 * @member: name of the field within the object.
 */
#define tgif_container_of(ptr, type, member)				\
	__extension__							\
	({								\
		const __typeof__(((type *) NULL)->member) * __ptr = (ptr); \
		(type *)((char *)__ptr - offsetof(type, member));	\
	})

#define tgif_struct_field_sizeof(_struct, _field) \
	sizeof(((_struct * )NULL)->_field)

#if defined(__SIZEOF_LONG__)
#define TGIF_BITS_PER_LONG	(__SIZEOF_LONG__ * 8)
#elif defined(_LP64)
#define TGIF_BITS_PER_LONG	64
#else
#define TGIF_BITS_PER_LONG	32
#endif

#define TGIF_PACKED	__attribute__((packed))

#endif /* _TGIF_MACROS_H */
