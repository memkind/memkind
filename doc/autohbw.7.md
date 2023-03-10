---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["autohbw.7.html"]
title: AUTOHBW
section: 7
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2014-2022, Intel Corporation)

[comment]: <> (autohbw.7 -- man page for autohbw)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**ENVIRONMENT**](#environment)\
[**NOTES**](#notes)\
[**EXAMPLES**](#examples)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**libautohbw.so** - An interposer library for redirecting
heap allocations

# SYNOPSIS #

```c
LD_PRELOAD=libautohbw.so command {arguments ...}
```

# DESCRIPTION #

**AutoHBW** library (`libautohbw.so`)
:   is an interposer library for redirecting heap allocations
(malloc, calloc, realloc, valloc, posix_memalign, memlign) to
high-bandwidth (**HBW**) memory. Consequently, **AutoHBW**
library can be used to automatically allocate high-bandwidth
memory without any modification to source code of an
application.

For instance, the following command-line runs existing binary
*/bin/ls* with **AutoHBW** library, automatically redirecting
heap allocations (larger than a given threshold) to
high-bandwidth memory.\
`LD_PRELOAD=libautohbw.so /bin/ls`

# ENVIRONMENT #

The behavior of **AutoHBW** library is controlled by the
following environment variables.

`AUTO_HBW_SIZE=x:[y]`
:   Indicates that any allocation larger than *x* and smaller
    than *y* should be allocated in HBW memory. *x* and *y* can
    be followed by a K, M, or G to indicate the size
    in Kilo/Memga/Giga bytes (e.g., 4K, 3M, 2G).

**Examples:**

`AUTO_HBW_SIZE=4K`
:   allocations larger than 4K allocated in **HBW**

`AUTO_HBW_SIZE=1M:5M`
:   allocations between 1M and 5M allocated in **HBW**

`AUTO_HBW_LOG=level`
:   Sets the value of logging (printing) level. If *level* is:

+ -1 no messages are printed
+ 0 no allocations messages are printed but INFO messages are
  printed
+ 1 a log message is printed for each allocation (Default)
+ 2 a log message is printed for each allocation with a
  backtrace. Redirect this output and use
  autohbw_get_src_lines.pl to find source lines for each
  allocation. Your application must be compiled with -g to
  see source lines.

**Notes:**

+ Logging adds extra overhead. Therefore, for performance
  critical runs, logging level should be 0
+ The amount of free memory printed with log messages is only
  approximate -- e.g. pages that are not touched yet are
  excluded

**Examples:**

`AUTO_HBW_LOG=1`

`AUTO_HBW_MEM_TYPE=memory_type`
:   Sets the type of memory type that should be automatically
    allocated. By default, this type is
    **MEMKIND_HBW_PREFERRED**, if **MCDRAM** is found in your
    system; otherwise, the default is **MEMKIND_DEFAULT**. The
    names of memory types are defined in
    [**memkind**(3)](/memkind/manpages/memkind.3.html)
    man page. The *memory_type* has to be one of
    **MEMKIND_DEFAULT**, **MEMKIND_HUGETLB**,
    **MEMKIND_INTERLEAVE**, **MEMKIND_HBW**,
    **MEMKIND_HBW_PREFERRED**, **MEMKIND_HBW_HUGETLB**,
    **MEMKIND_HBW_PRE‐FERRED_HUGETLB**,
    **MEMKIND_HBW_INTERLEAVE**

If you are requesting any huge TLB pages, please make sure
that the requested type is currently enabled in your OS.

**Examples:**

`AUTO_HBW_MEM_TYPE=MEMKIND_HBW_PREFERRED`
:   (default, if MCDRAM present)

`AUTO_HBW_MEM_TYPE=MEMKIND_DEFAULT`
:   (default, if MCDRAM absent)

`AUTO_HBW_MEM_TYPE=MEMKIND_HBW_HUGETLB`

`AUTO_HBW_MEM_TYPE=MEMKIND_HUGETLB`

`AUTO_HBW_DEBUG=0|1|2`
:   Set the debug message printing level. Default is 0.
    This is mainly for development.

# NOTES #

It is possible to temporarily disable/enable automatic HBW
allocations by calling `disableAutoHBW()` and `enableAutoHBW()`
in source code. To call these routines, please include
autohbw_api.h header file and link with -lautohbw.

If high-bandwidth memory is not physically present in your
system, the environment variable **MEMKIND_HBW_NODES** must be
set to indicate the high-bandwidth node as indicated in
[**memkind**(3)](/memkind/manpages/memkind.3.html).

When **libautohbw** is loaded with **LD_PRELOAD**, allocations with
size zero, like **malloc**(0), have the same result as the system's
standard library call.
Most notably, a valid pointer may be returned in such calls,
contrary to the default memkind behavior of returning NULL when
size zero is passed to malloc-like functions.

# EXAMPLES #

The following will run */bin/ls* with **AutoHBW** library. Make
sure that paths to both *libautohbw.so* and *libmemkind.so* are
included in **LD_LIBRARY_PATH**.

+ `LD_PRELOAD=libautohbw.so /bin/ls -l`

To run with **MPI**, a shell script must be created, with the
correct **LD_PRELOAD** command for each rank. For example, if we
put `LD_PRELOAD=libautohbw.so /bin/ls` in a shell script named
*autohbw_test.sh*, it can be executed with 2 MPI ranks as:

+ `mpirun -n 2 ./autohbw_test.sh`

# COPYRIGHT #

Copyright (C) 2014 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**memkind**(3), **malloc**(3), **numactl**(8)
