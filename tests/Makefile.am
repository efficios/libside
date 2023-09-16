# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2022 EfficiOS Inc.

AM_CPPFLAGS += -I$(top_srcdir)/tests/utils

SUBDIRS = utils

TEST_EXTENSIONS = .tap
TAP_LOG_DRIVER_FLAGS = --merge --comments
TAP_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' \
	SIDE_TESTS_SRCDIR='$(abs_top_srcdir)/tests' \
	SIDE_TESTS_BUILDDIR='$(abs_top_builddir)/tests' \
	$(SHELL) $(srcdir)/utils/tap-driver.sh

noinst_PROGRAMS = \
	regression/side-rcu-test \
	unit/test \
	unit/demo

regression_side_rcu_test_SOURCES = regression/side-rcu-test.c
regression_side_rcu_test_LDADD = \
	$(top_builddir)/src/librcu.la \
	$(top_builddir)/src/libsmp.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

unit_test_SOURCES = unit/test.c
unit_test_LDADD = \
	$(top_builddir)/src/libside.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

unit_demo_SOURCES = unit/demo.c
unit_demo_LDADD = \
	$(top_builddir)/src/libside.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

# Currently no tap tests to run
TESTS =
