---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind_fixed.3.html"]
title: MEMKIND_FIXED
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2021-2022, Intel Corporation)

[comment]: <> (memkind_fixed.3 -- man page for memkind_fixed)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memkind_fixed** - memkind operations on user-defined memory.

**Note:** This is EXPERIMENTAL API. The functionality and the header file itself can
be changed (including non-backward compatible changes) or removed.

# SYNOPSIS #

```c
int memkind_fixed_create(struct memkind *kind, struct memkind_ops *ops, const char *name);
int memkind_fixed_destroy(struct memkind *kind);
void *memkind_fixed_mmap(struct memkind *kind, void *addr, size_t size);
```

# DESCRIPTION #

The fixed memory memkind operations enable memory kinds built on user-defined memory.
Such memory may have different attributes, depending on how the memory mapping was created.

The fixed kinds are most useful when a specific memory characteristics, e.g. numa binding,
which are not fully supported by other memkind API, need to be manipulated.

The most convenient way to create fixed kinds is to use `memkind_create_fixed()`
(see **memkind**(3)).

`memkind_fixed_create()`
:   is an implementation of the memkind "create" operation for kinds that are
    created on the memory area supplied by the user. It allocates space for some
    fixed-kind-specific metadata, then calls `memkind_arena_create()`
    (see **memkind_arena**(3))

`memkind_fixed_destroy()`
:   is an implementation of the memkind "destroy" operation for kinds that are
    created on the memory area supplied by the user. This releases all of the
    resources allocated by `memkind_fixed_create()`, but please note that it
    does not de-allocate the underlying memory.

`memkind_fixed_mmap()`
:   allocates the file system space for a block of *size* bytes in
    the memory-mapped file associated with given kind. The *addr* hint is ignored.
    The return value is the address of mapped memory region or **MAP_FAILED** in
    the case of an error.

# COPYRIGHT #

Copyright (C) 2021 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**memkind**(3), **memkind_arena**(3), **memkind_default**(3), **memkind_hbw**(3), **memkind_hugetlb**(3), **libvmem**(3), **jemalloc**(3), **mbind**(2), **mmap**(2)
