dnl SPDX-License-Identifier: MIT
dnl SPDX-FileCopyrightText: 2022 EfficiOS Inc.
dnl
dnl Process this file with autoconf to produce a configure script.

# Project version information
m4_define([side_version_major], [0])
m4_define([side_version_minor], [1])
m4_define([side_version_patch], [0])
m4_define([side_version_dev_stage], [-pre])
m4_define([side_version], side_version_major[.]side_version_minor[.]side_version_patch[]side_version_dev_stage)

# Library version information of "libside"
# Following the numbering scheme proposed by libtool for the library version
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
m4_define([side_lib_version_current], [0])
m4_define([side_lib_version_revision], [0])
m4_define([side_lib_version_age], [0])
m4_define([side_lib_version], side_lib_version_current[:]side_lib_version_revision[:]side_lib_version_age)


##                     ##
## Autoconf base setup ##
##                     ##

AC_PREREQ([2.69])
AC_INIT([libside],[side_version],[mathieu dot desnoyers at efficios dot com],[],[https://github.com/compudj/libside/])

AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET
AC_CANONICAL_HOST


##                     ##
## Automake base setup ##
##                     ##

AM_INIT_AUTOMAKE([1.12 foreign dist-bzip2 no-dist-gzip nostdinc subdir-objects -Wall -Werror])
AM_MAINTAINER_MODE([enable])

# Enable silent rules by default
AM_SILENT_RULES([yes])


##                   ##
## C compiler checks ##
##                   ##

# Choose the C compiler
AC_PROG_CC
# AC_PROG_CC_STDC was merged in AC_PROG_CC in autoconf 2.70
m4_version_prereq([2.70], [], [AC_PROG_CC_STDC])

# Make sure the C compiler supports C99
AS_IF([test "$ac_cv_prog_cc_c99" = "no"], [AC_MSG_ERROR([The compiler does not support C99])])

# Enable available system extensions and LFS support
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

# Make sure the C compiler supports __attribute__
AX_C___ATTRIBUTE__
AS_IF([test "x$ax_cv___attribute__" != "xyes"],
  [AC_MSG_ERROR([The compiler does not support __attribute__ extensions])])

# Make sure we have pthread support
AX_PTHREAD([], [AC_MSG_ERROR([Could not configure pthread support])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_TYPEOF
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Detect warning flags supported by the C compiler and append them to
# WARN_CFLAGS.
m4_define([WARN_FLAGS_LIST], [ dnl
  -Wall dnl
  -Wextra dnl
  -Wmissing-prototypes dnl
  -Wmissing-declarations dnl
  -Wnull-dereference dnl
  -Wundef dnl
  -Wshadow dnl
  -Wjump-misses-init dnl
  -Wsuggest-attribute=format dnl
  -Wtautological-constant-out-of-range-compare dnl
  -Wnested-externs dnl
  -Wwrite-strings dnl
  -Wformat=2 dnl
  -Wstrict-aliasing dnl
  -Wmissing-noreturn dnl
  -Winit-self dnl
  -Wduplicated-cond dnl
  -Wduplicated-branches dnl
  -Wlogical-op dnl
  -Wredundant-decls dnl
])

# Pass -Werror as an extra flag during the test: this is needed to make the
# -Wunknown-warning-option diagnostic fatal with clang.
AC_LANG_PUSH([C])
AX_APPEND_COMPILE_FLAGS([WARN_FLAGS_LIST], [WARN_CFLAGS], [-Werror])
AC_LANG_POP([C])

AC_LANG_PUSH([C++])
AX_APPEND_COMPILE_FLAGS([WARN_FLAGS_LIST], [WARN_CXXFLAGS], [-Werror])
AC_LANG_POP([C++])

AE_IF_FEATURE_ENABLED([Werror], [WARN_CFLAGS="${WARN_CFLAGS} -Werror"])
AE_IF_FEATURE_ENABLED([Werror], [WARN_CXXFLAGS="${WARN_CXXFLAGS} -Werror"])


##                     ##
## C++ compiler checks ##
##                     ##

# Require a C++11 compiler with GNU extensions (-std=gnu++11).
#
# A compiler with GNU extensions is mandatory because of the usage of
# preprocessor `##' comma deletion with empty `__VA_ARGS__' in the public header
# files.
AX_CXX_COMPILE_STDCXX([11], [ext], [mandatory])


##               ##
## Header checks ##
##               ##

AC_HEADER_STDBOOL
AC_CHECK_HEADERS([ \
	limits.h \
	stddef.h \
	sys/time.h \
])


##                 ##
## Programs checks ##
##                 ##

AM_PROG_AR
AC_PROG_AWK
AC_PROG_MAKE_SET

# Initialize and configure libtool
LT_INIT


##                ##
## Library checks ##
##                ##

# Checks for library functions.
AC_FUNC_MMAP
AC_FUNC_FORK
AC_CHECK_FUNCS([ \
	atexit \
	memset \
	strerror \
])

# AC_FUNC_MALLOC causes problems when cross-compiling.
#AC_FUNC_MALLOC

PKG_CHECK_MODULES([RSEQ], [librseq])

##                             ##
## Optional features selection ##
##                             ##

# When given, add -Werror to WARN_CFLAGS and WARN_CXXFLAGS.
# Disabled by default
AE_FEATURE_DEFAULT_DISABLE
AE_FEATURE([Werror], [Treat compiler warnings as errors.])


##                                             ##
## Substitute variables for use in Makefile.am ##
##                                             ##

# Library versions for libtool
AC_SUBST([SIDE_LIBRARY_VERSION], [side_lib_version])

# The order in which the include folders are searched is important.
# The top_builddir should always be searched first in the event that a build
# time generated file is included.
AM_CPPFLAGS="-I\$(top_builddir)/include -I\$(top_srcdir)/include -include config.h $RSEQ_CFLAGS"
AC_SUBST(AM_CPPFLAGS)

AM_CFLAGS="$WARN_CFLAGS $PTHREAD_CFLAGS"
AC_SUBST(AM_CFLAGS)

AM_CXXFLAGS="$WARN_CXXFLAGS $PTHREAD_CFLAGS"
AC_SUBST(AM_CXXFLAGS)


##                                     ##
## Output files generated by configure ##
##                                     ##

AC_CONFIG_FILES([
	Makefile
	include/Makefile
	src/Makefile
	src/libside.pc
	tests/Makefile
	tests/static-checker/Makefile
	tests/utils/Makefile
])

AC_OUTPUT


#
# Mini-report on what will be built.
#

AE_PPRINT_INIT
AE_PPRINT_SET_INDENT(1)
AE_PPRINT_SET_TS(38)

AS_ECHO
AS_ECHO("${AE_PPRINT_COLOR_BLDBLU}libside $PACKAGE_VERSION${AE_PPRINT_COLOR_RST}")
AS_ECHO

AE_PPRINT_SUBTITLE([Features])

AE_PPRINT_PROP_STRING([Target architecture], $host_cpu)

report_bindir="`eval eval echo $bindir`"
report_libdir="`eval eval echo $libdir`"

# Print the bindir and libdir this `make install' will install into.
AS_ECHO
AE_PPRINT_SUBTITLE([Install directories])
AE_PPRINT_PROP_STRING([Binaries], [$report_bindir])
AE_PPRINT_PROP_STRING([Libraries], [$report_libdir])
