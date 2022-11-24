#!/bin/bash
#
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2022 EfficiOS Inc.

# This file is meant to be sourced at the start of shell script-based tests.


# Error out when encountering an undefined variable
set -u

# If "readlink -f" is available, get a resolved absolute path to the
# tests source dir, otherwise make do with a relative path.
scriptdir="$(dirname "${BASH_SOURCE[0]}")"
if readlink -f "." >/dev/null 2>&1; then
	testsdir=$(readlink -f "$scriptdir/..")
else
	testsdir="$scriptdir/.."
fi

# Allow overriding the source and build directories
if [ "x${SIDE_TESTS_SRCDIR:-}" = "x" ]; then
	SIDE_TESTS_SRCDIR="$testsdir"
fi
export SIDE_TESTS_SRCDIR

if [ "x${SIDE_TESTS_BUILDDIR:-}" = "x" ]; then
	SIDE_TESTS_BUILDDIR="$testsdir"
fi
export SIDE_TESTS_BUILDDIR

# By default, it will not source tap.sh.  If you to tap output directly from
# the test script, define the 'SH_TAP' variable to '1' before sourcing this
# script.
if [ "x${SH_TAP:-}" = x1 ]; then
	# shellcheck source=./tap.sh
	. "${SIDE_TESTS_SRCDIR}/utils/tap.sh"
fi
