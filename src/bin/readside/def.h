// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

/*
 * The rationale of this file is to define common types, macros and inline
 * helpers that can be used by all compilation units without having to include
 * individual files.
 */

/*
 * Ensure GNU source.
 */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Types */
typedef size_t  usize;
typedef ssize_t isize;

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uintptr_t uptr;

typedef float  f32;
typedef double f64;

typedef unsigned char        uchar;
typedef unsigned int         uint;
typedef unsigned long int    ulong;

#ifndef NULL
#  define NULL ((void*)0)
#endif

/* Macros */
#define CAT_PRIMITIVE(X, Y) X ## Y
#define CAT(X, Y) CAT_PRIMITIVE(X, Y)

#define STR_PRIMITIVE(X...) #X
#define STR(X...) STR_PRIMITIVE(X)

#define MAKE_ID(PREFIX) CAT(gensym_, CAT(PREFIX, __COUNTER__))

#define alignof(x) _Alignof(x)

#ifndef offsetof
#  define offsetof(type, member) __builtin_offsetof(type, member)
#endif

#define container_of_primitive(var, ptr, type, member)		\
	({							\
		void *var = (void *)(ptr);			\
		((type *)(var - offsetof(type, member)));	\
	})

#define container_of(ptr, type, member)					\
	container_of_primitive(MAKE_ID(container_of), ptr, type, member)

#define array_size(arr)			(sizeof(arr) / sizeof((arr)[0]))
#define field_size(x, field)		(sizeof((typeof(x)*)0)->field)
#define field_sizeof(type, field)	(sizeof((type*)0)->field)
#define paddingof(A, B)			((alignof(A) - alignof(B)) % alignof(A))
#define typecheck(X, Y)			__builtin_types_compatible_p(X, Y)
#define cast(type, value...)		((type)(value))

#if __has_builtin(__builtin_expect)
#  define likely(x)   __builtin_expect(!!(x), 1)
#  define unlikely(x) __builtin_expect(!!(x), 0)
#else
#  define likely(x)   (x)
#  define unlikely(x) (x)
#endif

/* Local includes. */
#include "logging.h"
#include "panic.h"

/* Die */
#define die(...)					\
	do {						\
		error(__VA_ARGS__);			\
		exit(EXIT_FAILURE);			\
	} while (0)

#define out_of_memory()				\
	die("out of memory")

/* Utilities */
static inline bool streq(const char *a, const char *b)
{
	return 0 == strcmp(a, b);
}

/* Assertions */
#ifdef DEBUG
#  define weak_assertion strong_assertion
#else
#  define weak_assertion(...) do { } while (0)
#endif

#define strong_assertion(_cond...) if (!(_cond)) panic("Failed assertion: `" STR(_cond) "'")

/* Assert that N is a power of two. */
static inline bool assert_pow2(unsigned long N)
{
	(void) N;

	strong_assertion(N && !(N & (N - 1)));

	return true;
}

/*
 * Return the padding requires to add to X to be aligned on P.  P must be a power
 * of two.
 */
static inline uint64_t pow2_padding(uint64_t x, uint64_t p)
{
	assert_pow2(p);

	return (p - x) & (p - 1u);
}

/*
 * Fast modulo of X mod P, where P is a power of two.
 */
static inline uint64_t fast_modpow2(uint64_t x, uint64_t p)
{
	assert_pow2(p);

	return x & (p - 1u);
}

/*
 * Allocation wrappers for malloc family.
 *
 * Abort on any allocation returning NULL.
 *
 * Define malloc family functions to static assertion always failing to ensure
 * the wrappers are used everywhere.
 */

static inline void xfree(void *ptr)
{
	free(ptr);
}

#ifdef free
#  undef free
#endif
#define free(...) ({ static_assert(false, "Use xfree"); })

__attribute__((__malloc__)) __attribute__((__returns_nonnull__))
static inline void *xmalloc(size_t size)
{
	void *ptr;

	ptr = calloc(1, size);

	if (unlikely(NULL == ptr)) {
		out_of_memory();
	}

	return ptr;
}

#ifdef malloc
#  undef malloc
#endif
#define malloc(...) ({ static_assert(false, "Use xmalloc"); })

__attribute__((__malloc__))  __attribute__((__returns_nonnull__))
static inline void *xcalloc(size_t nmemb, size_t size)
{
	void *ptr;

	ptr = calloc(nmemb, size);

	if (unlikely(NULL == ptr)) {
		out_of_memory();
	}

	return ptr;
}

#ifdef calloc
#  undef calloc
#endif
#define calloc(...) ({ static_assert(false, "Use xcalloc"); })

__attribute__((__malloc__)) __attribute__((__returns_nonnull__))
static inline void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);

	if (unlikely(NULL == ptr)) {
		out_of_memory();
	}

	return ptr;
}

#ifdef realloc
#  undef realloc
#endif
#define realloc(...) ({ static_assert(false, "Use xrealloc"); })

__attribute__((__malloc__)) __attribute__((__returns_nonnull__))
static inline void *xmemalign(size_t alignment, size_t size)
{
	void *ptr;
	int err;

	err = posix_memalign(&ptr, alignment, size);

	if (unlikely(0 != err)) {
		out_of_memory();
	}

	return ptr;
}

#ifdef posix_memalign
#  undef posix_memalign
#endif
#define posix_memalign(...) ({ static_assert(false, "Use xmemalign"); })

/* Wrappers around strdup family. */

__attribute__((__returns_nonnull__))
static inline char *xstrndup(const char *s, size_t n)
{
	char *duplica = strndup(s, n);

	if (!duplica) {
		out_of_memory();
	}

	return duplica;
}

#ifdef strndup
#  undef strndup
#endif
#define strndup(...) ({ static_assert(false, "Use xstrndup"); })

__attribute__((__returns_nonnull__))
static inline char *xstrdup(const char *s)
{
	char *duplica = strdup(s);

	if (!duplica) {
		out_of_memory();
	}

	return duplica;
}

#ifdef strdup
#  undef strdup
#endif
#define strdup(...) ({ static_assert(false, "Use xstrdup"); })
