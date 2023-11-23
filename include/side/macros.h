// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _SIDE_MACROS_H
#define _SIDE_MACROS_H

#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <side/endian.h>

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

#define side_offsetofend(type, member) \
	(offsetof(type, member) + sizeof(((type *)0)->member))

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
 * Compile time assertion.
 * - predicate: boolean expression to evaluate,
 * - msg: string to print to the user on failure when `static_assert()` is
 *   supported,
 * - c_identifier_msg: message to be included in the typedef to emulate a
 *   static assertion. This parameter must be a valid C identifier as it will
 *   be used as a typedef name.
 */
#ifdef __cplusplus
#define side_static_assert(predicate, msg, c_identifier_msg)  \
	static_assert(predicate, msg)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define side_static_assert(predicate, msg, c_identifier_msg)  \
	_Static_assert(predicate, msg)
#else
/*
 * Evaluates the predicate and emit a compilation error on failure.
 *
 * If the predicate evaluates to true, this macro emits a function
 * prototype with an argument type which is an array of size 0.
 *
 * If the predicate evaluates to false, this macro emits a function
 * prototype with an argument type which is an array of negative size
 * which is invalid in C and forces a compiler error. The
 * c_identifier_msg parameter is used as the argument identifier so it
 * is printed to the user when the error is reported.
 */
#define side_static_assert(predicate, msg, c_identifier_msg)  \
	void side_static_assert_proto(char c_identifier_msg[2*!!(predicate)-1])
#endif

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

#define SIDE_PACKED	__attribute__((packed))

#define side_padding(bytes)	char padding[bytes]

#define side_check_size(_type, _len)				\
	side_static_assert(sizeof(_type) == (_len),		\
		"Unexpected size for type: `" #_type "`",	\
		unexpected_size_for_type_##_type)

/*
 * The side_ptr macros allow defining a pointer type which is suitable
 * for use by 32-bit, 64-bit and 128-bit kernels without compatibility
 * code, while preserving information about the pointer type.
 *
 * Those pointers are stored as 128-bit integers, and the type of the
 * actual pointer is kept alongside with the 128-bit pointer value in a
 * 0-len array within a union.
 */

#if (SIDE_BYTE_ORDER == SIDE_LITTLE_ENDIAN)
# define SIDE_U128_PTR_IDX(n)	(n)
#else
# define SIDE_U128_PTR_IDX(n)	((16 / __SIZEOF_POINTER__) - (n) - 1)
#endif

#define side_raw_ptr_t(_type)					\
	union {							\
		_type v[16 / __SIZEOF_POINTER__];		\
	}

/* side_ptr_get() can be used as rvalue or lvalue. */
#define side_ptr_get(_field)	(_field).v[SIDE_U128_PTR_IDX(0)]

#if (__SIZEOF_POINTER__ == 4)
# define side_ptr_set(_field, _ptr)				\
	do {							\
		(_field).v[SIDE_U128_PTR_IDX(0)] = (_ptr);	\
		(_field).v[SIDE_U128_PTR_IDX(1)] = 0;		\
		(_field).v[SIDE_U128_PTR_IDX(2)] = 0;		\
		(_field).v[SIDE_U128_PTR_IDX(3)] = 0;		\
	} while (0)

/* Keep the correct field init order to make old g++ happy. */
# if (SIDE_BYTE_ORDER == SIDE_LITTLE_ENDIAN)
#  define SIDE_PTR_INIT(...)					\
	{							\
		.v = {						\
			[0] = (__VA_ARGS__),			\
			[1] = 0,				\
			[2] = 0,				\
			[3] = 0,				\
		},						\
	}
# else
#  define SIDE_PTR_INIT(...)					\
	{							\
		.v = {						\
			[0] = 0,				\
			[1] = 0,				\
			[2] = 0,				\
			[3] = (__VA_ARGS__),			\
		},						\
	}
# endif
#elif (__SIZEOF_POINTER__ == 8)
# define side_ptr_set(_field, _ptr)				\
	do {							\
		(_field).v[SIDE_U128_PTR_IDX(0)] = (_ptr);	\
		(_field).v[SIDE_U128_PTR_IDX(1)] = 0;		\
	} while (0)

/* Keep the correct field init order to make old g++ happy. */
# if (SIDE_BYTE_ORDER == SIDE_LITTLE_ENDIAN)
#  define SIDE_PTR_INIT(...)					\
	{							\
		.v = {						\
			[0] = (__VA_ARGS__),			\
			[1] = 0,				\
		},						\
	}
# else
#  define SIDE_PTR_INIT(...)					\
	{							\
		.v = {						\
			[0] = 0,				\
			[1] = (__VA_ARGS__),			\
		},						\
	}
# endif
#elif (__SIZEOF_POINTER__ == 16)
# define side_ptr_set(_field, _ptr)				\
	do {							\
		(_field).v[SIDE_U128_PTR_IDX(0)] = (_ptr);	\
	} while (0)

# define SIDE_PTR_INIT(...)					\
	{							\
		.v = {						\
			[0] = (__VA_ARGS__),			\
		},						\
	}
#else
# error "Unsupported pointer size"
#endif

#define side_ptr_t(_type)	side_raw_ptr_t(_type *)
#define side_func_ptr_t(_type)	side_raw_ptr_t(_type)

side_static_assert(sizeof(side_ptr_t(int)) == 16,
	"Unexpected size for side_ptr_t",
	unexpected_size_side_ptr_t);

/*
 * side_enum_t allows defining fixed-sized enumerations while preserving
 * typing information.
 */
#define side_enum_t(_enum_type, _size_type)			\
	union {							\
		_size_type v;					\
		struct {					\
			_enum_type t[0];			\
		} SIDE_PACKED s;				\
	}

#define side_enum_get(_field)					\
	((__typeof__((_field).s.t[0]))(_field).v)

#define side_enum_set(_field, _v)				\
	do {							\
		(_field).v = (_v);				\
	} while (0)

#define SIDE_ENUM_INIT(...)	{ .v = (__VA_ARGS__) }

#endif /* _SIDE_MACROS_H */
