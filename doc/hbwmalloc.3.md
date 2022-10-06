---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["hbwmalloc.3.html"]
title: HBWMALLOC
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2014-2022, Intel Corporation)

[comment]: <> (hbwmalloc.3 -- man page for hbwmalloc)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**RETURN VALUE**](#return-value)\
[**ERRORS**](#errors)\
[**NOTES**](#notes)\
[**UTILS**](#utils)\
[**ENVIRONMENT**](#environment)\
[**SYSTEM CONFIGURATION**](#system-configuration)\
[**KNOWN ISSUES**](#known-issues)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**hbwmalloc** - The high bandwidth memory interface.

**Note:** *hbwmalloc.h* functionality is considered as a stable API (STANDARD API).

# SYNOPSIS #

```c
#include <hbwmalloc.h>

Link with -lmemkind

int hbw_check_available(void);
void* hbw_malloc(size_t size);
void* hbw_calloc(size_t nmemb, size_t size);
void* hbw_realloc (void *ptr, size_t size);
void hbw_free(void *ptr);
size_t hbw_malloc_usable_size(void *ptr);
int hbw_posix_memalign(void **memptr, size_t alignment, size_t size);
int hbw_posix_memalign_psize(void **memptr, size_t alignment, size_t size, hbw_pagesize_t pagesize);
hbw_policy_t hbw_get_policy(void);
int hbw_set_policy(hbw_policy_t mode);
int hbw_verify_memory_region(void *addr, size_t size, int flags);
```

# DESCRIPTION #

`hbw_check_available()`
:   returns zero if high bandwidth memory is available or an
  error code described in the [ERRORS](#errors) section if not.

`hbw_malloc()`
:   allocates *size* bytes of uninitialized high bandwidth
  memory. The allocated space is suitably aligned (after possible
  pointer coercion) for storage of any type of object.
  If *size* is zero, then `hbw_malloc()` returns either NULL or
  a valid ptr, depending on the system's standard library
  malloc(0) behavior.

`hbw_calloc()`
:   allocates space for *nmemb* objects in high bandwidth
  memory, each *size* bytes in length. The result is identical
  to calling `hbw_malloc()` with an argument of *nmemb* * *size*,
  with the exception that the allocated memory is explicitly
  initialized to zero bytes.
  If *nmemb* or *size* is zero, then `hbw_calloc()` returns
  either NULL or a valid ptr, depending on the system's
  standard library malloc(0) behavior.

`hbw_realloc()`
:   changes the size of the previously allocated high bandwidth
    memory referenced by *ptr* to *size* bytes. The contents of the
    memory remain unchanged up to the lesser of the new and old
    sizes. If the new size is larger, the contents of the newly
    allocated portion of the memory are undefined. Upon success,
    the memory referenced by *ptr* is freed and a pointer to the
    newly allocated high bandwidth memory is returned.
    **Note:** `hbw_realloc()` may move the memory allocation,
    resulting in a different return value than *ptr*.\
    If *ptr* is NULL, the `hbw_realloc()` function behaves
    identically to `hbw_malloc()` for the specified size. If
    *size* is equal to zero and *ptr* is not NULL, then the call
    is equivalent to `hbw_free(ptr)` and NULL is returned. The
    address *ptr*, if not NULL, was returned by a previous call
    to `hbw_malloc()`, `hbw_calloc()`, `hbw_realloc()` or
    `hbw_posix_memalign()`. Otherwise, or if `hbw_free(ptr)` was
    called before, undefined behavior occurs.
    **Note:** `hbw_realloc()` cannot be used with a pointer
    returned by `hbw_posix_memalign_psize()`.

`hbw_free()`
:   causes the allocated memory referenced by *ptr* to be made
    available for future allocations. If *ptr* is NULL, no action
    occurs. In other case the address *ptr*, if not NULL, must have
    been returned by a previous call to `hbw_malloc()`,
    `hbw_calloc()`, `hbw_realloc()`, `hbw_posix_memalign()` or
    `hbw_posix_memalign_psize()`. Otherwise, if `hbw_free(ptr)` was
    called before, undefined behavior occurs.

`hbw_malloc_usable_size()`
:   returns the number of usable bytes in the block pointed to
    by *ptr*, a pointer to a block of memory allocated by
    `hbw_malloc()`, `hbw_calloc()`, `hbw_realloc()`,
    `hbw_posix_memalign()`, or `hbw_posix_memalign_psize()`.

`hbw_posix_memalign()`
:   allocates *size* bytes of high bandwidth memory such that
    the allocation’s base address is an even multiple of
    *alignment*, and returns the allocation in the value pointed to
    by *memptr*. The requested alignment must be a power of 2 at
    least as large as *sizeof(void)*. If *size* is 0, then
    `hbw_posix_memalign()` returns 0, with a NULL returned in
    *memptr*. See the [**ERRORS**](#errors) section for other
    possible return values.

`hbw_posix_memalign_psize()`
:   allocates *size* bytes of high bandwidth memory such that
    the allocation’s base address is an even multiple of
    *alignment*, and returns the allocation in the value pointed to
    by *memptr*. The requested *alignment* must be a power of 2 at
    least as large as *sizeof(void)*. The memory will be allocated
    using pages determined by the *pagesize* variable which may be
    one of the following enumerated values:

+ **HBW_PAGESIZE_4KB**\
 The four kilobyte page size option. Note that with transparent
 huge pages enabled these allocations may be promoted by the
 operating system to two megabyte pages.

+ **HBW_PAGESIZE_2MB**\
 The two megabyte page size option.
 **Note:** This page size requires huge pages configuration
 described in the [SYSTEM CONFIGURATION](#system-configuration)
 section.

 **Note:** **HBW_PAGESIZE_2MB**, option is not supported with the
 **HBW_POLICY_INTERLEAVE** policy which is described below.

`hbw_set_policy()`
:   sets the current fallback policy. The policy can be modified
    only once in the lifetime of an application and before calling
    any of: `hbw_malloc()`, `hbw_calloc()`, `hbw_realloc()`,
    `hbw_posix_memalign()`, or `hbw_posix_memalign_psize()`
    functions.
    **Note:** If the policy is not set, then
    **HBW_POLICY_PREFERRED** will be used by default.

+ **HBW_POLICY_BIND**\
 If insufficient high bandwidth memory from the nearest NUMA
 node is available to satisfy a request, the allocated pointer
 is set to NULL and *errno* is set to **ENOMEM**. If
 insufficient high bandwidth memory pages are available at the
 fault time the *Out Of Memory (OOM) Killer* is triggered. Note
 that pages are faulted exclusively from the high bandwidth NUMA
 node nearest at the time of allocation, not at the time of
 fault.

+ **HBW_POLICY_BIND_ALL**\
 If insufficient high bandwidth memory is available to satisfy a
 request, the allocated pointer is set to NULL and *errno* is
 set to **ENOMEM**. If insufficient high bandwidth memory pages
 are available at the fault time the *Out Of Memory (OOM)
 Killer* is triggered. Note that pages are faulted from the high
 bandwidth NUMA nodes. Nearest NUMA node is selected at the time
 of the page fault.

+ **HBW_POLICY_PREFERRED**\
 If insufficient memory is available from the high bandwidth
 NUMA node closest at the allocation time, fall back to standard
 memory (default) with the smallest NUMA distance.

+ **HBW_POLICY_INTERLEAVE**\
 Interleave faulted pages from across all high bandwidth NUMA
 nodes using standard size pages (the Transparent Huge Page
 feature is disabled).

`hbw_get_policy()`
:   returns the current fallback policy when insufficient high
bandwidth memory is available.

`hbw_verify_memory_region()`
:   verifies if a memory region fully falls into high bandwidth
    memory. Returns 0 if memory address range from *addr* to
    *addr +* *size* is allocated in high bandwidth memory, -1
    if any fragment of memory was not backed by high bandwidth memory
    (e.g. when memory is not initialized) or one of error codes
    described in the [ERRORS](#errors) section.
    **Notes:** Using this function in production code may result in
    a serious performance penalty.
    **The *flags* argument may includeoptional flags that modify function behavior:**

+ **HBW_TOUCH_PAGES**\
 Before checking pages, function will touch the first byte of all
 pages in the address range starting from *addr* to *addr +* *size*
 by reading and writing (so the content will be overwritten by the
 same data as has been read). Using this option may trigger the
 *Out Of Memory Killer*.

# RETURN VALUE #

`hbw_get_policy()` returns **HBW_POLICY_BIND**, **HBW_POLICY_BIND_ALL**,
**HBW_POLICY_PREFERRED** or **HBW_POLICY_INTERLEAVE** which represents
the current high bandwidth policy. `hbw_free()` do not have a return value.
`hbw_malloc()`, `hbw_calloc()` and `hbw_realloc()` return the pointer to
the allocated memory or NULL if the request fails. `hbw_posix_memalign()`,
`hbw_posix_memalign_psize()` and `hbw_set_policy()` return zero on success
and return an error code as described in the [ERRORS](#errors) section
below on failure.

# ERRORS #

Error codes described here are the POSIX standard error codes as
defined in <*errno.h*>

`hbw_check_available()`
:   returns **ENODEV** if high bandwidth memory is unavailable.

**`hbw_posix_memalign()`** and **`hbw_posix_memalign_psize()`**\
If the *alignment* parameter is not a power of two, or was not
a multiple of *sizeof(void*)*, then **EINVAL** is returned.
If the policy and pagesize combination is unsupported then
**EINVAL** is returned. If there was insufficient memory to
satisfy the request then **ENOMEM** is returned.

`hbw_set_policy()`
:   returns **EPERM** if hbw_set_policy() was called more than
    once, or **EINVAL** if mode argument was neither **HBW_POLICY_PREFERRED**,
    **HBW_POLICY_BIND**, **HBW_POLICY_BIND_ALL** nor **HBW_POLICY_INTERLEAVE**.

`hbw_verify_memory_region()`
:   returns **EINVAL** if *addr* is NULL, *size* equals 0 or flags
    contained an unsupported bit set. If memory pointed by *addr* could
    not be verified then **EFAULT** is returned.

# NOTES #

The <*hbwmalloc.h*> file defines the external functions and enumerations
for the hbwmalloc library. These interfaces define a heap manager that
targets high bandwidth memory numa nodes.

# UTILS #

*/usr/bin/memkind-hbw-nodes*
:   Prints a comma-separated list of high bandwidth nodes.

# ENVIRONMENT #

MEMKIND_HBW_NODES
:   This environment variable is a comma-separated list of NUMA
    nodes that are treated as high bandwidth. Uses the *libnuma*
    routine `numa_parse_nodestring()` for parsing, so the syntax
    described in the `numa(3)` man page for this routine applies,
    for example: *1-3,5* is a valid setting.

MEMKIND_ARENA_NUM_PER_KIND
:   This environment variable allows leveraging internal mechanism
    of the library for setting number of arenas per kind. Value should
    be a positive integer (not greater than **INT_MAX** defined in
    <*limits.h*>). The user should set the value based on the
    characteristics of the application that is using the library.
    Higher value can provide better performance in extremely multithreaded
    applications at the cost of memory overhead. See section
    **IMPLEMENTATION NOTES** of **jemalloc**(3) for more details about
    arenas.

MEMKIND_HEAP_MANAGER
:   Controls heap management behavior in the memkind library by
switching to one of the available heap managers.\
Possible values are:

+ JEMALLOC - sets the *jemalloc* heap manager
+ TBB - sets the *Intel Threading Building Blocks* heap manager.
  This option requires installed
  *Intel Threading Building Blocks* library.

**Note:** If the **MEMKIND_HEAP_MANAGER** is not set then the
*jemalloc* heap manager will be used by default.

# SYSTEM CONFIGURATION #

HUGETLB (huge pages)
:   Current number of "persistent" huge pages can be read from the
    */proc/sys/vm/nr_hugepages* file. The proposed way of setting
    hugepages is: `sudo sysctl vm.nr_hugepages=<number_of_hugepages>`.
    More information can be found here:
    https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt

Interfaces for obtaining 2MB (*HUGETLB*) memory need allocated huge
pages in the kernel’s huge page pool.

# KNOWN ISSUES #

HUGETLB (huge pages)
:   There might be some overhead in huge pages consumption caused
    by heap management. If your allocation fails because of the OOM,
    please try to allocate extra huge pages (e.g. 8 huge pages).

# COPYRIGHT #

Copyright (C) 2014 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**malloc**(3), **numa**(3), **jemalloc**(3), **memkind**(3)
