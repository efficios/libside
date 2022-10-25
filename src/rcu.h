// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <sched.h>
#include <stdint.h>
#include <pthread.h>
#include <poll.h>

#define SIDE_CACHE_LINE_SIZE		256
#define SIDE_RCU_PERCPU_ARRAY_SIZE	2

struct side_rcu_percpu_count {
	uintptr_t begin;
	uintptr_t end;
}  __attribute__((__aligned__(SIDE_CACHE_LINE_SIZE)));

struct side_rcu_cpu_gp_state {
	struct side_rcu_percpu_count count[SIDE_RCU_PERCPU_ARRAY_SIZE];
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
	 * This acquire MO pairs with the release fence at the end of
	 * side_rcu_wait_grace_period().
	 */
	(void) __atomic_add_fetch(&gp_state->percpu_state[cpu].count[period].begin, 1, __ATOMIC_ACQUIRE);
	return period;
}

static inline
void side_rcu_read_end(struct side_rcu_gp_state *gp_state, unsigned int period)
{
	int cpu = sched_getcpu();

	if (cpu < 0)
		cpu = 0;
	/*
	 * This release MO pairs with the acquire fence at the beginning
	 * of side_rcu_wait_grace_period().
	 */
	(void) __atomic_add_fetch(&gp_state->percpu_state[cpu].count[period].end, 1, __ATOMIC_RELEASE);
}

static inline
void wait_for_cpus(struct side_rcu_gp_state *gp_state)
{
	unsigned int prev_period = 1 - gp_state->period;

	/*
	 * Wait for the sum of CPU begin/end counts to match for the
	 * previous period.
	 */
	for (;;) {
		uintptr_t sum = 0;	/* begin - end */
		int i;

		for (i = 0; i < gp_state->nr_cpus; i++) {
			struct side_rcu_cpu_gp_state *cpu_state = &gp_state->percpu_state[i];

			sum -= __atomic_load_n(&cpu_state->count[prev_period].end, __ATOMIC_RELAXED);
		}

		/*
		 * Read end counts before begin counts. Reading end
		 * before begin count ensures we never see an end
		 * without having seen its associated begin, in case of
		 * a thread migration during the traversal over each
		 * cpu.
		 */
		__atomic_thread_fence(__ATOMIC_ACQ_REL);

		for (i = 0; i < gp_state->nr_cpus; i++) {
			struct side_rcu_cpu_gp_state *cpu_state = &gp_state->percpu_state[i];

			sum += __atomic_load_n(&cpu_state->count[prev_period].begin, __ATOMIC_RELAXED);
		}
		if (!sum) {
			break;
		} else {
			/* Retry after 10ms. */
			poll(NULL, 0, 10);
		}
	}
}

static inline
void side_rcu_wait_grace_period(struct side_rcu_gp_state *gp_state)
{
	/*
	 * This release fence pairs with the acquire MO __atomic_add_fetch
	 * in side_rcu_read_begin().
	 */
	__atomic_thread_fence(__ATOMIC_RELEASE);

	pthread_mutex_lock(&gp_state->gp_lock);

	wait_for_cpus(gp_state);

	/* Flip period: 0 -> 1, 1 -> 0. */
	(void) __atomic_xor_fetch(&gp_state->period, 1, __ATOMIC_RELAXED);

	wait_for_cpus(gp_state);

	pthread_mutex_unlock(&gp_state->gp_lock);

	/*
	 * This acquire fence pairs with the release MO __atomic_add_fetch
	 * in side_rcu_read_end().
	 */
	__atomic_thread_fence(__ATOMIC_ACQUIRE);
}
