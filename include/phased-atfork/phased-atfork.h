// SPDX-License-Identifier: MIT
/*
 * Copyright 2026 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef _PHASED_ATFORK_H
#define _PHASED_ATFORK_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Phased pthread_atfork coordination.
 *
 * glibc pthread_atfork provides a single flat prepare list (run in
 * reverse registration order), which cannot express "all service
 * threads are quiesced before any lock is acquired" nor
 * cross-library lock-ordering levels. This library registers
 * pthread_atfork once and dispatches fork participants in phases:
 *
 * - prepare: block all signals, then invoke ALL quiesce callbacks
 *   (their relative order is immaterial by contract), then invoke
 *   acquire callbacks in ascending level order, then fork()
 *   proceeds.
 * - parent: invoke parent callbacks in descending level order, then
 *   restore the signal mask.
 * - child: invoke child callbacks in ascending level order, then
 *   restore the signal mask.
 *
 * Participant contracts:
 *
 * - quiesce may wait for the participant's own service threads
 *   (pause/ack protocols), but must not acquire, nor depend on,
 *   anything another participant's quiescence can hold or need.
 * - acquire performs lock acquisitions only: no waits on thread
 *   progress, no grace periods.
 * - parent releases the locks taken by acquire and resumes the
 *   threads paused by quiesce.
 * - child reinitializes the participant's own state; it runs
 *   single-threaded until a child callback recreates threads. Lower
 *   levels are already reinitialized when it runs.
 * - Callbacks must not call fork(), and must not register or
 *   unregister participants.
 * - Participants must remain loaded for the process lifetime (link
 *   with -Wl,-z,nodelete).
 *
 * Any callback pointer may be NULL, in which case the participant is
 * skipped for that phase.
 *
 * The value of this coordination depends on the library being a
 * process-wide singleton: link against the shared library so that
 * all participants share a single dispatcher and a single
 * pthread_atfork registration.
 *
 * Depends on glibc >= 2.24 so that atfork prepare handlers may wait
 * on threads which use malloc (glibc commit 8a727af9).
 */

/*
 * Levels are signed integers; lower levels acquire their locks
 * before higher levels (and reinitialize before them in the child).
 * The following conventional levels are suggested for tracing
 * stacks, where instrumentation-layer notification locks are outer
 * to tracer control locks, which are outer to instrumentation-layer
 * leaf locks:
 */
enum phased_atfork_level {
	PHASED_ATFORK_LEVEL_INSTRUMENTATION_OUTER = 0,
	PHASED_ATFORK_LEVEL_TRACER = 1,
	PHASED_ATFORK_LEVEL_INSTRUMENTATION_LEAF = 2,
	PHASED_ATFORK_LEVEL_SERVICE_RESTART = 3,
};

struct phased_atfork_ops {
	void (*quiesce)(void *priv);
	void (*acquire)(void *priv);
	void (*parent)(void *priv);
	void (*child)(void *priv);
};

struct phased_atfork_participant;

/*
 * Register a fork participant. The first registration installs the
 * pthread_atfork handlers. Waits for any in-flight fork sequence to
 * complete. Returns NULL on error.
 */
struct phased_atfork_participant *phased_atfork_register(
		const struct phased_atfork_ops *ops, void *priv, int level);

/*
 * Unregister a fork participant. Waits for any in-flight fork
 * sequence to complete; the participant's callbacks are not invoked
 * after this call returns.
 */
void phased_atfork_unregister(struct phased_atfork_participant *participant);

/*
 * Whole-sequence entry points for fork() wrapper libraries which
 * must run the prepare sequence before calling fork() explicitly
 * (e.g. LD_PRELOAD wrappers predating this library, or clone()-based
 * process creation which does not run atfork handlers). Calls nest
 * per owning thread: the atfork-installed dispatch becomes a no-op
 * when the sequence was already driven by the same thread.
 * Concurrent fork attempts from other threads serialize on
 * ownership.
 */
void phased_atfork_prepare(void);
void phased_atfork_parent(void);
void phased_atfork_child(void);

#ifdef __cplusplus
}
#endif

#endif /* _PHASED_ATFORK_H */
