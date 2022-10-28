// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <sched.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <poll.h>
#include <stdlib.h>

#include "rcu.h"
#include "smp.h"

/* active_readers is an input/output parameter. */
static
void check_active_readers(struct side_rcu_gp_state *gp_state, bool *active_readers)
{
	uintptr_t sum[2] = { 0, 0 };	/* begin - end */
	int i;

	for (i = 0; i < gp_state->nr_cpus; i++) {
		struct side_rcu_cpu_gp_state *cpu_state = &gp_state->percpu_state[i];

		if (active_readers[0])
			sum[0] -= __atomic_load_n(&cpu_state->count[0].end, __ATOMIC_RELAXED);
		if (active_readers[1])
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

		if (active_readers[0])
			sum[0] += __atomic_load_n(&cpu_state->count[0].begin, __ATOMIC_RELAXED);
		if (active_readers[1])
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
static
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
void side_rcu_wait_grace_period(struct side_rcu_gp_state *gp_state)
{
	bool active_readers[2] = { true, true };

	/*
	 * This memory barrier (D) pairs with memory barriers (A) and
	 * (B) on the read-side.
	 *
	 * It orders prior loads and stores before the "end"/"begin"
	 * reader state loads. In other words, it orders prior loads and
	 * stores before observation of active readers quiescence,
	 * effectively ensuring that read-side critical sections which
	 * exist after the grace period completes are ordered after
	 * loads and stores performed before the grace period.
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
	 *
	 * It orders the "end"/"begin" reader state loads before
	 * following loads and stores. In other words, it orders
	 * observation of active readers quiescence before following
	 * loads and stores, effectively ensuring that read-side
	 * critical sections which existed prior to the grace period
	 * are ordered before loads and stores performed after the grace
	 * period.
	 */
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
}

void side_rcu_gp_init(struct side_rcu_gp_state *rcu_gp)
{
	memset(rcu_gp, 0, sizeof(*rcu_gp));
	rcu_gp->nr_cpus = get_possible_cpus_array_len();
	if (!rcu_gp->nr_cpus)
		abort();
	pthread_mutex_init(&rcu_gp->gp_lock, NULL);
	rcu_gp->percpu_state = calloc(rcu_gp->nr_cpus, sizeof(struct side_rcu_cpu_gp_state));
	if (!rcu_gp->percpu_state)
		abort();
}

void side_rcu_gp_exit(struct side_rcu_gp_state *rcu_gp)
{
	pthread_mutex_destroy(&rcu_gp->gp_lock);
	free(rcu_gp->percpu_state);
}
