# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2022-2024 EfficiOS Inc.


AM_CPPFLAGS += -I$(top_srcdir)/src/lib

lib_LTLIBRARIES = libside-console-tracer.la

libside_console_tracer_la_SOURCES = \
	tracer.c

libside_console_tracer_la_LIBADD = \
	$(top_builddir)/src/lib/libside/libside.la \
	$(top_builddir)/src/lib/libside-tools/libside-tools.la
