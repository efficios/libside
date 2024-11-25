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

/* Stringify X after expansion. */
#define SIDE_STR_PRIMITIVE(x...) #x
#define SIDE_STR(x...) SIDE_STR_PRIMITIVE(x)

/* Concatenate X with Y after expansion. */
#define SIDE_CAT_PRIMITIVE(x, y...) x ## y
#define SIDE_CAT(x, y...) SIDE_CAT_PRIMITIVE(x, y)

/* Same as SIDE_CAT, but can expand SIDE_CAT within the expansion itself. */
#define SIDE_CAT2_PRIMITIVE(x, y...) x ## y
#define SIDE_CAT2(x, y...) SIDE_CAT2_PRIMITIVE(x, y)

/* Expect a trailing semicolon. */
#define SIDE_EXPECT_SEMICOLON(...) side_static_assert(1, "", _)

/*
 * The diagnostic macros can be used to turn-off warnings using inline _Pragma.
 */
#if defined(__clang__)

#  define SIDE_DIAGNOSTIC(x)			\
	_Pragma(SIDE_STR(clang diagnostic x))

#elif defined(__GNUC__)

#  define SIDE_DIAGNOSTIC(x)			\
	_Pragma(SIDE_STR(GCC diagnostic x))

#endif

#ifdef __cplusplus
#  define SIDE_DIAGNOSTIC_C(...)
#  define SIDE_DIAGNOSTIC_CXX SIDE_DIAGNOSTIC
#else
#  define SIDE_DIAGNOSTIC_C SIDE_DIAGNOSTIC
#  define SIDE_DIAGNOSTIC_CXX(...)
#endif

#define SIDE_PUSH_DIAGNOSTIC()						\
	SIDE_DIAGNOSTIC(push)						\
	SIDE_DIAGNOSTIC(ignored "-Wpragmas")

#define SIDE_POP_DIAGNOSTIC()			\
    SIDE_DIAGNOSTIC(pop)

/*
 * Define a unique identifier in the compilation unit.
 */
#define SIDE_MAKE_ID(prefix)					\
    SIDE_CAT(libside_gensym_, SIDE_CAT(prefix, __COUNTER__))

/*
 * Private helpers for C++.
 *
 * Class libside::stack_copy<T>: A wrapper around std::initializer_list<T>.  It
 * copies the values in the intializer list in a buffer allocated on the stack.
 * The rationale is the following:
 *
 *   Dynamic compound literals are used in function scopes, i.e., the storage is
 *   on the stack. C++ does not support them well.
 *
 *   To overcome this issue, the compound literals are memcpy onto an allocated
 *   buffer on the stack using alloca(3).
 *
 *   See the following paragraphs taken from GCC documentation for the rationale:
 *
 *     In C, a compound literal designates an unnamed object with static or
 *     automatic storage duration. In C++, a compound literal designates a
 *     temporary object that only lives until the end of its full-expression. As
 *     a result, well-defined C code that takes the address of a subobject of a
 *     compound literal can be undefined in C++, so G++ rejects the conversion
 *     of a temporary array to a pointer. For instance, if the array compound
 *     literal example above appeared inside a function, any subsequent use of
 *     foo in C++ would have undefined behavior because the lifetime of the
 *     array ends after the declaration of foo.
 *
 *     As an optimization, G++ sometimes gives array compound literals longer
 *     lifetimes: when the array either appears outside a function or has a
 *     const-qualified type. If foo and its initializer had elements of type
 *     char *const rather than char* , or if foo were a global variable, the
 *     array would have static storage duration. But it is probably safest just
 *     to avoid the use of array compound literals in C++ code.
 *
 * Function libside::initializer_list_size<T>: Return the number of elements in
 * an initializer list.  This can be used either at compile time or at runtime.
 * The rationale for its usage is the same as libside::stack_copy<T>, only it
 * allows finding the number of the elements allocated in the stack buffer.
 * Furthermore, there is a bug in GCC 13 where taking the address of a temporary
 * address in a sizeof operator triggers a compile error.  This work around
 * does not seems to be affected.
 */
