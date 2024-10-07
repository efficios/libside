// SPDX-License-Identifier: MIT
/*
 *
 * Copyright (C) 2012,2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

/*
 * This header defines the following endian macros based on the current
 * platform endian headers:
 *
 *   BYTE_ORDER         this macro shall have a value equal to one
 *                      of the *_ENDIAN macros in this header.
 *   FLOAT_WORD_ORDER   this macro shall have a value equal to one
 *                      of the *_ENDIAN macros in this header.
 *   LITTLE_ENDIAN      if BYTE_ORDER == LITTLE_ENDIAN, the host
 *                      byte order is from least significant to
 *                      most significant.
 *   BIG_ENDIAN         if BYTE_ORDER == BIG_ENDIAN, the host byte
 *                      order is from most significant to least
 *                      significant.
 *
 * Direct byte swapping interfaces:
 *
 *   uint16_t bswap_16(uint16_t x); (* swap bytes 16-bit word *)
 *   uint32_t bswap_32(uint32_t x); (* swap bytes 32-bit word *)
 *   uint64_t bswap_64(uint32_t x); (* swap bytes 64-bit word *)
 */

#ifndef _SIDE_ENDIAN_H
#define _SIDE_ENDIAN_H

#include <math.h>

#if defined(__SIZEOF_LONG__)
# define SIDE_BITS_PER_LONG	(__SIZEOF_LONG__ * 8)
#elif defined(_LP64)
# define SIDE_BITS_PER_LONG	64
#else
# define SIDE_BITS_PER_LONG	32
#endif

#if (defined(__linux__) || defined(__CYGWIN__))
#define side_bswap_16(x)		__builtin_bswap16(x)
#define side_bswap_32(x)		__builtin_bswap32(x)
#define side_bswap_64(x)		__builtin_bswap64(x)

#define SIDE_BYTE_ORDER			__BYTE_ORDER
#define SIDE_LITTLE_ENDIAN		__LITTLE_ENDIAN
#define SIDE_BIG_ENDIAN			__BIG_ENDIAN

#ifdef __FLOAT_WORD_ORDER
#define SIDE_FLOAT_WORD_ORDER		__FLOAT_WORD_ORDER
#else /* __FLOAT_WORD_ORDER */
#define SIDE_FLOAT_WORD_ORDER		__BYTE_ORDER
#endif /* __FLOAT_WORD_ORDER */

#elif defined(__FreeBSD__)

#include <sys/endian.h>

#define side_bswap_16(x)		bswap16(x)
#define side_bswap_32(x)		bswap32(x)
#define side_bswap_64(x)		bswap64(x)

#define SIDE_BYTE_ORDER			BYTE_ORDER
#define SIDE_LITTLE_ENDIAN		LITTLE_ENDIAN
#define SIDE_BIG_ENDIAN			BIG_ENDIAN
#define SIDE_FLOAT_WORD_ORDER		BYTE_ORDER

#else
#error "Please add support for your OS."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __HAVE_FLOAT128
static inline
void side_bswap_128p(char *p)
{
	int i;

	for (i = 0; i < 8; i++)
		p[i] = p[15 - i];
}
#endif

#ifdef __cplusplus
}
#endif

#if SIDE_BITS_PER_LONG == 64
# define side_bswap_pointer(x)	side_bswap_64(x)
#else
# define side_bswap_pointer(x)	side_bswap_32(x)
#endif

#endif /* _SIDE_ENDIAN_H */
