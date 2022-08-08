---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind_pmem.3.html"]
title: MEMKIND_PMEM
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2015-2022, Intel Corporation)

[comment]: <> (memkind_pmem.3 -- man page for memkind_pmem)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memkind_pmem** - file-backed memory memkind operations.

**Note:** This is EXPERIMENTAL API. The functionality and the header file itself can be changed (including non-backward compatible changes) or removed.

# SYNOPSIS #

```c
int memkind_pmem_create(struct memkind *kind, struct memkind_ops *ops, const char *name);
int memkind_pmem_destroy(struct memkind *kind);
void *memkind_pmem_mmap(struct memkind *kind, void *addr, size_t size);
int memkind_pmem_get_mmap_flags(struct memkind *kind, int *flags);
int memkind_pmem_validate_dir(const char *dir);
```

# DESCRIPTION #

The pmem memory memkind operations enable memory kinds built on memory-mapped files. These support traditional volatile memory allocation in a fashion similar to **libvmem**(3) library. It uses the **mmap**(2) system call to create a pool of volatile memory. Such memory may have different attributes, depending on the file system containing the memory-mapped files. (See also http://pmem.io/pmdk/libvmem).

The pmem kinds are most useful when used with DAX (direct mapping of persistent memory), which is memory-addressable persistent storage that supports load/store access without being paged via the system page cache. A Persistent Memory-aware file system is typically used to provide this type of access.

The most convenient way to create pmem kinds is to use `memkind_create_pmem()` or `memkind_create_pmem_with_config()` (see **memkind**(3)).

`memkind_pmem_create()`
:   is an implementation of the memkind "create" operation for file-backed memory kinds. This allocates a space for some pmem-specific metadata, then calls `memkind_arena_create()` (see **memkind_arena**(3))

`memkind_pmem_destroy()` is an implementation of the memkind "destroy" operation for file-backed memory kinds. This releases all of the resources allocated by `memkind_pmem_create()` and allows the file system space to be reclaimed.

`memkind_pmem_mmap()` allocates the file system space for a block of *size* bytes in the memory-mapped file associated with given kind. The *addr* hint is ignored. The return value is the address of mapped memory region or **MAP_FAILED** in the case of an error.

`memkind_pmem_get_mmap_flags()` sets *flags* to **MAP_SHARED**. See **mmap**(2) for more information about these flags.

`memkind_pmem_validate_dir()` returns zero if file created in specified *pmem_dir* supports DAX (direct mapping of persistent memory) or an error code from the **ERRORS** if not.

MEMKIND_PMEM_CHUNK_SIZE
:   The size of the PMEM chunk size.

## COPYRIGHT ##

Copyright (C) 2015 - 2022 Intel Corporation. All rights reserved.

## SEE ALSO ##

**memkind**(3), **memkind_arena**(3), **memkind_default**(3), **memkind_hbw**(3), **memkind_hugetlb**(3), **libvmem**(3), **jemalloc**(3), **mbind**(2), **mmap**(2)
