// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <sched.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <poll.h>

#define SIDE_CACHE_LINE_SIZE		256

struct side_rcu_percpu_count {
	uintptr_t begin;
	uintptr_t end;
}  __attribute__((__aligned__(SIDE_CACHE_LINE_SIZE)));

struct side_rcu_cpu_gp_state {
	struct side_rcu_percpu_count count[2];
};

struct side_rcu_gp_state {
	struct side_rcu_cpu_gp_state *percpu_state;
	int nr_cpus;
	unsigned int period;
	pthread_mutex_t gp_lock;
};

//TODO: replace atomics by rseq (when available)
//TODO: replace acquire/release by membarrier+compiler barrier (when available)
//TODO: implement wait/wakeup for grace period using sys_futex
static inline
unsigned int side_rcu_read_begin(struct side_rcu_gp_state *gp_state)
{
	int cpu = sched_getcpu();
	unsigned int period = __atomic_load_n(&gp_state->period, __ATOMIC_RELAXED);

	if (cpu < 0)
		cpu = 0;
	/*
	 * This memory barrier (A) ensures that the contents of the
	 * read-side critical section does not leak before the "begin"
	 * counter increment. It pairs with memory barriers (D) and (E).
	 *
	 * This memory barrier (A) also ensures that the "begin"
	 * increment is before the "end" increment. It pairs with memory
	 * barrier (C). It is redundant with memory barrier (B) for that
	 * purpose.
	 */
	(void) __atomic_add_fetch(&gp_state->percpu_state[cpu].count[period].begin, 1, __ATOMIC_SEQ_CST);
	return period;
}

static inline
void side_rcu_read_end(struct side_rcu_gp_state *gp_state, unsigned int period)
{
	int cpu = sched_getcpu();

	if (cpu < 0)
		cpu = 0;
	/*
	 * This memory barrier (B) ensures that the contents of the
	 * read-side critical section does not leak after the "end"
	 * counter increment. It pairs with memory barriers (D) and (E).
	 *
	 * This memory barrier (B) also ensures that the "begin"
	 * increment is before the "end" increment. It pairs with memory
	 * barrier (C). It is redundant with memory barrier (A) for that
	 * purpose.
	 */
	(void) __atomic_add_fetch(&gp_state->percpu_state[cpu].count[period].end, 1, __ATOMIC_SEQ_CST);
}

#define side_rcu_dereference(p) \
	__extension__ \
	({ \
		(__typeof__(p) _____side_v = __atomic_load_n(&(p), __ATOMIC_CONSUME); \
		(_____side_v); \
	})

#define side_rcu_assign_pointer(p, v)	__atomic_store_n(&(p), v, __ATOMIC_RELEASE); \

/* active_readers is an input/output parameter. */
static inline
void check_active_readers(struct side_rcu_gp_state *gp_state, bool *active_readers)
{
	uintptr_t sum[2] = { 0, 0 };	/* begin - end */
	int i;

	for (i = 0; i < gp_state->nr_cpus; i++) {
		struct side_rcu_cpu_gp_state *cpu_state = &gp_state->percpu_state[i];

		sum[0] -= __atomic_load_n(&cpu_state->count[0].end, __ATOMIC_RELAXED);
		sum[1] -= __atomic_load_n(&cpu_state->count[1].end, __ATOMIC_RELAXED);
	}

	/*
	 * This memory barrier (C) pairs with either of memory barriers
	 * (A) or (B) (one is sufficient).
	 *
	 * Read end counts before begin counts. Reading "end" before
	 * "begin" counts ensures we never see an "end" without having
	 * seen its associated "begin", because "begin" is always
	 * incremented before "end", as guaranteed by memory barriers
	 * (A) or (B).
	 */
	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	for (i = 0; i < gp_state->nr_cpus; i++) {
		struct side_rcu_cpu_gp_state *cpu_state = &gp_state->percpu_state[i];

		sum[0] += __atomic_load_n(&cpu_state->count[0].begin, __ATOMIC_RELAXED);
		sum[1] += __atomic_load_n(&cpu_state->count[1].begin, __ATOMIC_RELAXED);
	}
	if (active_readers[0])
		active_readers[0] = sum[0];
	if (active_readers[1])
		active_readers[1] = sum[1];
}

/*
 * Wait for previous period to have no active readers.
 *
 * active_readers is an input/output parameter.
 */
static inline
void wait_for_prev_period_readers(struct side_rcu_gp_state *gp_state, bool *active_readers)
{
	unsigned int prev_period = gp_state->period ^ 1;

	/*
	 * If a prior active readers scan already observed that no
	 * readers are present for the previous period, there is no need
	 * to scan again.
	 */
	if (!active_readers[prev_period])
		return;
	/*
	 * Wait for the sum of CPU begin/end counts to match for the
	 * previous period.
	 */
	for (;;) {
		check_active_readers(gp_state, active_readers);
		if (!active_readers[prev_period])
			break;
		/* Retry after 10ms. */
		poll(NULL, 0, 10);
	}
}

/*
 * The grace period completes when it observes that there are no active
 * readers within each of the periods.
 *
 * The active_readers state is initially true for each period, until the
 * grace period observes that no readers are present for each given
 * period, at which point the active_readers state becomes false.
 */
static inline
void side_rcu_wait_grace_period(struct side_rcu_gp_state *gp_state)
{
	bool active_readers[2] = { true, true };

	/*
	 * This memory barrier (D) pairs with memory barriers (A) and
	 * (B) on the read-side.
	 */
	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	/*
	 * First scan through all cpus, for both period. If no readers
	 * are accounted for, we have observed quiescence and can
	 * complete the grace period immediately.
	 */
	check_active_readers(gp_state, active_readers);
	if (!active_readers[0] && !active_readers[1])
		goto end;

	pthread_mutex_lock(&gp_state->gp_lock);

	wait_for_prev_period_readers(gp_state, active_readers);
	/*
	 * If the reader scan detected that there are no readers in the
	 * current period as well, we can complete the grace period
	 * immediately.
	 */
	if (!active_readers[gp_state->period])
		goto unlock;

	/* Flip period: 0 -> 1, 1 -> 0. */
	(void) __atomic_xor_fetch(&gp_state->period, 1, __ATOMIC_RELAXED);

	wait_for_prev_period_readers(gp_state, active_readers);
unlock:
	pthread_mutex_unlock(&gp_state->gp_lock);
end:
	/*
	 * This memory barrier (E) pairs with memory barriers (A) and
	 * (B) on the read-side.
	 */
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
}
