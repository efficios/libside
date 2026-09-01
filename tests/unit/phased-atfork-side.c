// SPDX-License-Identifier: MIT
/*
 * Copyright 2026 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

/*
 * Test libside behavior across fork() with phased-atfork
 * coordination. A parent-only thread is deterministically parked
 * inside a tracer callback — hence inside a side RCU read-side
 * critical section — while the main thread forks. Without child-side
 * repair of the RCU reader state, every grace period in the child
 * would hang forever. The child then exercises grace periods on both
 * RCU domains (tracer callback unregistration for the event domain,
 * statedump provider unregistration for the statedump domain), and
 * joins the recreated statedump agent thread.
 */

#include <side/trace.h>

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tap.h"

side_static_event(my_provider_event, "test_phased_atfork", "myevent",
	SIDE_LOGLEVEL_DEBUG, side_field_list(side_field_s32("val")));

static struct side_event_state *my_event_state;

static
void tracer_notif_cb(enum side_tracer_notification notif,
		struct side_event_state **states, uint32_t nr_events,
		void *priv __attribute__((unused)))
{
	uint32_t i;

	for (i = 0; i < nr_events; i++) {
		struct side_event_state *state = states[i];
		struct side_event_description *event;

		if (!state)
			continue;
		event = side_event_state_description(state);
		if (!event)
			continue;
		if (strcmp(side_ptr_rel_get(event->provider_name), "test_phased_atfork") != 0
				|| strcmp(side_ptr_rel_get(event->event_name), "myevent") != 0)
			continue;
		if (notif == SIDE_TRACER_NOTIFICATION_INSERT_EVENTS)
			my_event_state = state;
		else
			my_event_state = NULL;
	}
}

static sem_t blocker_sem;
static bool in_callback;

static
void blocking_tracer_call(const struct side_event_description *desc __attribute__((unused)),
		const struct side_arg_vec *side_arg_vec __attribute__((unused)),
		void *priv __attribute__((unused)),
		void *caller_addr __attribute__((unused)))
{
	__atomic_store_n(&in_callback, true, __ATOMIC_SEQ_CST);
	/* Park within the side RCU read-side critical section. */
	while (sem_wait(&blocker_sem))
		;
}

static
void *reader_thread_fn(void *arg __attribute__((unused)))
{
	side_event(my_provider_event, side_arg_list(side_arg_s32(42)));
	return NULL;
}

static
void statedump_cb(void *statedump_request_key __attribute__((unused)))
{
}

int main(void)
{
	struct side_statedump_request_handle *statedump_handle;
	struct side_tracer_handle *tracer_handle;
	pthread_t reader_thread;
	uint64_t key;
	int status;
	pid_t pid;

	plan_tests(4);

	if (sem_init(&blocker_sem, 0, 0))
		abort();
	if (side_tracer_request_key(&key) != SIDE_ERROR_OK)
		abort();
	tracer_handle = side_tracer_event_notification_register(tracer_notif_cb, NULL);
	if (!tracer_handle)
		abort();
	ok(my_event_state != NULL, "static event known to tracer");

	/* Spawns the statedump agent thread. */
	statedump_handle = side_statedump_request_notification_register(
		"test_phased_atfork", statedump_cb,
		SIDE_STATEDUMP_MODE_AGENT_THREAD);
	if (!statedump_handle)
		abort();

	if (side_tracer_callback_register(my_event_state, blocking_tracer_call,
			NULL, key) != SIDE_ERROR_OK)
		abort();
	if (pthread_create(&reader_thread, NULL, reader_thread_fn, NULL))
		abort();
	/* Wait for the reader to park inside its read-side section. */
	while (!__atomic_load_n(&in_callback, __ATOMIC_SEQ_CST))
		sched_yield();

	pid = fork();
	if (pid < 0)
		abort();
	if (!pid) {
		/*
		 * Child. The parked reader thread does not exist here,
		 * but its read-side critical section was in flight at
		 * fork() time: the grace periods below hang unless the
		 * child-side repair reset the RCU reader state.
		 */
		if (side_tracer_callback_unregister(my_event_state,
				blocking_tracer_call, NULL, key) != SIDE_ERROR_OK)
			_exit(10);
		/* Disabled event: fast path must be usable. */
		side_event(my_provider_event, side_arg_list(side_arg_s32(43)));
		/*
		 * Joins the agent thread recreated by the
		 * service-restart participant; waits a grace period on
		 * the statedump RCU domain.
		 */
		side_statedump_request_notification_unregister(statedump_handle);
		side_tracer_event_notification_unregister(tracer_handle);
		_exit(0);
	}
	if (waitpid(pid, &status, 0) < 0)
		abort();
	ok(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		"child grace periods completed and agent thread recreated (status %d)",
		status);

	/* Resume the parked reader before parent-side teardown. */
	if (sem_post(&blocker_sem))
		abort();
	if (pthread_join(reader_thread, NULL))
		abort();
	ok(side_tracer_callback_unregister(my_event_state, blocking_tracer_call,
			NULL, key) == SIDE_ERROR_OK,
		"parent tracer callback unregister after fork");
	side_statedump_request_notification_unregister(statedump_handle);
	side_tracer_event_notification_unregister(tracer_handle);
	ok(1, "parent teardown clean");

	return exit_status();
}
