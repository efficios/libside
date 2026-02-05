<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 EfficiOS Inc.
-->

Unit Tests
==========

This directory contains unit tests for libside exercising the various side event
types and the console tracer.

Test programs
-------------

 - test: main unit test exercising all side event types (C).
 - test-cxx: C++ version of the unit tests.
 - test-no-sc: tests without static checker.
 - test-no-sc-cxx: C++ tests without static checker.
 - demo: demonstration program showing basic side event usage.
 - statedump: tests for the state dump functionality.

Console tracer
--------------

All unit tests link against `libside-console-tracer`, which prints side events
to stdout.
