// SPDX-License-Identifier: MIT
/*
 * Copyright 2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>

static int nr_reader_threads = 2;
static int nr_writer_threads = 2;
static int duration_s = 10;

static volatile int start_test, stop_test;

struct thread_ctx {
	pthread_t thread_id;
	uint64_t count;
};

#include "../../src/rcu.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static struct side_rcu_gp_state test_rcu_gp;

#define POISON_VALUE	55

struct test_data {
	int v;
};

static struct test_data *rcu_p;

static
void *test_reader_thread(void *arg)
{
	struct thread_ctx *thread_ctx = (struct thread_ctx *) arg;
	uint64_t count = 0;

	while (!start_test) { }

	while (!stop_test) {
		struct test_data *p;
		int v, period;

		period = side_rcu_read_begin(&test_rcu_gp);
		p = side_rcu_dereference(rcu_p);
		if (p) {
			v = p->v;
			if (v != 0 && v != 1) {
				fprintf(stderr, "Unexpected value: %d\n", v);
				abort();
			}
		}
		side_rcu_read_end(&test_rcu_gp, period);
		count++;
	}
	thread_ctx->count = count;
	return NULL;
}

static
void *test_writer_thread(void *arg)
{
	struct thread_ctx *thread_ctx = (struct thread_ctx *) arg;
	uint64_t count = 0;

	while (!start_test) { }

	while (!stop_test) {
		struct test_data *new_data, *old_data;

		new_data = calloc(1, sizeof(struct test_data));
		if (!new_data)
			abort();

		pthread_mutex_lock(&lock);
		old_data = rcu_p;
		if (old_data)
			new_data->v = old_data->v ^ 1;	/* 0 or 1 */
		side_rcu_assign_pointer(rcu_p, new_data);
		pthread_mutex_unlock(&lock);

		side_rcu_wait_grace_period(&test_rcu_gp);

		if (old_data) {
			old_data->v = POISON_VALUE;
			free(old_data);
		}
		count++;
	}
	thread_ctx->count = count;
	return NULL;
}

int main(int argc, char **argv)
{
	struct thread_ctx *reader_ctx;
	struct thread_ctx *writer_ctx;
	int i, ret;
	int sleep_s = duration_s;
	uint64_t read_tot = 0, write_tot = 0;

	side_rcu_gp_init(&test_rcu_gp);
	//TODO: parse command line args.

	reader_ctx = calloc(nr_reader_threads, sizeof(struct thread_ctx));
	if (!reader_ctx)
		abort();
	writer_ctx = calloc(nr_writer_threads, sizeof(struct thread_ctx));
	if (!writer_ctx)
		abort();


	for (i = 0; i < nr_reader_threads; i++) {
		ret = pthread_create(&reader_ctx[i].thread_id, NULL, test_reader_thread, &reader_ctx[i]);
		if (ret) {
			errno = ret;
			perror("pthread_create");
			abort();
		}
	}
	for (i = 0; i < nr_writer_threads; i++) {
		ret = pthread_create(&writer_ctx[i].thread_id, NULL, test_writer_thread, &writer_ctx[i]);
		if (ret) {
			errno = ret;
			perror("pthread_create");
			abort();
		}
	}

	start_test = 1;

	while (sleep_s > 0) {
		sleep_s = sleep(sleep_s);
	}

	stop_test = 1;

	for (i = 0; i < nr_reader_threads; i++) {
		void *res;

		ret = pthread_join(reader_ctx[i].thread_id, &res);
		if (ret) {
			errno = ret;
			perror("pthread_join");
			abort();
		}
		read_tot += reader_ctx[i].count;
	}
	for (i = 0; i < nr_writer_threads; i++) {
		void *res;

		ret = pthread_join(writer_ctx[i].thread_id, &res);
		if (ret) {
			errno = ret;
			perror("pthread_join");
			abort();
		}
		write_tot += writer_ctx[i].count;
	}
	printf("Summary: duration: %d, nr_reader_threads: %d, nr_writer_threads: %d, reads: %" PRIu64 ", writes: %" PRIu64 "\n",
		duration_s, nr_reader_threads, nr_writer_threads, read_tot, write_tot);
	free(reader_ctx);
	free(writer_ctx);
	side_rcu_gp_exit(&test_rcu_gp);
	return 0;
}
