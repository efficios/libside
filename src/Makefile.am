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
lib_LTLIBRARIES = libside.la

libside_la_SOURCES = \
	compiler.h \
	list.h \
	rculist.h \
	side.c \
	tracer.c \
	visit-arg-vec.c \
	visit-arg-vec.h \
	visit-description.c \
	visit-description.h

libside_la_LDFLAGS = -no-undefined -version-info $(SIDE_LIBRARY_VERSION)
libside_la_LIBADD = \
	librcu.la \
	libsmp.la \
	$(RSEQ_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libside.pc
