// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 EfficiOS Inc.
// SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com>

#ifndef READSIDE_LOGGING_H
#define READSIDE_LOGGING_H

#include <stdio.h>

enum loglevel {
	LOGLEVEL_ERROR,
	LOGLEVEL_WARNING,
	LOGLEVEL_DEBUG,
};

extern int current_loglevel;

#define logging(lvl, prefix, fmt, ...)					\
	if (current_loglevel >= lvl)					\
		fprintf(stderr, prefix fmt "\n", ##__VA_ARGS__)

#define error(fmt, ...) 	logging(LOGLEVEL_ERROR, "Error: ", fmt, ##__VA_ARGS__)
#define warning(fmt, ...) 	logging(LOGLEVEL_WARNING, "Warning: ", fmt, ##__VA_ARGS__)
#define debug(fmt, ...) 	logging(LOGLEVEL_DEBUG, "Debug: ", fmt, ##__VA_ARGS__)

#endif	/* READSIDE_LOGGING_H */
