# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2022 EfficiOS Inc.

# Internal convenience libraries
noinst_LTLIBRARIES = \
	librcu.la \
	libsmp.la

librcu_la_SOURCES = \
	rcu.c \
	rcu.h

libsmp_la_SOURCES = \
	smp.c \
	smp.h

# Public libaries
lib_LTLIBRARIES = libtgif.la

libtgif_la_SOURCES = \
	list.h \
	tgif.c \
	tracer.c

libtgif_la_LDFLAGS = -no-undefined -version-info $(TGIF_LIBRARY_VERSION)
libtgif_la_LIBADD = \
	librcu.la \
	libsmp.la \
	$(RSEQ_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtgif.pc
