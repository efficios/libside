// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

/*
 * Data structure for set of strings.  Used for remembering the set of visited
 * paths.
 *
 * Operations:
 *
 *   - make
 *   - drop
 *   - add
 *
 * Removal of strings is not possible.
 *
 * A bloom filter BF(m=BF_M, k=BF_K) is created for a set.  The probability of a
 * false positive on lookup is given by the following formula:
 *
 *	ɛ = (1 - (1 - m^-1)^kn)^k
 *
 * where `n' is the number of items in the set.
 *
 * Given this, the false positive rate is 1% at around 6228 elements for
 * BF(65536, 4).
 *
 * Debug:
 *
 * If the loglevel is at DEBUG, then statistics are printed on stderr when a set
 * is dropped.
 */

#include <limits.h>

#include "hash.h"
#include "set.h"

#define BF_K  4ul
#define BF_M  65536ul

static_assert(BF_M % CHAR_BIT == 0,
	"Parameter `m' of bloom filter must be a multiple of CHAR_BIT.");

#define BF_SIZE (BF_M / CHAR_BIT)

/*
 * Each function is a 32-bit value.  The key is the concatenation of all
 * functions.
 */
typedef u32 set_key_t[BF_K];

struct set_node {
	set_key_t key;
	char *path;
	struct set_node *next;
};

struct set {
	u8 bloom_filter[BF_SIZE];
	struct {
		u64 alloc_size;
		u64 element_count;
		u64 lookup_count;
		u64 bf_true_negative;
		u64 bf_true_positive;
		u64 bf_false_positive;
	} stat;
	size_t length;
	struct set_node *heads[];
};

static inline bool key_eq(const set_key_t a, const set_key_t b)
{
	return 0 == memcmp(a, b, sizeof(set_key_t));
}

set_t make_set(size_t len_pow)
{
	struct set *set;
	size_t length;
	size_t size;

	/*
	 * Default bucket count is 1 << 9.
	 * Maximum bucket count is 1 << 16.
	 */
	if (0 == len_pow) {
		len_pow = 9;
	} else if (len_pow > 16){
		len_pow = 16;
	}

	/*
	 * Ensure that the number of bucket is a power of 2.  This allows for
	 * fast modulo on keys to find corresponding buckets.
	 */
	length = 1ul << len_pow;
	assert_pow2(length);

	size = sizeof(*set) + length * sizeof(struct set_node *);

	set = xcalloc(1, size);

	set->stat.alloc_size = size;
	set->length = length;

	return set;
}

void drop_set(set_t set)
{
	bool debug_dump = current_loglevel >= LOGLEVEL_DEBUG;

	for (size_t k = 0; k < set->length; ++k) {
		for (struct set_node *node = set->heads[k]; node;) {
			struct set_node *tmp = node;

			if (debug_dump){
				set->stat.alloc_size += (sizeof(struct set_node) +
							strlen(node->path) + 1);
			}

			node = node->next;
			xfree(tmp->path);
			xfree(tmp);
		}
	}

	if (debug_dump) {
		debug("set statistics:\n"
			"\tAllocated bytes: %zu\n"
			"\tElement:         %" PRIu64 "\n"
			"\tBytes/element:   %.4f\n"
			"\tLookup:          %" PRIu64 "\n"
			"\tBF(m=%zu, k=%zu):\n"
			"\t\ttrue negative:  %.4f\n"
			"\t\ttrue positive:  %.4f\n"
			"\t\tfalse positive: %.4f\n"
			"\n",
			set->stat.alloc_size,
			set->stat.element_count,
			cast(double, set->stat.alloc_size) / set->stat.element_count,
			set->stat.lookup_count,
			BF_M, BF_K,
			cast(double, set->stat.bf_true_negative) / set->stat.lookup_count,
			cast(double, set->stat.bf_true_positive) / set->stat.lookup_count,
			cast(double, set->stat.bf_false_positive) / set->stat.lookup_count);
	}

	xfree(set);
}

/*
 * FIXME: Is the sum of sub-keys really optimal here for the distribution?
 */
static inline u64 key_index(const set_t set, const set_key_t key)
{
	u64 acc = 0;
	for (size_t k = 0; k < BF_K; ++k) {
		acc += key[k];
	}
	return fast_modpow2(acc, set->length);
}

static inline struct set_node *key_head(set_t set, const set_key_t key)
{
	return set->heads[key_index(set, key)];
}

static void hash_path(const char *path, set_key_t key)
{
	size_t len = strlen(path);
	for (size_t k = 0; k < BF_K; ++k) {
		MurmurHash3_generic_32(path, len, k, &key[k]);
	}
}

static inline void add_bloom_filter(u8 filter[BF_SIZE], const set_key_t key)
{
	for (size_t k = 0; k < BF_K; ++k) {

		u32 n = key[k] % BF_M;
		u32 j = n / 8;
		u32 i = n % 8;

		filter[j] |= (1ul << i);
	}
}

static bool in_bloom_filter(const u8 filter[BF_SIZE], const set_key_t key)
{
	for (size_t k = 0; k < BF_K; ++k) {

		u32 n = key[k] % BF_M;
		u32 j = n / 8;
		u32 i = n % 8;

		if (!(filter[j] & (1ul << i))) {
			return false;
		}
	}

	return true;
}

static bool path_in_set(set_t set, const set_key_t key, const char *path)
{
	set->stat.lookup_count++;

	if (!in_bloom_filter(set->bloom_filter, key)) {
		set->stat.bf_true_negative++;
		return false;
	}

	for (struct set_node *node = key_head(set, key); node; node = node->next) {
		if (key_eq(node->key, key) && streq(node->path, path)) {
			set->stat.bf_true_positive++;
			return true;
		}
	}

	set->stat.bf_false_positive++;

	return false;
}

static void insert_path_in_set(set_t set, const set_key_t key, const char *path)
{
	struct set_node *new_head;

	new_head = xcalloc(1, sizeof(struct set_node));

	memcpy(&new_head->key, key, sizeof(set_key_t));
	new_head->next = key_head(set, key);
	new_head->path = xstrdup(path);

	set->heads[key_index(set, key)] = new_head;

	add_bloom_filter(set->bloom_filter, key);

	set->stat.element_count++;
}

bool add_to_set(set_t set, const char *path)
{
	set_key_t key;

	hash_path(path, key);

	if (path_in_set(set, key, path)) {
		return false;
	}

	insert_path_in_set(set, key, path);

	return true;
}
