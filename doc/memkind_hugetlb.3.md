---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind_hugetlb.3.html"]
title: MEMKIND_HUGETLB
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2014-2022, Intel Corporation)

[comment]: <> (memkind_hugetlb.3 -- man page for memkind_hugetlb)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memkind_hugetlb** - hugetlb memory memkind operations.

**Note:** This is EXPERIMENTAL API. The functionality and the header file
itself can be changed (including non-backward compatible changes) or removed.

# SYNOPSIS #

```c
int memkind_hugetlb_check_available_2mb(struct memkind *kind);
int memkind_hugetlb_get_mmap_flags(struct memkind *kind, int *flags);
void memkind_hugetlb_init_once(void);
```

# DESCRIPTION #

The hugetlb memory memkind operations enable memory kinds which use the
Linux hugetlbfs file system. For more information about the hugetlbfs
see link below.\
⟨https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt⟩

`memkind_hugetlb_check_available_2mb()`
:   check if there are 2MB pages reserved in the default hugetlbfs. If the
    kind implements `ops.get_mbind_nodemask()`, then only the *NUMA* nodes set
    by the nodemask are checked, otherwise every *NUMA* node is checked.

`memkind_hugetlb_get_mmap_flags()`
:   sets the flags for the **mmap**() system call such that the hugetlbfs
    is utilized for allocations.

`memkind_hugetlb_init_once()`
:   this function initializes **MEMKIND_HUGETLB** kind and it should not
    be called more than once. Note: `memkind_hugetlb_init_once()` may reserve
    some extra memory.

# COPYRIGHT #

Copyright (C) 2014 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**memkind**(3), **memkind_arena**(3), **memkind_default**(3), **memkind_hbw**(3), **memkind_pmem**(3), **jemalloc**(3), **mbind**(2), **mmap**(2)
