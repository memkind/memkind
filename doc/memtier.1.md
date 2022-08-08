---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memtier.1.html"]
title: MEMTIER
section: 1
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2022, Intel Corporation)

[comment]: <> (memtier.1 -- man page for memtier)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**OPTIONS**](#options)\
[**CAVEATS**](#caveats)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memtier** - redirect malloc calls to memkind memory tiering

# SYNOPSIS #

```c
memtier [ -r ratio ] [ -t thresholds ] program args...
```

# DESCRIPTION #

`memtier`
:   runs the given program, redirecting memory allocations to different memory
    tiers based on a given policy. This allows unmodified programs to run following
    a non-greedy allocation scheme that avoids a performance cliff when DRAM is exhausted.

# OPTIONS #

-r ratio
:   Specifies the ratio as a pair of integers separated by a colon, where the first number
    gives the relative share of near DRAM to be used while the second number gives "far" memory,
    be it CXL memory or PMEM. Eg. "1:4" means that 20% of allocations will use DRAM, 80% PMEM.
    If the ratio is not given, it defaults to the relative share of physically installed memory.
    This version of memtier recognizes only two tiers (DRAM and KMEM_DAX).

-t thresholds
:   Turns on DYNAMIC_THRESHOLD mode where small allocations are served from DRAM and large
    ones from far memory. The syntax is described in depth in man libmemtier,
    eg. -t INIT_VAL=1024,MIN_VAL=512,MAX_VAL=65536 starts with a threshold of 1024,
    then varies it between 512..65536 attempting to match the requested ratio.

-v
:   Displays settings that are passed to libmemtier.

-h
:   Shows a help message then exits.

# CEVEATS #

This tool doesn't expose all features of libmemtier. Only DRAM and KMEM_DAX memory
is supported, to use FS_DAX you need to configure the library directly, please
refer to its man page.

Or alternatively, you can switch that memory to KMEM_DAX mode. For that
purpose, please refer to "man ndctl-reconfigure-namespace"
and "man daxctl-reconfigure-device".

# COPYRIGHT #

Copyright (C) 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**libmemtier**(7), **memkind**(3), **malloc**(3)
