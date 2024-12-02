# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2024 EfficiOS Inc.

# def.h is include by all compilation units.
AM_CPPFLAGS += \
	-include $(abs_top_srcdir)/src/bin/readside/def.h \
	-ffile-prefix-map=../../../../src/bin/readside=readside \
	-I $(abs_top_srcdir)/src/lib \
	-I $(abs_top_srcdir)/src/bin/readside \
	-I $(abs_top_srcdir)/include

AM_LDFLAGS = -rdynamic

bin_PROGRAMS = readside

readside_SOURCES = \
	argpar.c \
	argpar.h \
	file-type.c \
	file-type.h \
	hash.c \
	hash.h \
	logging.c \
	logging.h \
	panic.c \
	panic.h \
	readers/elf.h \
	readers/elf/elf.c \
	readers/elf/elf32.c \
	readers/elf/elf64.c \
	readers/elf/internal.h \
	readside.c \
	set.c \
	set.h \
	utils.c \
	utils.h \
	visitors/common.c \
	visitors/common.h \
	visitors/json.c \
	visitors/s-expr.c \
	visitors/text.c


readside_LDADD = \
	$(top_builddir)/src/lib/libside-tools/libside-tools.la
