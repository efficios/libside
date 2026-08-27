// SPDX-License-Identifier: MIT
/*
 * Copyright 2026 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <phased-atfork/phased-atfork.h>

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>

struct phased_atfork_participant {
	struct phased_atfork_ops ops;
	void *priv;
	int level;
	/* Doubly-linked list sorted by ascending level. */
	struct phased_atfork_participant *prev, *next;
};

/*
 * The state lock protects the participant list, the atfork
 * installation flag and the fork-sequence ownership. It is never
 * held while invoking participant callbacks nor across fork(): the
 * participant list is kept stable during a fork sequence by
 * ownership (list mutators wait for the sequence to complete).
 */
static pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t state_cond = PTHREAD_COND_INITIALIZER;

static struct phased_atfork_participant *list_head;	/* lowest level */
static struct phased_atfork_participant *list_tail;	/* highest level */

static bool atfork_installed;

/*
 * Fork-sequence ownership. A single thread owns the sequence from
 * prepare dispatch to parent/child completion; re-entry by the same
 * thread (explicit wrapper call + atfork-installed dispatch) nests.
 */
static bool owner_set;
static pthread_t owner;
static int owner_nest;
/* Set once the parent/child unwind dispatch ran for this sequence. */
static bool unwind_done;
/* Signal mask saved by the outermost prepare. */
static sigset_t saved_sigmask;

static
void atfork_prepare_handler(void)
{
	phased_atfork_prepare();
}

static
void atfork_parent_handler(void)
{
	phased_atfork_parent();
}

static
void atfork_child_handler(void)
{
	phased_atfork_child();
}

static
bool self_owns_sequence(void)
{
	return owner_set && pthread_equal(owner, pthread_self());
}

struct phased_atfork_participant *phased_atfork_register(
		const struct phased_atfork_ops *ops, void *priv, int level)
{
	struct phased_atfork_participant *participant, *iter;

	if (!ops)
		return NULL;
	participant = (struct phased_atfork_participant *)
			calloc(1, sizeof(struct phased_atfork_participant));
	if (!participant)
		return NULL;
	participant->ops = *ops;
	participant->priv = priv;
	participant->level = level;

	pthread_mutex_lock(&state_lock);
	/* Registering from a participant callback is forbidden. */
	if (self_owns_sequence())
		abort();
	while (owner_set)
		pthread_cond_wait(&state_cond, &state_lock);
	if (!atfork_installed) {
		if (pthread_atfork(atfork_prepare_handler,
				atfork_parent_handler,
				atfork_child_handler)) {
			pthread_mutex_unlock(&state_lock);
			free(participant);
			return NULL;
		}
		atfork_installed = true;
	}
	/*
	 * Insert after existing participants of the same level, so
	 * equal levels dispatch in registration order.
	 */
	for (iter = list_head; iter && iter->level <= level; iter = iter->next)
		;
	participant->next = iter;
	if (iter) {
		participant->prev = iter->prev;
		iter->prev = participant;
	} else {
		participant->prev = list_tail;
		list_tail = participant;
	}
	if (participant->prev)
		participant->prev->next = participant;
	else
		list_head = participant;
	pthread_mutex_unlock(&state_lock);
	return participant;
}

void phased_atfork_unregister(struct phased_atfork_participant *participant)
{
	if (!participant)
		return;
	pthread_mutex_lock(&state_lock);
	/* Unregistering from a participant callback is forbidden. */
	if (self_owns_sequence())
		abort();
	while (owner_set)
		pthread_cond_wait(&state_cond, &state_lock);
	if (participant->prev)
		participant->prev->next = participant->next;
	else
		list_head = participant->next;
	if (participant->next)
		participant->next->prev = participant->prev;
	else
		list_tail = participant->prev;
	pthread_mutex_unlock(&state_lock);
	free(participant);
}

void phased_atfork_prepare(void)
{
	struct phased_atfork_participant *participant;
	sigset_t all_sigs;

	pthread_mutex_lock(&state_lock);
	if (self_owns_sequence()) {
		owner_nest++;
		pthread_mutex_unlock(&state_lock);
		return;
	}
	while (owner_set)
		pthread_cond_wait(&state_cond, &state_lock);
	owner = pthread_self();
	owner_set = true;
	owner_nest = 1;
	unwind_done = false;
	pthread_mutex_unlock(&state_lock);

	/*
	 * Block all signals across the fork sequence so signal
	 * handlers cannot run while participant locks are held.
	 */
	sigfillset(&all_sigs);
	if (pthread_sigmask(SIG_BLOCK, &all_sigs, &saved_sigmask))
		abort();

	/*
	 * Quiesce stage: every participant parks its service threads
	 * before any participant acquires locks, so that a quiescing
	 * thread can never block on a lock held for fork.
	 */
	for (participant = list_head; participant; participant = participant->next) {
		if (participant->ops.quiesce)
			participant->ops.quiesce(participant->priv);
	}
	/* Acquire stage: ascending level order. */
	for (participant = list_head; participant; participant = participant->next) {
		if (participant->ops.acquire)
			participant->ops.acquire(participant->priv);
	}
}

void phased_atfork_parent(void)
{
	struct phased_atfork_participant *participant;
	bool unwind;

	pthread_mutex_lock(&state_lock);
	if (!self_owns_sequence())
		abort();
	unwind = !unwind_done;
	unwind_done = true;
	pthread_mutex_unlock(&state_lock);

	if (unwind) {
		/* Descending level order. */
		for (participant = list_tail; participant; participant = participant->prev) {
			if (participant->ops.parent)
				participant->ops.parent(participant->priv);
		}
		if (pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL))
			abort();
	}

	pthread_mutex_lock(&state_lock);
	if (!--owner_nest) {
		owner_set = false;
		pthread_cond_broadcast(&state_cond);
	}
	pthread_mutex_unlock(&state_lock);
}

void phased_atfork_child(void)
{
	struct phased_atfork_participant *participant;

	/*
	 * Unlocked accesses: only the fork-sequence owner survives in
	 * the child, and only the owner thread runs this function.
	 */
	if (!self_owns_sequence())
		abort();
	if (!unwind_done) {
		/*
		 * Single-threaded point. Reinitialize the
		 * synchronization state inherited from the parent
		 * (waiters do not exist in the child) BEFORE child
		 * callbacks recreate service threads.
		 */
		pthread_mutex_init(&state_lock, NULL);
		pthread_cond_init(&state_cond, NULL);
		unwind_done = true;
		/* Ascending level order. */
		for (participant = list_head; participant; participant = participant->next) {
			if (participant->ops.child)
				participant->ops.child(participant->priv);
		}
		if (pthread_sigmask(SIG_SETMASK, &saved_sigmask, NULL))
			abort();
	}
	pthread_mutex_lock(&state_lock);
	if (!--owner_nest) {
		owner_set = false;
		pthread_cond_broadcast(&state_cond);
	}
	pthread_mutex_unlock(&state_lock);
}
