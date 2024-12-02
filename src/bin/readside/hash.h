//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

/* See NOTEs further ahead about changes made to this file. */

#ifndef READSIDE_HASH_H
#define READSIDE_HASH_H

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio

#if defined(_MSC_VER) && (_MSC_VER < 1600)

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

// Other compilers

#else	// defined(_MSC_VER)

#include <stdint.h>

#endif // !defined(_MSC_VER)

//-----------------------------------------------------------------------------

void MurmurHash3_x86_32  ( const void * key, int len, uint32_t seed, void * out );

void MurmurHash3_x86_128 ( const void * key, int len, uint32_t seed, void * out );

void MurmurHash3_x64_128 ( const void * key, int len, uint32_t seed, void * out );

/*
 * NOTE: Although this code is optimized for the x86 and x86-64 architectures,
 * it is used generically.
 */
static inline void MurmurHash3_generic_32(const void *key, int len, uint32_t seed, void *out)
{
	MurmurHash3_x86_32(key, len, seed, out);
}

static inline void MurmurHash3_generic_128(const void *key, int len, uint32_t seed, void *out)
{
#if (defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64))
	MurmurHash3_x64_128(key, len, seed, out);
#elif (defined(__i386__) || defined(__i386))
	MurmurHash3_x86_128(key, len, seed, out);
#else
	MurmurHash3_x64_128(key, len, seed, out);
#endif
}

//-----------------------------------------------------------------------------

#endif /* READSIDE_HASH_H */
