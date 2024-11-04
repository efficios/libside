# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2022-2024 EfficiOS Inc.

AM_CPPFLAGS += -I$(top_srcdir)/src/lib

noinst_LTLIBRARIES = \
	librcu.la

librcu_la_SOURCES = \
	rcu.c \
	rcu.h

librcu_la_LIBADD = \
	$(top_builddir)/src/lib/libsmp/libsmp.la
