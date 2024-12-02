// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "panic.h"

void do_panic(const char *file, int linum, const char *func_name, const char *fmt, ...)
{
	va_list ap;
	char buf[4096];
	void *bt[16];
	usize size;
	char bar[] = "================================================================================\n";

	va_start(ap, fmt);

	vsnprintf(buf, sizeof(buf), fmt, ap);

	va_end(ap);

	fprintf(stderr,
		"Readside [%d] panicked at [%s:%d] in %s(): %s\n",
		getpid(), file, linum, func_name, buf);

	fprintf(stderr, "Backtrace:\n%s", bar);
	size = backtrace(bt, array_size(bt));
	backtrace_symbols_fd(bt, size, STDERR_FILENO);
	fputs(bar, stderr);

	fflush(NULL);

	/* Do not run atexit */
	_Exit(EXIT_FAILURE);
}
