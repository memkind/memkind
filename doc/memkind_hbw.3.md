---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind_hbw.3.html"]
title: MEMKIND_HBW
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2014-2022, Intel Corporation)

[comment]: <> (memkind_hbw.3 -- man page for memkind_hbw)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memkind_hbw** - high bandwidth memory memkind operations.

**Note:** This is EXPERIMENTAL API. The functionality and the header file
itself can be changed (including non-backward compatible changes) or removed.

# SYNOPSIS #

```c
int memkind_hbw_check_available(struct memkind *kind);
int memkind_hbw_hugetlb_check_available(struct memkind *kind);
int memkind_hbw_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
int memkind_hbw_get_preferred_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
int memkind_hbw_all_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
void memkind_hbw_init_once(void);
void memkind_hbw_hugetlb_init_once(void);
void memkind_hbw_all_hugetlb_init_once(void);
void memkind_hbw_preferred_init_once(void);
void memkind_hbw_preferred_hugetlb_init_once(void);
void memkind_hbw_interleave_init_once(void);
```

# DESCRIPTION #

High bandwidth memory memkind operations.

`memkind_hbw_check_available()`
:   returns zero if library was able to detect heterogeneous NUMA node bandwidths.
    Returns **MEMKIND_ERROR_UNAVAILABLE** if the detection mechanism failed.
    Detection mechanism can be also overridden by the environment variable
    **MEMKIND_HBW_NODES** as described in the **memkind**(3) man page.

`memkind_hbw_hugetlb_check_available()`
:   In addition to checking for high bandwidth memory as is done by
    `memkind_hbw_check_available()`, this also checks for 2MB huge pages
    as is done by `memkind_hugetlb_check_available_2mb()`.

`memkind_hbw_get_mbind_nodemask()`
:   sets the *nodemask* bit to one that corresponds to the high bandwidth NUMA
    nodes that has the closest NUMA distance to the CPU of the calling process.
    All other bits up to *maxnode* are set to zero. The *nodemask* can be used
    in conjunction with the ***mbind**(2) system call.

**Note:** The function will fail if two or more high bandwidth memory *NUMA* nodes
are in the same the closest *NUMA* distance to the CPU of the calling process.
memkind_hbw_get_preferred_mbind_nodemask() sets the *nodemask* bit to one that
corresponds to the high bandwidth NUMA node that has the closest *NUMA* distance
to the CPU of the calling process. All other bits up to *maxnode* are set to zero.
The *nodemask* can be used in conjunction with the **mbind**(2) system call.

`memkind_hbw_all_get_mbind_nodemask()`
:   sets the *nodemask* bits to one that correspond to the all high bandwidth NUMA
    nodes in the system. All other bits up to *maxnode* are set to zero. The *nodemask*
    can be used in conjunction with the **mbind**(2) system call.

`memkind_hbw_init_once()`
:   This function initializes **MEMKIND_HBW** kind and it should not be called
    more than once. **Note:** `memkind_hbw_init_once()` may reserve some extra memory.

`memkind_hbw_hugetlb_init_once()`
:   This function initializes **MEMKIND_HBW_HUGETLB** kind and it should not
    be called more than once. **Note:** `memkind_hbw_hugetlb_init_once()` may
    reserve some extra memory.

`memkind_hbw_preferred_init_once()`
:   This function initializes **MEMKIND_HBW_PREFERRED** kind and it should not be
    called more than once. **Note:** `memkind_hbw_preferred_init_once()` may reserve
    some extra memory.

`memkind_hbw_preferred_hugetlb_init_once()`
:   This function initializes **MEMKIND_HBW_PREFERRED_HUGETLB** kind and it
    should not be called more than once. **Note:**
    `memkind_hbw_preferred_hugetlb_init_once()` may reserve some extra memory.

`memkind_hbw_all_hugetlb_init_once()`
:   This function initializes **MEMKIND_HBW_ALL_HUGETLB** kind and it should not
    be called more than once. **Note:** `memkind_hbw_all_hugetlb_init_once()`
    may reserve some extra memory.

`memkind_hbw_interleave_init_once()`
:   This function initializes **MEMKIND_HBW_INTERLEAVE** kind and it should not
    be called more than once. **Note:** `memkind_hbw_interleave_init_once()
    may reserve some extra memory.

# COPYRIGHT #

Copyright (C) 2014 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**memkind**(3), **memkind_arena**(3), **memkind_default**(3), **memkind_hugetlb**(3), **memkind_pmem**(3), **jemalloc**(3), **mbind**(2), **mmap**(2)
