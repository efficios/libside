<!--
SPDX-FileCopyrightText: 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>

SPDX-License-Identifier: CC-BY-4.0
-->

# SIDE-SPECRC-1.0 SIDE Instrumentation ABI Specification

*This document is under heavy construction. Please beware of the
  potholes as you wander through it.*

**RFC 2119**: The key words *MUST*, *MUST NOT*, *REQUIRED*, *SHOULD*,
*SHOULD NOT*, *MAY*, and *OPTIONAL* in this document, when emphasized,
are to be interpreted as described in
[RFC 2119](https://datatracker.ietf.org/doc/html/rfc2119).

**Document identification**: The name of this document (CTF2‑SPECRC‑1.0)
follows the specification of
[CTF2‑DOCID‑2.0](https://diamon.org/ctf/CTF2-DOCID-2.0.html).

## Introduction

The purpose of the SIDE instrumentation ABI specification is to allow
kernel and user-space tracers to attach to static and dynamic
instrumentation of user-space applications.

The SIDE instrumentation ABI key characteristics:

- runtime and language agnostic,
- supports multiple concurrent tracers,
- instrumentation is not specific to a tracer, so there is no need
  to rebuild applications if using a different tracer,
- instrumentation can be either static or dynamic,
- supports complex and nested types,
- supports both static and dynamic types.

The SIDE instrumentation ABI expresses the instrumentation description
as data (no generated code). Instrumentation arguments are passed on the
stack as an array of typed items, along with a reference to the
instrumentation description.

The following ABIs are introduced to let applications declare their
instrumentation and insert instrumentation calls:

- an event description ABI,
- a type description ABI,
- an event and type attribute ABI, which allows associating key-value
  tuples to events and types,
- an ABI defining how applications provide arguments to instrumentation
  calls.

The combination of the type description and type argument ABIs is later
refered to as the SIDE type system.

The ABI exposed to kernel and user-space tracers allow them to list
and connect to the instrumentation, and conditionally enables
instrumentation when at least one tracer is using it.

The type description and type argument ABIs include support for
statically known types and dynamic types. Nested structures, arrays, and
variable-length arrays are supported.

The libside C API is a reference implementation of the SIDE ABI for
instrumentation of C/C++ applications by user-space tracers following
the default calling convention (System V ELF ABI on Linux, MS ABI on
Windows) and eventually by Linux kernel tracers through the User Events
ABI.

A set of macros is provided with the libside C API for convenience of
C/C++ application instrumentation.


## Genesis

The SIDE instrumentaion ABI and libside library learn from the user
feedback about experience with LTTng-UST and Linux kernel tracepoints,
and therefore they introduce significant changes (and vast
simplifications) to the way instrumentation is done compared to
LTTng-UST and Linux kernel tracepoints.

Here is a list of pre-existing userspace instrumentation facilities
along with a discussion of their pros/cons:

- Linux kernel User Events ABI
  - Exposes a stable ABI allowing applications to register their event
    names/field types to the kernel,
  - Can be expected to have a large effect on application instrumentation,
  - My concerns:
    – Should be co-designed with a userspace instrumentation API/ABI rather than only
      focusing on the kernel ABI,
    – Should allow purely userspace tracers to use the same instrumentation as userspace
      tracers implemented within the Linux kernel,
    – Tracers can target their specific use-cases, but infrastructure should be shared,
    – Limit fragmentation of the instrumentation ecosystem.

- Improvements over tracepoints:
  - Improve compiler error reporting vs tracepoints
  - API uses standard header inclusion practices
  - share ABI across runtimes (no need to reimplement tracepoints for
    each language, or to use string only payloads)

- Improvements over SDT: allow expressing additional event semantic
  (e.g.  user attributes, versioning, nested and compound data types)
  - libside has less impact on control flow when disabled (no stack setup)
  - SDT ABI is focused on architecture calling conventions, libside ABI
    is easier to use from runtime environments which have an ABI
    different from the native architecture (golang, rust, python, java).
    libside instrumentation ABI calls a small fixed set of functions.

- Comparison with ETW
  - similar to libside in terms of array of arguments,
  - does not support pre-registration of events (static typing)
    - type information received at runtime from the instrumentation
      callsite.


## Desiderata

- Common instrumentation for kernel and purely userspace tracers,
  - Instrumentation is self-described,
  - Support compound and nested types,
  - Support pre-registration of events,
  - Do not rely on compiled event-specific code,
  - Independent from ELF,
  - Simple ABI for instrumented code, kernel, and user-space tracers,
  - Support concurrent tracers,
  - Expose API to allow dynamic instrumentation libraries to register
    their events/payloads.

- Support statically typed instrumentation

- Support dynamically typed instrumentation
  - Natively cover dynamically-typed languages
  - The support for events with dynamic fields allows lessening the number
    of statically declared events in situation where an application
    possesses seldom-used events with a large variety of parameter types.
  - The support for mixed static and dynamic event fields allows
    implementation of post-processing string formatting along with a
    variadic payload, while keeping trace data in a structured format.

- Performance considerations for userspace tracers.
  - Maintain performance characteristics comparable to existing
    userspace tracers.
  - Low overhead, good scalability when used by userspace tracers.

- Allows tracing user-space through a kernel tracer. Even through it is
  an approach that adds more overhead, it has the benefit of not
  requiring agent threads to be deployed into applications, which is
  useful to trace locked-down processes.

- Instrumentation registration APIs
  - Instrumentation can be generated at runtime
    - dynamic patching,
    - JIT
  - Instrumentation can be declared statically (static instrumentation)
  - Instrumentation can be enabled dynamically.
    - Very low overhead when not in use.

- libside must be extensible in the future.
  - Extension scheme should allow adding new types in the future without
    requiring complex logic to future-proof tracers.
  - Exposed types are invariant,
  - libside ABI and API can be extended by adding new types.

- the side ABI should allow multiple instances and versions within
  a process (e.g. libside for C/C++, Java side ABI, Python side ABI...).

- Both event description and payload are data (no generated text).
  - It allows tracers to directly interpret the event payload from their
    description, removing the need for code generation. This lessens the
    instruction cache pollution compared to code generation approaches.
  - Tracer interpreter for filtering and field capture can directly use
    the instrumentation data, without need for setting up a structured
    argument layout on the stack within the tracer.

- Validation of argument vs event description coherence.

- Passing arguments to events should be:
  - Conveniently express application data structures to be expected as
    instrumentation input.
  - Flexible,
  - Efficient,
  - If all are not possible combined, specialize types for each purpose.

- Allow tracers to passively collect application state transitions.

- Allow tracers to actively sample the current state of an application.

- Error messages generated when misusing the API should be easy to
  comprehend and resolve.

- Allow expressing additional custom semantic augmenting events and
  types.


## Design / Architecture

- Compiler error messages are easy to understand because it is a simple
  header file without any repeated inclusion tricks.

- Variadic events.

- Instrumentation API/ABI:
  – Type system,
    - Type visitor callbacks
      - (perfetto)
    - Stack-copy types
    - Data-gathering types
    - Dynamic types.
  – Helper macros for C/C++,
  – Express instrumentation description as data,
  – Instrumentation arguments are passed on the stack as a data array
    (similar to iovec) along with a reference to instrumentation
    description,
  – Instrumentation is conditionally enabled when at least one tracer is
    registered to it.

- Tracer-agnostic API/ABI:
  – Available events notifications,
  – Conditionally enabling instrumentation,
  – Synchronize registered user-space tracer callbacks with RCU,
  – Co-designed to interact with User Events.

- Application state dump
  - How are applications/libraries meant to provide state information ?
  - How are tracers meant to interact with state dump ?
  - statedump mode polling
  - statedump mode agent thread

- RCU to synchronize userspace tracers registration vs invocation

- How tracers are meant to interact with libside ?

- How is C/C++ language instrumentation is meant to be used ?

- How are dynamic instrumentation facilities meant to interact with
  libside ?

- How is a kernel tracer meant to interact with libside ?

- How is gdb (ptrace) meant to interact with libside ?

- Validation that instrumentation arguments match event description
  fields cannot be done by the compiler, requires either:
  - run time check,
  - static checker (only for static instrumentation).

- Event attributes.

- Type attributes.
