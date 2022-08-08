---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind_dax_kmem.3.html"]
title: MEMKIND_DAX_KMEM
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2019-2022, Intel Corporation)

[comment]: <> (memkind_dax_kmem.3 -- man page for memkind_dax_kmem)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memkind_dax_kmem** - DAX KMEM memory memkind operations.

**Note:** This is EXPERIMENTAL API. The functionality and the header file itself can be changed (including non-backward compatible changes) or removed.

# SYNOPSIS #

```c
int memkind_dax_kmem_all_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
```

# DESCRIPTION #

DAX KMEM memory memkind operations.

`memkind_dax_kmem_all_get_mbind_nodemask()`
:   sets the *nodemask* bits to one that correspond to the all persistent memory NUMA nodes in the system. All other bits up to *maxnode* are set to zero. The *nodemask* can be used in conjunction with the **mbind**(2) system call.

## COPYRIGHT ##

Copyright (C) 2019 - 2022 Intel Corporation. All rights reserved.

## SEE ALSO ##

**memkind**(3), **memkind_arena**(3), **memkind_default**(3), **memkind_hugetlb**(3), **memkind_pmem**(3), **jemalloc**(3), **mbind**(2), **mmap**(2)
