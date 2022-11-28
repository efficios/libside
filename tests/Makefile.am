# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2022 EfficiOS Inc.

AM_CPPFLAGS += -I$(top_srcdir)/tests/utils

SUBDIRS = utils

TEST_EXTENSIONS = .tap
TAP_LOG_DRIVER_FLAGS = --merge --comments
TAP_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' \
	TGIF_TESTS_SRCDIR='$(abs_top_srcdir)/tests' \
	TGIF_TESTS_BUILDDIR='$(abs_top_builddir)/tests' \
	$(SHELL) $(srcdir)/utils/tap-driver.sh

noinst_PROGRAMS = \
	regression/tgif-rcu-test \
	unit/test

regression_tgif_rcu_test_SOURCES = regression/tgif-rcu-test.c
regression_tgif_rcu_test_LDADD = \
	$(top_builddir)/src/librcu.la \
	$(top_builddir)/src/libsmp.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

unit_test_SOURCES = unit/test.c
unit_test_LDADD = \
	$(top_builddir)/src/libtgif.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

# Currently no tap tests to run
TESTS =