#ifdef __cplusplus
#  include <type_traits>
#  include <initializer_list>
namespace libside {

	template <typename T>
	T *stack_copy(T *data, std::initializer_list<T> init) {
		size_t size = init.size();
		if (size) {
			size_t k = 0;
			for (const T v : init) {
				__builtin_memcpy((void*)&data[k++], (void*)&v, sizeof(T));
			}
		}
		return data;
	};

	template <typename T>
	uint32_t initializer_list_size(std::initializer_list<T> init)
	{
		return init.size();
	}
};
#endif

/*
 * Compound literals with static storage are needed by SIDE
 * instrumentation.
 * Compound literals are part of the C99 and C11 standards, but not
 * part of the C++ standards. They are supported by most C++ compilers
 * though.
 *
 * However, there is a compiler bug in GCC 13, where taking the address of a
 * temporary address triggers a compile time error, even within a sizeof
 * operator.  See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117817>.
 *
 * For this major version of GCC, or if the configurable macro
 * `SIDE_ALLOCATE_COMPOUND_LITERAL_ON_HEAP' is defined, then compound literal
 * are created using the `new' operator and their size is determined by using
 * the libside::initializer_list_size<T> wrapper, which seems unaffected by the
 * bug.
 *
 * FIXME: Add cutoff for this work around whenever the bug if fixed upstream.
 * Currently, the fix is targeted for 13.4.
 *
 * NOTE: Allocating compound literals with the heap assumes that this is done
 * for static variables.  These are leaked and will trigger false positives for
 * tools like Valgrind and Address Sanitizer.
 *
 * Example use:
 * static struct mystruct *var = SIDE_COMPOUND_LITERAL(struct mystruct, { 1, 2, 3 });
 */
#if defined(__cplusplus) && (defined(SIDE_ALLOCATE_COMPOUND_LITERAL_ON_HEAP) || (defined(__GNUC__) && (__GNUC__ == 13)))
#  define SIDE_COMPOUND_LITERAL(type, ...)  new (type[]) { __VA_ARGS__ }

