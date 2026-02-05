<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2022 EfficiOS Inc.
-->

libside
=======
Software Instrumentation Dynamically Enabled

Dependencies
------------
 - [librseq](https://github.com/compudj/librseq) at build and run time.

Building
--------

### Prerequisites

This source tree is based on the Autotools suite from GNU to simplify
portability. Here are some things you should have on your system in order to
compile the Git repository tree:

  - **[GNU Autotools](http://www.gnu.org/software/autoconf/)**
  - **[GNU Libtool](https://www.gnu.org/software/libtool/)**
  - **[GNU Make](https://www.gnu.org/software/make/)**
  - **[pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config)**

### Building steps

If you get the tree from the Git repository, you will need to run

    ./bootstrap

in its root. It calls all the GNU tools needed to prepare the tree
configuration.

To build and install, do:

    ./configure
    make
    sudo make install
    sudo ldconfig

Console Tracer
--------------

libside includes a console tracer library (`libside-console-tracer`) for
debugging and testing instrumented applications. When linked with an
application, it registers as a tracer, enables all side events, and prints them
to stdout in a human-readable format.

### Usage

Link your application with `-lside-console-tracer` in addition to `-lside`:

    gcc -o myapp myapp.c -lside -lside-console-tracer

Or use `pkg-config(1)`:

    gcc -o myapp myapp.c $(pkg-config --cflags --libs libside-console-tracer)

The console tracer can also be loaded at runtime using `LD_PRELOAD` without
modifying or relinking the application:

    LD_PRELOAD=libside-console-tracer.so ./myapp

### Example output

    provider: myprovider, event: myevent, fields: { abc: { value: 42 } def: { value: -500 } }
