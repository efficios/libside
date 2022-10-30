// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _SIDE_RCU_H
#define _SIDE_RCU_H

#include <sched.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <poll.h>
#include <side/trace.h>
#include <rseq/rseq.h>

#define SIDE_CACHE_LINE_SIZE		256

struct side_rcu_percpu_count {
	uintptr_t begin;
	uintptr_t rseq_begin;
	uintptr_t end;
	uintptr_t rseq_end;
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

//TODO: implement wait/wakeup for grace period using sys_futex
static inline
unsigned int side_rcu_read_begin(struct side_rcu_gp_state *gp_state)
{
	unsigned int period = __atomic_load_n(&gp_state->period, __ATOMIC_RELAXED);
	struct side_rcu_cpu_gp_state *cpu_gp_state;
	int cpu;

	if (side_likely(rseq_offset > 0)) {
		cpu = rseq_cpu_start();
		cpu_gp_state = &gp_state->percpu_state[cpu];
		if (!rseq_addv((intptr_t *)&cpu_gp_state->count[period].rseq_begin, 1, cpu))
			goto fence;
	}
	cpu = sched_getcpu();
	if (side_unlikely(cpu < 0))
		cpu = 0;
	cpu_gp_state = &gp_state->percpu_state[cpu];
	(void) __atomic_add_fetch(&cpu_gp_state->count[period].begin, 1, __ATOMIC_RELAXED);
fence:
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
	rseq_barrier();
	return period;
}

static inline
void side_rcu_read_end(struct side_rcu_gp_state *gp_state, unsigned int period)
{
	struct side_rcu_cpu_gp_state *cpu_gp_state;
	int cpu;

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
	rseq_barrier();

	if (side_likely(rseq_offset > 0)) {
		cpu = rseq_cpu_start();
		cpu_gp_state = &gp_state->percpu_state[cpu];
		if (!rseq_addv((intptr_t *)&cpu_gp_state->count[period].rseq_end, 1, cpu))
			return;
	}
	cpu = sched_getcpu();
	if (side_unlikely(cpu < 0))
		cpu = 0;
	cpu_gp_state = &gp_state->percpu_state[cpu];
	(void) __atomic_add_fetch(&cpu_gp_state->count[period].end, 1, __ATOMIC_RELAXED);
}

#define side_rcu_dereference(p) \
	__extension__ \
	({ \
		__typeof__(p) _____side_v = __atomic_load_n(&(p), __ATOMIC_CONSUME); \
		(_____side_v); \
	})

#define side_rcu_assign_pointer(p, v)	__atomic_store_n(&(p), v, __ATOMIC_RELEASE); \

void side_rcu_wait_grace_period(struct side_rcu_gp_state *gp_state) __attribute__((visibility("hidden")));
void side_rcu_gp_init(struct side_rcu_gp_state *rcu_gp) __attribute__((visibility("hidden")));
void side_rcu_gp_exit(struct side_rcu_gp_state *rcu_gp) __attribute__((visibility("hidden")));

#endif /* _SIDE_RCU_H */
