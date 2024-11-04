# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2022-2024 EfficiOS Inc.

AM_CPPFLAGS += -I$(top_srcdir)/src/lib

lib_LTLIBRARIES = libside.la

libside_la_SOURCES = \
	compiler.h \
	list.h \
	rculist.h \
	side.c

libside_la_LDFLAGS = -no-undefined -version-info $(SIDE_LIBRARY_VERSION)
libside_la_LIBADD = \
	$(top_builddir)/src/lib/librcu/librcu.la \
	$(RSEQ_LIBS)
