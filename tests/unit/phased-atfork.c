// SPDX-License-Identifier: MIT
/*
 * Copyright 2026 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <phased-atfork/phased-atfork.h>

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tap.h"

/*
 * Dispatch sequence log. Only written by fork-sequence callbacks,
 * which are single-threaded by construction.
 */
static char log_buf[256];
static int log_len;

static
void log_event(const char *event)
{
	log_len += snprintf(log_buf + log_len, sizeof(log_buf) - log_len,
			"%s ", event);
}

static
void log_reset(void)
{
	log_buf[0] = '\0';
	log_len = 0;
}

#define DEFINE_PARTICIPANT_CALLBACKS(n) \
static void quiesce_##n(void *priv __attribute__((unused))) { log_event("q" #n); } \
static void acquire_##n(void *priv __attribute__((unused))) { log_event("a" #n); } \
static void parent_##n(void *priv __attribute__((unused)))  { log_event("p" #n); } \
static void child_##n(void *priv __attribute__((unused)))   { log_event("c" #n); } \
static const struct phased_atfork_ops ops_##n = { \
	.quiesce = quiesce_##n, \
	.acquire = acquire_##n, \
	.parent = parent_##n, \
	.child = child_##n, \
}

DEFINE_PARTICIPANT_CALLBACKS(0);
DEFINE_PARTICIPANT_CALLBACKS(1);
DEFINE_PARTICIPANT_CALLBACKS(2);
DEFINE_PARTICIPANT_CALLBACKS(3);

static const char prepare_seq_full[] = "q0 q1 q2 q3 a0 a1 a2 a3 ";
static const char prepare_seq_no2[] = "q0 q1 q3 a0 a1 a3 ";

static
bool sigmask_is_clear(void)
{
	sigset_t cur;

	if (pthread_sigmask(SIG_SETMASK, NULL, &cur))
		abort();
	return !sigismember(&cur, SIGUSR1);
}

/*
 * Child-side expectations are communicated through the exit status:
 * 0 on success, distinctive non-zero codes on failure.
 */
static
void child_check_and_exit(const char *prepare_seq, const char *child_seq)
{
	char want[256];

	snprintf(want, sizeof(want), "%s%s", prepare_seq, child_seq);
	if (strcmp(log_buf, want) != 0)
		_exit(10);
	if (!sigmask_is_clear())
		_exit(11);
	_exit(0);
}

static
bool parent_check(pid_t pid, const char *prepare_seq, const char *parent_seq,
		char *got, size_t got_len)
{
	char want[256];
	int status;

	snprintf(want, sizeof(want), "%s%s", prepare_seq, parent_seq);
	snprintf(got, got_len, "%s", log_buf);
	if (waitpid(pid, &status, 0) < 0)
		abort();
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		diag("child failed with status %d", status);
		return false;
	}
	if (strcmp(log_buf, want) != 0) {
		diag("parent sequence mismatch: got '%s', want '%s'",
			log_buf, want);
		return false;
	}
	return true;
}

int main(void)
{
	struct phased_atfork_participant *p0, *p1, *p2, *p3;
	char got[256];
	pid_t pid;

	plan_tests(6);

	/* Register out of order: dispatch must sort by level. */
	p2 = phased_atfork_register(&ops_2, NULL, 2);
	p0 = phased_atfork_register(&ops_0, NULL, 0);
	p3 = phased_atfork_register(&ops_3, NULL, 3);
	p1 = phased_atfork_register(&ops_1, NULL, 1);
	if (!p0 || !p1 || !p2 || !p3)
		abort();

	/* atfork-driven flow: plain fork(). */
	log_reset();
	pid = fork();
	if (pid < 0)
		abort();
	if (!pid)
		child_check_and_exit(prepare_seq_full, "c0 c1 c2 c3 ");
	ok(parent_check(pid, prepare_seq_full, "p3 p2 p1 p0 ",
			got, sizeof(got)),
		"atfork-driven: child ok, parent sequence '%s'", got);
	ok(sigmask_is_clear(),
		"atfork-driven: parent signal mask restored");

	/*
	 * Wrapper-driven flow: explicit prepare before fork(), explicit
	 * parent/child after. The atfork-installed dispatch must no-op
	 * (same-owner nesting): every callback runs exactly once.
	 */
	log_reset();
	phased_atfork_prepare();
	pid = fork();
	if (pid < 0)
		abort();
	if (!pid) {
		phased_atfork_child();
		child_check_and_exit(prepare_seq_full, "c0 c1 c2 c3 ");
	}
	phased_atfork_parent();
	ok(parent_check(pid, prepare_seq_full, "p3 p2 p1 p0 ",
			got, sizeof(got)),
		"wrapper-driven: callbacks exactly once, sequence '%s'", got);
	ok(sigmask_is_clear(),
		"wrapper-driven: parent signal mask restored");

	/*
	 * Unregister the level-2 participant; a further fork (also
	 * proving ownership was fully released) must skip it.
	 */
	phased_atfork_unregister(p2);
	log_reset();
	pid = fork();
	if (pid < 0)
		abort();
	if (!pid)
		child_check_and_exit(prepare_seq_no2, "c0 c1 c3 ");
	ok(parent_check(pid, prepare_seq_no2, "p3 p1 p0 ",
			got, sizeof(got)),
		"after unregister: level 2 skipped, sequence '%s'", got);

	phased_atfork_unregister(p0);
	phased_atfork_unregister(p1);
	phased_atfork_unregister(p3);
	log_reset();
	pid = fork();
	if (pid < 0)
		abort();
	if (!pid) {
		if (log_buf[0] != '\0')
			_exit(12);
		_exit(0);
	}
	ok(parent_check(pid, "", "", got, sizeof(got)),
		"all unregistered: empty dispatch");

	return exit_status();
}
