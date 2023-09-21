// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _SIDE_MACROS_H
#define _SIDE_MACROS_H

#include <stddef.h>
#include <limits.h>

/* Helper macros */

#define SIDE_ARRAY_SIZE(arr)	(sizeof(arr) / sizeof((arr)[0]))

/*
 * Compound literals with static storage are needed by SIDE
 * instrumentation.
 * Compound literals are part of the C99 and C11 standards, but not
 * part of the C++ standards. They are supported by most C++ compilers
 * though.
 *
 * Example use:
 * static struct mystruct *var = LTTNG_UST_COMPOUND_LITERAL(struct mystruct, { 1, 2, 3 });
 */
#define SIDE_COMPOUND_LITERAL(type, ...)   (type[]) { __VA_ARGS__ }

#define side_likely(x)		__builtin_expect(!!(x), 1)
#define side_unlikely(x)	__builtin_expect(!!(x), 0)

#define SIDE_PARAM(...)	__VA_ARGS__

/*
 * SIDE_PARAM_SELECT_ARG1
 *
 * Select second argument. Use inside macros to implement optional last
 * macro argument, such as:
 *
 * #define macro(_a, _b, _c, _optional...) \
 *     SIDE_PARAM_SELECT_ARG1(_, ##_optional, do_default_macro())
 *
 * This macro is far from pretty, but attempts to create a cleaner layer
 * on top fails for various reasons:
 *
 * - The libside API needs to use the default argument selection as an
 *   argument to itself (recursively), e.g. for fields and for types, so
 *   using the argument selection within an extra layer of macro fails
 *   because the extra layer cannot expand recursively.
 * - Attempts to make the extra layer of macro support recursion through
 *   another layer of macros which expands all arguments failed because
 *   the optional argument may contain commas, and is therefore expanded
 *   into multiple arguments before argument selection, which fails to
 *   select the optional argument content after its first comma.
 */
#define SIDE_PARAM_SELECT_ARG1(_arg0, _arg1, ...) _arg1

/*
 * side_container_of - Get the address of an object containing a field.
 *
 * @ptr: pointer to the field.
 * @type: type of the object.
 * @member: name of the field within the object.
 */
#define side_container_of(ptr, type, member)				\
	__extension__							\
	({								\
		const __typeof__(((type *) NULL)->member) * __ptr = (ptr); \
		(type *)((char *)__ptr - offsetof(type, member));	\
	})

#define side_struct_field_sizeof(_struct, _field) \
	sizeof(((_struct * )NULL)->_field)

#if defined(__SIZEOF_LONG__)
# define SIDE_BITS_PER_LONG	(__SIZEOF_LONG__ * 8)
#elif defined(_LP64)
# define SIDE_BITS_PER_LONG	64
#else
# define SIDE_BITS_PER_LONG	32
#endif

#define SIDE_PACKED	__attribute__((packed))

#endif /* _SIDE_MACROS_H */
