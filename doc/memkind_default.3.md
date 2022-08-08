---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind_default.3.html"]
title: MEMKIND_DEFAULT
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2014-2022, Intel Corporation)

[comment]: <> (memkind_default.3 -- man page for memkind_default)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memkind_default** - default implementations for memkind operations.

**Note:** This is EXPERIMENTAL API. The functionality and the header file itself can be changed (including non-backward compatible changes), or removed.

# SYNOPSIS #

```c
int memkind_default_create(struct memkind *kind, struct memkind_ops *ops, const char *name);
int memkind_default_destroy(struct memkind *kind);
void *memkind_default_malloc(struct memkind *kind, size_t size);
void *memkind_default_calloc(struct memkind *kind, size_t num, size_t size);
int memkind_default_posix_memalign(struct memkind *kind, void **memptr, size_t alignment, size_t size);
void *memkind_default_realloc(struct memkind *kind, void *ptr, size_t size);
void memkind_default_free(struct memkind *kind, void *ptr);
void *memkind_default_mmap(struct memkind *kind, void *addr, size_t size);
int memkind_default_mbind(struct memkind *kind, void *ptr, size_t len);
int memkind_default_get_mmap_flags(struct memkind *kind, int *flags);
int memkind_default_get_mbind_mode(struct memkind *kind, int *mode);
size_t memkind_default_malloc_usable_size(struct memkind *kind, void *ptr);
int memkind_preferred_get_mbind_mode(struct memkind *kind, int *mode);
int memkind_interleave_get_mbind_mode(struct memkind *kind, int *mode);
int memkind_nohugepage_madvise(struct memkind *kind, void *addr, size_t size);
int memkind_posix_check_alignment(struct memkind *kind, size_t alignment);
int memkind_default_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
void memkind_default_init_once(void);
bool size_out_of_bounds(size_t size);
```

# DESCRIPTION #

Default implementations for memkind operations which include a several useful methods that are not part of the **MEMKIND_DEFAULT** kind which is a fall through to the jemalloc implementation.

`memkind_default_create()`
:   implements the required start up for every kind. If a kind does not point to this function directly for its `ops.create()` operation, then the function that it points to must call `memkind_default_create()` at its start.

`memkind_default_destroy()`
:   implements the required shutdown for every kind. If a kind does not point to this function directly for its `ops.destroy()` operation, then the function that it points to must call `memkind_default_destroy()` at its end.

`memkind_default_malloc()`
:   is a direct call through the jemalloc's `malloc()`.

`memkind_default_calloc()`
:   is a direct call through the jemalloc's `calloc()`.

`memkind_default_posix_memalign()`
:   is a direct call through the jemalloc's `posix_memalign()`.

`memkind_default_realloc()`
:   is a direct call through the jemalloc's `realloc()`.

`memkind_default_free()`
:   is a direct call through the jemalloc's `free()`. Note that this method can be called on any pointer returned by a jemalloc allocation, and in particular, all of the arena allocations described in **memkind_arena**(3) can use this function for freeing.

`memkind_default_mmap()`
:   This  calls the ops->`get_mmap_flags()` operations for the kind, or falls back on the default implementations if the function pointers are *NULL*. The results of these calls are passed to the **mmap**(2) call to allocate pages from the operating system. The addr is the hint passed through to mmap(2) and *size* is the size of the buffer to be allocated. The return value is the allocated buffer or **MAP_FAILED** in the case of an error.

`memkind_default_mbind()`
:   makes calls the kind's `ops.get_mbind_nodemask()` and `ops.get_mbind_mode()` operations to gather inputs and then calls the **mbind**(2) system call using the results along with and user input *ptr* and *len*.

`memkind_default_get_mmap_flags()`
:   sets *flags* to **MAP_PRIVATE | MAP_ANONYMOUS**. See **mmap**(2) for more information about these flags.

`memkind_default_get_mbind_mode()`
:   sets *mode* to **MPOL_BIND**. See **mbind**(2) for more information about this flag.

`memkind_default_malloc_usable_size()`
:   is a direct call through the jemalloc's `malloc_usable_size()`.

`memkind_preferred_get_mbind_mode()`
:   sets *mode* to **MPOL_PREFERRED**.  See **mbind**(2) for more information about this flag.

`memkind_interleave_get_mbind_mode()`
:   sets *mode* to **MPOL_INTERLEAVE**.  See **mbind**(2) for more information about this flag.

`memkind_nohugepage_madvise()`
:   calls **madvise**(2) with the **MADV_NOHUGEPAGE advice**. See **madvise**(2) for more information about this option.

`memkind_posix_check_alignment()`
:   can be used to check the alignment value for `memkind_posix_memalign()` to ensure that is abides by the POSIX requirements: alignment must be a power of 2 at least as large as `sizeof(void*)`.

`memkind_default_get_mbind_nodemask()`
:   wraps jemalloc's **copy_bitmask_to_bitmask**. This function copies body of the bitmask structure into passed pointer.

`memkind_default_init_once()`
:   initializes heap manager.

`size_out_of_bounds()`
:   returns true if given size is out of bounds, otherwise will return false.

## COPYRIGHT ##

Copyright (C) 2014 - 2022 Intel Corporation. All rights reserved.

## SEE ALSO ##

**memkind**(3), **memkind_arena**(3), **memkind_hbw**(3), **memkind_hugetlb**(3), **memkind_pmem**(3), **jemalloc**(3), **mbind**(2), **mmap**(2)