#  define SIDE_LITERAL_ARRAY(_type, ...)				\
	{								\
		SIDE_PTR_INIT(SIDE_COMPOUND_LITERAL(_type, ##__VA_ARGS__)),  \
		libside::initializer_list_size<_type>({ __VA_ARGS__ }),	\
	}
#else
#  define SIDE_COMPOUND_LITERAL(type, ...)   (type[]) { __VA_ARGS__ }

#  define SIDE_LITERAL_ARRAY(_type, ...)				\
	{								\
		SIDE_PTR_INIT(SIDE_COMPOUND_LITERAL(_type, ##__VA_ARGS__)),  \
		SIDE_ARRAY_SIZE(SIDE_COMPOUND_LITERAL(_type, ##__VA_ARGS__)), \
	}
#endif

/*
 * Dynamic compound literals in C are the same as the static ones.  For C++, the
 * values are copied from a std::initializer_list onto a buffer allocated on the
 * stack.
 *
 * NOTE: For C++, the allocation on the stack is made with alloca(3), resulting
 * in increase of the stack size for every dynamic literal created within a
 * function.  If such creation is done within a loop, the behavior is undefined.
 * Currently, dynamic literals are only used for dynamic attribute lists, which
 * are used by all dynamic arguments.  Users should avoid using dynamic
 * arguments in loops.  To overcome this limit, users can call a helper noinline
 * function that does the instrumentation.
 */
#ifdef __cplusplus
#  define SIDE_DYNAMIC_LITERAL_ARRAY(_type, ...)			\
	{								\
		SIDE_PTR_INIT(libside::stack_copy<std::remove_const<_type>::type>((std::remove_const<_type>::type *)__builtin_alloca(sizeof(_type) * libside::initializer_list_size<_type>({__VA_ARGS__})), { __VA_ARGS__ })), \
		libside::initializer_list_size<_type>({__VA_ARGS__}),		\
	}
#else
#  define SIDE_DYNAMIC_LITERAL_ARRAY SIDE_LITERAL_ARRAY
#endif

/*
 * Define an array of type `_type'.  The rationale is to wrap an array pointer
 * and its associated length in a single structure.  This standardizes array
 * elements and length accesses.
 *
 * Example:
 *
 * struct bytevector {
 * 	side_array_t(uint8_t) bytes;
 * };
 *
 * struct bytevector my_bytevector = {
 * 	.bytes = SIDE_LITERAL_ARRAY(uint8_t, 16, 32, 64);
 * };
 */
#define side_array_t(_type)                                             \
	struct {							\
		side_ptr_t(_type) elements;				\
		uint32_t length;					\
	} SIDE_PACKED

/*
 * Return a pointer to the first element in `_array'.
 *
 * Example:
 *
 * printf("%u\n", *side_array_elements(&my_bytevector->bytes, 0));
 * | 16
 */
#define side_array_elements(_array) side_ptr_get((_array)->elements)

/*
 * Return a pointer to the element at index `_k' in `_array'.
 *
 * No bounds checking is done.
 *
 * Example:
 *
 * printf("%u\n", *side_array_at(&my_bytevector->bytes, 1));
 * | 32
 */
#define side_array_at(_array, _k) &side_array_elements(_array)[_k]

/*
 * Return the length of `_array'.
 *
 * Example:
 *
 * printf("%zu\n", side_array_length(&my_bytevector->bytes));
 * | 3
 */
#define side_array_length(_array) (_array)->length

/*
 * Iterate over the elements of `_array'.  For each iteration, the variable
 * `_it' points to a new element.
 *
 * Do not assume traversal order.  Use `side_array_index_of_element()' to
 * determine the index of `_it' within `_array'.
 *
 * Example:
 *
 * uint8_t *byte;
 * side_for_each_element_in_array(byte, &my_bytevector->bytes) {
 * 	printf("%u\n", *byte);
 * }
 * | 16
 * | 32
 * | 64
 */
#define side_for_each_element_in_array(_it, _array)                     \
    for ((_it) = side_array_elements(_array);                           \
         ((_it) - side_array_elements(_array)) < side_array_length(_array); \
         (_it)++)

/*
 * Return the index of element `_it' in `_array'.
 *
 * No bounds checking is done.
 *
 * Example:
 *
 * printf("%zu\n",
 * 	side_array_index_of_element(&my_bytevector->bytes,
 * 				side_array_at(&my_bytevector->bytes, 2)));
 * | 2
 */
#define side_array_index_of_element(_array, _it)	\
	((_it) - side_array_elements(_array))

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
			(__VA_ARGS__),				\
			0,					\
			0,					\
			0,					\
		},						\
	}
# else
#  define SIDE_PTR_INIT(...)					\
	{							\
		.v = {						\
			0,					\
			0,					\
			0,					\
			(__VA_ARGS__),				\
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
			(__VA_ARGS__),				\
			0,					\
		},						\
	}
# else
#  define SIDE_PTR_INIT(...)					\
	{							\
		.v = {						\
			 0,					\
			(__VA_ARGS__),				\
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
			(__VA_ARGS__),				\
		},						\
	}
#else
# error "Unsupported pointer size"
#endif

#define side_ptr_t(_type)	side_raw_ptr_t(_type *)
#define side_func_ptr_t(_type)	side_raw_ptr_t(_type)

/*
 * In C++, it is not possible to declare types in expressions within a sizeof.
 */
#ifdef __cplusplus
namespace side {
	namespace macros {
		using side_ptr_t_int = side_ptr_t(int);
		side_static_assert(sizeof(side_ptr_t_int) == 16,
				"Unexpected size for side_ptr_t",
				unexpected_size_side_ptr_t);
	};
};
#else
side_static_assert(sizeof(side_ptr_t(int)) == 16,
	"Unexpected size for side_ptr_t",
	unexpected_size_side_ptr_t);
#endif

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
