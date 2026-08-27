# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 EfficiOS Inc.

AM_CPPFLAGS += -I$(top_srcdir)/src/lib

lib_LTLIBRARIES = libphased-atfork.la

libphased_atfork_la_SOURCES = \
	phased-atfork.c

# Shared-only: the coordination depends on being a process-wide
# singleton; a static variant would allow multiple dispatchers in one
# process. nodelete: dlclose would silently unregister the atfork
# handlers.
libphased_atfork_la_LDFLAGS = -no-undefined -shared \
	-version-info $(PHASED_ATFORK_LIBRARY_VERSION) \
	-Wl,-z,nodelete
libphased_atfork_la_LIBADD = $(PTHREAD_LIBS)
