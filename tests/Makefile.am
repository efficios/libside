# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2022 EfficiOS Inc.

AM_CPPFLAGS += -I$(top_srcdir)/tests/utils

SUBDIRS = utils static-checker

LOG_DRIVER_FLAGS = --merge --comments
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' \
	SIDE_TESTS_SRCDIR='$(abs_top_srcdir)/tests' \
	SIDE_TESTS_BUILDDIR='$(abs_top_builddir)/tests' \
	$(SHELL) $(srcdir)/utils/tap-driver.sh

noinst_PROGRAMS = \
	regression/side-rcu-test \
	unit/test \
	unit/test-cxx \
	unit/test-no-sc \
	unit/test-no-sc-cxx \
	unit/demo \
	unit/statedump

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

unit_test_cxx_SOURCES = unit/test-cxx.cpp
unit_test_cxx_LDADD = \
	$(top_builddir)/src/libside.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

unit_test_no_sc_SOURCES = unit/test-no-sc.c
unit_test_no_sc_LDADD = \
	$(top_builddir)/src/libside.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

unit_test_no_sc_cxx_SOURCES = unit/test-no-sc-cxx.cpp
unit_test_no_sc_cxx_LDADD = \
	$(top_builddir)/src/libside.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

unit_demo_SOURCES = unit/demo.c
unit_demo_LDADD = \
	$(top_builddir)/src/libside.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

unit_statedump_SOURCES = unit/statedump.c
unit_statedump_LDADD = \
	$(top_builddir)/src/libside.la \
	$(top_builddir)/tests/utils/libtap.la \
	$(RSEQ_LIBS)

# Currently no tap tests to run
TESTS =	static-checker/run-tests
