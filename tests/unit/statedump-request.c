// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: 2026 EfficiOS Inc.

/*
 * Asking whether a statedump is still to be taken, and being told when
 * one has been.
 *
 * A tracer requests a statedump and the application takes it whenever
 * its own mode says: a thread libside spawns, or, in polling mode, the
 * next time the application runs its pending requests. So a tracer
 * which wants to know whether the state it asked for has arrived needs
 * both halves checked here.
 *
 * side_tracer_statedump_request_pending() is the level. The case it
 * exists for is the one in the middle: a request which has been taken
 * off the queue and whose callback has not returned is neither waiting
 * nor done, and reporting it as done would tell a tracer the state has
 * arrived while it is still being written.
 *
 * side_tracer_statedump_completion_register() is the edge, so that a
 * tracer waiting for a statedump need not poll for it. It is a hint,
 * and what it must guarantee is that the request already reads as taken
 * by the time the callback runs -- otherwise being told is of no use.
 */

#include <side/trace.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "tap.h"

side_static_event(dumped_event, "myprovider", "dumped", SIDE_LOGLEVEL_INFO,
	side_field_list(side_field_u32("id")));

static struct side_statedump_request_handle *handle;
static struct side_statedump_completion_handle *completion;
static uint64_t key_a, key_b;

/* Set by the statedump callback, read by the test. */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t entered = PTHREAD_COND_INITIALIZER;
static pthread_cond_t observed = PTHREAD_COND_INITIALIZER;
static int in_callback, was_observed;
static bool pending_while_dumping, other_key_while_dumping;
/*
 * Whether the statedump callback waits to be let go. Only for the
 * statedump another thread takes: in polling mode the thread running
 * the callback is the one which would release it.
 */
static bool hold_in_callback;

/* Set by the completion callback. */
static int notifications;
static bool pending_when_notified = true, notified_by_dumping_thread;
static void *notified_priv;
static pthread_t dumping_thread;

/* Wait until the statedump callback has been entered. */
static
void wait_for_callback(void)
{
	pthread_mutex_lock(&lock);
	while (!in_callback)
		pthread_cond_wait(&entered, &lock);
	pthread_mutex_unlock(&lock);
}

/* Let a statedump callback which is waiting to be let go return. */
static
void release_callback(void)
{
	pthread_mutex_lock(&lock);
	was_observed = 1;
	pthread_cond_broadcast(&observed);
	pthread_mutex_unlock(&lock);
}

static
void statedump_cb(void *statedump_request_key)
{
	dumping_thread = pthread_self();
	/*
	 * The request being taken right now is neither queued nor
	 * finished, and must read as outstanding all the same.
	 */
	pending_while_dumping = side_tracer_statedump_request_pending(key_a);
	other_key_while_dumping = side_tracer_statedump_request_pending(key_b);

	pthread_mutex_lock(&lock);
	in_callback = 1;
	pthread_cond_broadcast(&entered);
	/*
	 * Stay in the callback until the test has looked, so that what
	 * it observes is this statedump being taken rather than a race
	 * with its completion.
	 */
	while (hold_in_callback && !was_observed)
		pthread_cond_wait(&observed, &lock);
	pthread_mutex_unlock(&lock);

	side_statedump_event_call(dumped_event, statedump_request_key,
		side_arg_list(side_arg_u32(42)));
}

static
void completion_cb(uint64_t key, void *priv)
{
	notifications++;
	notified_priv = priv;
	notified_by_dumping_thread = pthread_equal(pthread_self(), dumping_thread);
	if (key == key_a)
		pending_when_notified = side_tracer_statedump_request_pending(key_a);
}

/*
 * Polling mode: nothing is taken until the application asks for it to
 * be, which is what lets the queued state be observed at all.
 */
static
void test_polling(void)
{
	handle = side_statedump_request_notification_register("polling",
			statedump_cb, SIDE_STATEDUMP_MODE_POLLING);
	if (!handle)
		abort();

	/*
	 * Registering a statedump callback queues a statedump for every
	 * tracer, which will reach both keys.
	 */
	ok(side_tracer_statedump_request_pending(key_a),
		"registering a callback leaves a statedump outstanding for every key");
	side_statedump_run_pending_requests(handle);
	ok(!side_tracer_statedump_request_pending(key_a),
		"and none once it has been taken");

	if (side_tracer_statedump_request(key_a) != SIDE_ERROR_OK)
		abort();
	ok(side_tracer_statedump_request_pending(key_a),
		"a request which has not been taken is outstanding");
	ok(!side_tracer_statedump_request_pending(key_b),
		"and only for the key it was made with");

	in_callback = 0;
	side_statedump_run_pending_requests(handle);
	ok(pending_while_dumping,
		"a request being taken is still outstanding");
	ok(!other_key_while_dumping,
		"a request being taken is outstanding for its key alone");
	ok(!side_tracer_statedump_request_pending(key_a),
		"a request which has been taken is not outstanding");

	if (side_tracer_statedump_request(key_a) != SIDE_ERROR_OK)
		abort();
	if (side_tracer_statedump_request_cancel(key_a) != SIDE_ERROR_OK)
		abort();
	ok(!side_tracer_statedump_request_pending(key_a),
		"a cancelled request is not outstanding");

	side_statedump_request_notification_unregister(handle);
	ok(!side_tracer_statedump_request_pending(key_a),
		"nothing is outstanding once no callback is registered");
	if (side_tracer_statedump_request(key_a) != SIDE_ERROR_OK)
		abort();
	ok(!side_tracer_statedump_request_pending(key_a),
		"nor is a request made while none is registered");
}

/*
 * Agent thread mode: the statedump is taken on a thread of libside's,
 * so the window in which it is being taken is observable from here.
 */
static
void test_agent_thread(void)
{
	handle = side_statedump_request_notification_register("agent",
			statedump_cb, SIDE_STATEDUMP_MODE_AGENT_THREAD);
	if (!handle)
		abort();
	/* Registration waits for the statedump it queues to be taken. */
	ok(!side_tracer_statedump_request_pending(key_a),
		"nothing is outstanding once registration has settled");

	in_callback = 0;
	was_observed = 0;
	notifications = 0;
	hold_in_callback = true;
	if (side_tracer_statedump_request(key_a) != SIDE_ERROR_OK)
		abort();
	wait_for_callback();
	ok(side_tracer_statedump_request_pending(key_a),
		"a request being taken by the agent thread is outstanding");
	release_callback();

	/* Waits for the agent thread, which has nothing left to take. */
	side_statedump_request_notification_unregister(handle);
	hold_in_callback = false;
	ok(!side_tracer_statedump_request_pending(key_a),
		"and is not once the agent thread has taken it");
	ok(notifications > 0, "taking a statedump notifies the tracer");
	ok(notified_by_dumping_thread,
		"the tracer is notified by the thread which took it");
	ok(!pending_when_notified,
		"a statedump already reads as taken when the tracer is told of it");
	ok(notified_priv == (void *) 0x5105,
		"the tracer is given the pointer it registered");
}

int main(void)
{
	plan_tests(17);

	if (side_tracer_request_key(&key_a) != SIDE_ERROR_OK ||
	    side_tracer_request_key(&key_b) != SIDE_ERROR_OK)
		abort();
	completion = side_tracer_statedump_completion_register(completion_cb,
			(void *) 0x5105);
	if (!completion)
		abort();

	test_polling();
	test_agent_thread();

	side_tracer_statedump_completion_unregister(completion);
	return exit_status();
}
