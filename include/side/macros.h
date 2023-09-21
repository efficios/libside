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

/*
 * The side_ptr macros allow defining a pointer type which is suitable
 * for use by 32-bit and 64-bit kernels without compatibility code,
 * while preserving information about the pointer type.
 *
 * Those pointers are stored as 64-bit integers, and the type of the
 * actual pointer is kept alongside with the 64-bit pointer value in a
 * 0-len array within a union.
 *
 * uintptr_t will fit within a uint64_t except on architectures with
 * 128-bit pointers. This provides fixed-size pointers on architectures
 * with pointer size of 64-bit or less. Architectures with larger
 * pointer size will have to handle the ABI offset specifics explicitly.
 */
#if (__SIZEOF_POINTER__ <= 8)
# define side_ptr_t(_type)					\
	union {							\
		uint64_t v;					\
		_type *t[0];					\
	}
# define side_ptr_get(_field)					\
	((__typeof__((_field).t[0]))(uintptr_t)(_field).v)
# define side_ptr_set(_field, _ptr)				\
	do {							\
		(_field).v = (uint64_t)(uintptr_t)(_ptr);	\
	} while (0)
#else
# define side_ptr_t(_type)					\
	union {							\
		uintptr_t v;					\
		_type *t[0];					\
	}
# define side_ptr_get(_field)					\
	((__typeof__((_field).t[0]))(_field).v)
# define side_ptr_set(_field, _ptr)				\
	do {							\
		(_field).v = (uintptr_t)(_ptr);			\
	} while (0)
#endif

#define SIDE_PTR_INIT(...)	{ .v = (uintptr_t) (__VA_ARGS__) }

#endif /* _SIDE_MACROS_H */
