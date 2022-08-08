---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind_arena.3.html"]
title: MEMKIND_ARENA
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2014-2022, Intel Corporation)

[comment]: <> (memkind_arena.3 -- man page for memkind_arena)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**MEMORY STATISTICS PRINT OPTIONS**](#memory-statistics-print-options)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memkind_arena** - jemalloc arena allocation memkind operations.\
**Note:** This is **EXPERIMENTAL API**. The functionality and the header file
itself can be changed (including non-backward compatible changes) or removed.

# SYNOPSIS #

```c
int memkind_arena_create(struct memkind *kind, struct memkind_ops *ops, const char *name);
int memkind_arena_create_map(struct memkind *kind, extent_hooks_t *hooks);
int memkind_arena_destroy(struct memkind *kind);
void *memkind_arena_malloc(struct memkind *kind, size_t size);
void *memkind_arena_calloc(struct memkind *kind, size_t num, size_t size);
int memkind_arena_posix_memalign(struct memkind *kind, void **memptr, size_t alignment, size_t size);
void *memkind_arena_realloc(struct memkind *kind, void *ptr, size_t size);
void *memkind_arena_realloc_with_kind_detect(void *ptr, size_t size);
int memkind_thread_get_arena(struct memkind *kind, unsigned int *arena, size_t size);
int memkind_bijective_get_arena(struct memkind *kind, unsigned int *arena, size_t size);
struct memkind *get_kind_by_arena(unsigned arena_ind);
struct memkind *memkind_arena_detect_kind(void *ptr);
int memkind_arena_finalize(struct memkind *kind);
void memkind_arena_init(struct memkind *kind);
void memkind_arena_free(struct memkind *kind, void *ptr);
void memkind_arena_free_with_kind_detect(void *ptr);
int memkind_arena_update_memory_usage_policy(struct memkind *kind, memkind_mem_usage_policy policy);
int memkind_arena_set_max_bg_threads(size_t threads_limit);
int memkind_arena_set_bg_threads(bool state);
int memkind_arena_stats_print(void (*write_cb) (void *, const char *), void *cbopaque, memkind_stat_print_opt opts);
```

# DESCRIPTION #

This header file is a collection of functions can be used to populate the memkind
operations structure for memory kinds that use jemalloc.

`memkind_arena_create()`
:   is an implementation of the memkind "create" operation for memory kinds that use jemalloc.
    This calls `memkind_default_create()` (see `memkind_default(3)`) followed by
    `memkind_arena_create_map()` described below.

`memkind_arena_create_map()`
:   creates the *arena_map* array for the memkind structure pointed to by *kind* which can
    be indexed by the `ops.get_arena()` function from the kind's operations. If get_arena points
    `memkind_thread_get_arena()` then there will be four arenas created for each processor
    and if get_arena points to `memkind_bijective_get_arena()` then just one arena is created.

`memkind_arena_destroy()`
:   is an implementation of the memkind "destroy" operation for memory kinds that use jemalloc.
    This releases all of the resources allocated by `memkind_arena_create()`.

`memkind_arena_malloc()`
:   is an implementation of the memkind "malloc" operation for memory kinds that use jemalloc.
    This allocates memory using the arenas created by `memkind_arena_create()` through the
    jemalloc's `mallocx()` interface. It uses the memkind "get_arena" operation to select the arena.

`memkind_arena_calloc()`
:   is an implementation of the memkind "calloc" operation for memory kinds that use jemalloc.
    This allocates memory using the arenas created by `memkind_arena_create()` through the
    jemalloc's `mallocx()` interface. It uses the memkind "get_arena" operation to select the arena.

`memkind_arena_posix_memalign()`
:   is an implementation of the memkind "posix_memalign" operation for memory kinds that use
    jemalloc. This allocates memory using the arenas created by memkind_arena_create() through
    the jemalloc's `mallocx()` interface. It uses the memkind "get_arena" operation to select
    the arena. The POSIX standard requires that `posix_memalign(3)` may not set *errno* however
    the jemalloc's `malocx()` routine may. In an attempt to abide by the standard *errno* is
    recorded before calling jemalloc's `mallocx()` and then reset after the call.

`memkind_arena_realloc()`
:   is an implementation of the memkind "realloc" operation for memory kinds that use jemalloc.
    This allocates memory using the arenas created by `memkind_arena_create()` through the
    jemalloc's `mallocx()` interface. It uses the memkind "get_arena" operation to select the arena.

`memkind_arena_realloc_with_kind_detect()`
:   function will look up for kind associated to the allocated memory referenced by *ptr*
    and call (depending on kind value) `memkind_arena_realloc()` or `memkind_default_realloc()`.

`memkind_thread_get_arena()`
:   retrieves the *arena* index that is bound to to the calling thread based on a hash of its
    thread ID. The *arena* index can be used with the **MALLOCX_ARENA** macro to set flags for
    jemalloc's `mallocx()`.

`memkind_bijective_arena_get_arena()`
:   retrieves the *arena* index to be used with the MALLOCX_ARENA macro to set flags for
    jemalloc's `mallocx()`. Use of this operation implies that only one arena is used for
    the *kind*.

`memkind_arena_free()`
:   is an implementation of the memkind "free" operation for memory kinds that use jemalloc.
    It causes the allocated memory referenced by *ptr*, which must have been returned by
    a previous call to `memkind_arena_malloc()`, `memkind_arena_calloc()` or
    `memkind_arena_realloc()` to be made available for future allocations. It uses the memkind
    "get_arena" operation to select the arena.

`memkind_arena_free_with_kind_detect()`
:   function will look up for kind associated to the allocated memory referenced by *ptr*
    and call `memkind_arena_free()`.

`memkind_arena_detect_kind()`
:   returns pointer to memory kind structure associated with given allocated memory referenced
    by *ptr*.

`get_kind_by_arena()`
:   returns pointer to memory kind structure associated with given arena.

`memkind_arena_finalize()`
:   is an implementation of the memkind "finalize" operation for memory kinds that use jemalloc.
    This function releases all resources allocated by `memkind_arena_create()` and it's called when
    main() finishes or after calling exit() function.

`memkind_arena_init()`
:   creates arena map with proper hooks per specified kind.

`memkind_arena_update_memory_usage_policy()`
:   function changes time, which determine how fast jemalloc returns unused pages back to
    the operating system, in other words how fast it deallocates file space.

`memkind_arena_set_max_bg_threads()`
sets the maximum number of internal background worker threads in jemalloc. The *threads_limit*
    specify limit of background threads which can be enabled (0 means no limit).

`memkind_arena_set_bg_threads()` enables/disables internal background worker threads in jemalloc.

`memkind_arena_stats_print()`
:   prints summary statistics. This function wraps jemalloc's function `je_malloc_stats_print()`.
    Uses *write_cb* function to print the output. While providing custom writer function,
    use `syscall(2)` rather than `write(2)` Pass *NULL* in order to use the default write_cb
    function which prints the output to the stderr. Use *cbopaque* parameter in order to pass
    some data to your *write_cb* function. Pass additional options using *opts*. For more details
    on *opts* see the MEMORY **STATISTICS PRINT OPTIONS** section below. Returns
    MEMKIND_ERROR_INVALID when failed to parse options string, MEMKIND_SUCCESS on success.

# MEMORY STATISTICS PRINT OPTIONS #

The available options for printing statistics:

MEMKIND_STAT_PRINT_ALL
:   Print all statistics.

MEMKIND_STAT_PRINT_JSON_FORMAT
:   Print statistics in JSON format.

MEMKIND_STAT_PRINT_OMIT_GENERAL
:   Omit general information that never changes during execution.

MEMKIND_STAT_PRINT_OMIT_MERGED_ARENA
:   Omit merged arena statistics.

MEMKIND_STAT_PRINT_OMIT_DESTROYED_MERGED_ARENA
:   Omit destroyed merged arena statistics.

MEMKIND_STAT_PRINT_OMIT_PER_ARENA
:   Omit per arena statistics.

MEMKIND_STAT_PRINT_OMIT_PER_SIZE_CLASS_BINS
:   Omit per size class statistics for bins.

MEMKIND_STAT_PRINT_OMIT_PER_SIZE_CLASS_LARGE
:   Omit per size class statistics for large objects.

MEMKIND_STAT_PRINT_OMIT_MUTEX
:   Omit all mutex statistics.

MEMKIND_STAT_PRINT_OMIT_EXTENT
:   Omit extent statistics.

# COPYRIGHT #

Copyright (C) 2014 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**memkind**(3), **memkind_default**(3), **memkind_hbw**(3), **memkind_hugetlb**(3), **memkind_pmem**(3), **jemalloc**(3), **mbind**(2), **mmap**(2), **syscall**(2), **write**(2)
