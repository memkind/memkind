---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind.3.html"]
title: MEMKIND
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2014-2023, Intel Corporation)

[comment]: <> (memkind.3 -- man page for memkind)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**RETURN VALUE**](#return-value)\
[**KINDS**](#kinds)\
[**MEMORY TYPES**](#memory-type)\
[**MEMORY BINDING POLICY**](#memory-binding-policy)\
[**MEMORY FLAGS**](#memory-flags)\
[**MEMORY USAGE POLICY**](#memory-usage-policy)\
[**MEMORY STATISTICS TYPE**](#memory-statistics-type)\
[**MEMORY STATISTICS PRINT OPTIONS**](#memory-statistics-print-options)\
[**ERRORS**](#errors)\
[**UTILS**](#utils)\
[**ENVIRONMENT**](#environment)\
[**SYSTEM CONFIGURATION**](#system-configuration)\
[**STATIC LINKING**](#static-links)\
[**KNOWN ISSUES**](#known-issues)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memkind** - A heap manager that enables allocations to memory with
different properties.

This header expose a STANDARD and an EXPERIMENTAL API. API Standards
are described below in this man page.

# SYNOPSIS #

```c
#include <memkind.h>

Link with -lmemkind
```
**EXPERIMENTAL API:**
```c
HEAP MANAGEMENT:
int memkind_posix_memalign(memkind_t kind, void **memptr, size_t alignment, size_t size);

KIND MANAGEMENT:
int memkind_create_kind(memkind_memtype_t memtype_flags, memkind_policy_t policy, memkind_bits_t flags, memkind_t *kind);
```

**STANDARD API:**
```c
ERROR HANDLING:
void memkind_error_message(int err, char *msg, size_t size);

LIBRARY VERSION:
int memkind_get_version();

HEAP MANAGEMENT:
void *memkind_malloc(memkind_t kind, size_t size);
void *memkind_calloc(memkind_t kind, size_t num, size_t size);
void *memkind_realloc(memkind_t kind, void *ptr, size_t size);
void memkind_free(memkind_t kind, void *ptr);
size_t memkind_malloc_usable_size(memkind_t kind, void *ptr);
void *memkind_defrag_reallocate(memkind_t kind, void *ptr);
memkind_t memkind_detect_kind(void *ptr);

KIND CONFIGURATION MANAGEMENT:
struct memkind_config *memkind_config_new();
void memkind_config_delete(struct memkind_config *cfg);
void memkind_config_set_path(struct memkind_config *cfg, const char *pmem_dir);
void memkind_config_set_size(struct memkind_config *cfg, size_t pmem_size);
void memkind_config_set_memory_usage_policy(struct memkind_config *cfg, memkind_mem_usage_policy policy);

KIND MANAGEMENT:
int memkind_create_fixed(void *addr, size_t size, memkind_t *kind);
int memkind_create_pmem(const char *dir, size_t max_size, memkind_t *kind);
int memkind_create_pmem_with_config(struct memkind_config *cfg, memkind_t *kind);
int memkind_destroy_kind(memkind_t kind);
int memkind_check_available(memkind_t kind);
ssize_t memkind_get_capacity(memkind_t kind);
int memkind_check_dax_path(const char *pmem_dir);
void memkind_set_allow_zero_allocs(memkind_t kind, bool allow_zero_allocs);

STATISTICS:
int memkind_update_cached_stats(void);
int memkind_get_stat(memkind_t kind, memkind_stat stat, size_t *value);
int memkind_stats_print(void (*write_cb) (void *, const char *), void *cbopaque, memkind_stat_print_opt opts);

DECORATORS:
void memkind_malloc_pre(memkind_t *kind, size_t *size);
void memkind_malloc_post(memkind_t kind, size_t size, void **result);
void memkind_calloc_pre(memkind_t *kind, size_t *nmemb, size_t *size);
void memkind_calloc_post(memkind_t kind, size_t nmemb, size_t size, void **result);
void memkind_posix_memalign_pre(memkind_t *kind, void **memptr, size_t *alignment, size_t *size);
void memkind_posix_memalign_post(memkind_t kind, void **memptr, size_t alignment, size_t size, int *err);
void memkind_realloc_pre(memkind_t *kind, void **ptr, size_t *size);
void memkind_realloc_post(memkind_t *kind, void *ptr, size_t size, void **result);
void memkind_free_pre(memkind_t *kind, void **ptr);
void memkind_free_post(memkind_t kind, void *ptr);

ENHANCEMENTS:
int memkind_set_bg_threads(bool state);
```

# DESCRIPTION #

#### ERROR HANDLING ####

`void memkind_error_message(int err, char *msg, size_t size)`
:   converts an error number err returned by a member of the memkind interface
    to an error message *msg* where the maximum size of the message is passed by the
    *size* parameter.

#### HEAP MANAGEMENT ####

The functions described in this section define a heap manager with an interface
modeled on the ISO C standard API’s, except that the user must specify the kind
of memory with the first argument to each function. See the [**KINDS**](#kinds)
section below for a full description of the implemented kinds. For the file-backed
kind of memory see `memkind_create_pmem()` or `memkind_create_pmem_with_config()`.
For the memory kind created on user-specified area, please check `memkind_create_fixed()`.

`void *memkind_malloc(memkind_t kind, size_t size)`
:   allocates *size* bytes of uninitialized memory of the specified *kind*. The allocated
    space is suitably aligned (after possible pointer coercion) for storage of any type of
    object. If *size* is 0, then `memkind_malloc()` returns *NULL*.

`void *memkind_calloc(memkind_t kind, size_t num, size_t size)`
:   allocates space for *num* objects each *size* bytes in length in memory of the
    specified *kind*. The result is identical to calling `memkind_malloc()` with an argument
    of *num* * *size*, with the exception that the allocated memory is explicitly initialized
    to zero bytes. If *num* or *size* is 0, then `memkind_calloc()` returns *NULL*.

`void *memkind_realloc(memkind_t kind, void *ptr, size_t size)`
:   changes the size of the previously allocated memory referenced by *ptr* to *size* bytes
    of the specified *kind*. The contents of the memory remain unchanged up to the lesser
    of the new and old sizes. If the new size is larger, the contents of the newly allocated
    portion of the memory are undefined. Upon success, the memory referenced by *ptr* is
    freed and a pointer to the newly allocated memory is returned.
    **Note:** `memkind_realloc()` may move the memory allocation, resulting in a different
    return value than *ptr*.
    \
    If *ptr* is *NULL*, the `memkind_realloc()` function behaves identically to
    `memkind_malloc()` for the specified size. If *size* is equal to zero, and
    *ptr* is not *NULL*, then the call is equivalent to `memkind_free(kind, ptr)`
    and *NULL* is returned. The address *ptr*, if not *NULL*, must have been returned
    by a previous call to `memkind_malloc()`, `memkind_calloc()`, `memkind_realloc()`,
    `memkind_defrag_reallocate()` or `memkind_posix_memalign()` with the same *kind*
    as specified to the call to `memkind_realloc()`. Otherwise, if `memkind_free(kind, ptr)`
    was called before, undefined behavior occurs. In cases where the kind is unknown in the
    context of the call to `memkind_realloc()` *NULL* can be given as the *kind* specified
    to `memkind_realloc()`, but this will require an internal lookup for a correct kind.
    **Note:** The lookup for *kind* could result in a serious performance penalty, which
    can be avoided by specifying a correct *kind*. If both *kind* and *ptr* are *NULL*,
    then `memkind_realloc()` returns *NULL* and sets *errno* to **EINVAL**.

`int memkind_posix_memalign(memkind_t kind, void **memptr, size_t alignment, size_t size)`
:   allocates *size* bytes of memory of a specified *kind* such that the allocation’s
    base address is an even multiple of *alignment*, and returns the allocation in the value
    pointed to by *memptr*. The requested *alignment* must be a power of 2 at least as large
    as *sizeof(void*)*. If *size* is 0, then `memkind_posix_memalign()` returns 0, with a
    *NULL* returned in *memptr*.

`size_t memkind_malloc_usable_size(memkind_t kind, void *ptr)`
:   function provides the same semantics as `malloc_usable_size(3)`, but operates
    on the specified *kind*.
    **NOTE:** In cases where the kind is unknown in the context of the call to
    `memkind_malloc_usable_size()` *NULL* can be given as the *kind*, but this could
    require an internal lookup for correct kind.
    `memkind_malloc_usable_size()` is supported by the TBB heap manager described in the
    [ENVIRONMENT](#environment) section, since Intel TBB 2019 Update 4.

`void *memkind_defrag_reallocate(memkind_t kind, void *ptr)`
:   reallocates the object conditionally inside the specific *kind*. The function
    determines if it’s worthwhile to move allocation to the reduce degree of external
    fragmentation of the heap. In case of failure function returns *NULL*, otherwise
    function returns a pointer to reallocated memory and memory referenced by *ptr*
    was released and should not be accessed. If *ptr* is *NULL*, then
    `memkind_defrag_reallocate()` returns *NULL*. In cases where the *kind* is unknown
    in the context of the call to `memkind_defrag_reallocate()` *NULL* can be given
    as the *kind* specified to `memkind_defrag_reallocate()`, but this will require
    an internal lookup for the correct *kind*.
    **Note:** The lookup for *kind* could result in a serious performance penalty,
    which can be avoided by specifying a correct *kind*.

`memkind_t memkind_detect_kind(void *ptr)`
:   returns the kind associated with allocated memory referenced by *ptr*.
    This pointer must have been returned by a previous call to `memkind_malloc()`,
    `memkind_calloc()`, `memkind_realloc()`, `memkind_defrag_reallocate()` or
    `memkind_posix_memalign()`. If *ptr* is *NULL*, then `memkind_detect_kind()`
    returns *NULL*.
    **Note:** This function has non-trivial performance overhead.

`void memkind_free(memkind_t kind, void *ptr)`
:   causes the allocated memory referenced by *ptr* to be made available for
    future allocations. This pointer must have been returned by a previous call to
    `memkind_malloc()`, `memkind_calloc()`, `memkind_realloc()`,
    `memkind_defrag_reallocate()` or `memkind_posix_memalign()`. Otherwise, if
    `memkind_free(*kind*, *ptr*)` has already been called before, undefined behavior
    occurs. If *ptr* is *NULL*, no operation is performed. In cases where the kind
    is unknown in the context of the call to `memkind_free()` *NULL* can be given
    as the *kind*, but this will require an internal lookup for correct kind.
    **Note:** The lookup for *kind* could result in a serious
    performance penalty, which can be avoided by specifying a correct *kind*.

#### KIND CONFIGURATION MANAGEMENT ####

The functions described in this section define a way to create, delete and update
kind specific configuration. Except of `memkind_config_new()`, user must specify
the memkind configuration with the first argument to each function. API described
here is most useful with file-backed kind of memory, e.g.
`memkind_create_pmem_with_config()` method.

`struct memkind_config *memkind_config_new()`
:   creates the memkind configuration.

`void memkind_config_delete(struct memkind_config *cfg)`
:   deletes previously created memkind configuration, which must have been
    returned by a previous call to `memkind_config_new()`.

`void memkind_config_set_path(struct memkind_config *cfg, const char *pmem_dir)`
:   updates the memkind *pmem_dir* configuration parameter, which specifies
    the directory path, where file-backed kind of memory will be created.
    **Note:** This function does not validate that *pmem_dir* specifies a valid path.

`void memkind_config_set_size(struct memkind_config *cfg, size_t pmem_size)`
:   updates the memkind *pmem_size* configuration parameter, which allows to limit
    the file-backed kind memory partition.
    **Note:** This function does not validate that *pmem_size* is in valid range.

`void memkind_config_set_memory_usage_policy(struct memkind_config *cfg, memkind_mem_usage_policy policy)`
:   updates the memkind *policy* configuration parameter, which allows to tune up
    memory utilization. The user should set the value based on the characteristics of
    the application that is using the library (e.g. prioritize memory usage, CPU utilization),
    for more details about *policy* see the [MEMORY USAGE POLICY](#memory-usage-policy)
    section below.
    **Note:** This function does not validate that *policy* is in valid range.

#### KIND MANAGEMENT ####

There are built-in kinds that are always available and these are enumerated in
the [KINDS](#kinds) section. The user can also create their own kinds of memory.
This section describes the API’s that enable the tracking of the different kinds
of memory and determining their properties.

`int memkind_create_fixed(void *addr, size_t size, memkind_t *kind)`
:   is a function used to create a kind under user-specified area of memory.
    The memory can be allocated in any possible way, e.g. it might be a static array
    or an mmapped area. User can specify any properties using functions such as mbind.
    User is also responsible for de-allocation of memory after the kind destruction.
    The memory area must remain valid until fixed_kind is destroyed. The area starts
    at address *addr* and has size *size*. When heap manager runs out of memory
    (located under user-specified area), a call to **memkind_malloc()** returns
    *NULL* and **errno** is set to **ENOMEM**.

`int memkind_create_pmem(const char *dir, size_t max_size, memkind_t *kind)`
:   is a convenient function used to create a file-backed kind of memory.
    It allocates a temporary file in the given directory *dir*. The file is
    created in a fashion similar to **tmpfile(3)**, so that the file name does
    not appear when the directory is listed and the space is automatically
    freed when the program terminates. The file is truncated to a size of
    *max_size* bytes and the resulting space is memory-mapped.
    Note that the actual file system space is not allocated immediately,
    but only on a call to `memkind_pmem_mmap()` (see memkind_pmem(3)). This
    allows to create a pmem memkind of a pretty large size without the need
    to reserve in advance the corresponding file system space for the entire
    heap. If the value of *max_size* equals 0, pmem memkind is only limited
    by the capacity of the file system mounted under *dir* argument. The minimum
    *max_size* value which allows to limit the size of kind by the library is
    defined as **MEMKIND_PMEM_MIN_SIZE**. Calling `memkind_create_pmem()`
    with a size smaller than that and different than 0 will return an error.
    The maximum allowed size is not limited by **memkind**, but by the file
    system specified by the *dir* argument. The *max_size* passed in is the
    raw size of the memory pool and **jemalloc** will use some of that space
    for its own metadata. Returns zero if the pmem memkind is created successfully
    or an error code from the [ERRORS](#errors) section if not.

`int memkind_create_pmem_with_config(struct memkind_config *cfg, memkind_t *kind)`
:   is a second function used to create a file-backed kind of memory.
    Function behaves similar to `memkind_create_pmem()` but instead of passing
    *dir* and *max_size* arguments, it uses *config* param to specify
    characteristics of created file-backed kind of memory
    (see [**KIND CONFIGURATION MANAGEMENT**](#kind-configuration-managment) section).

`int memkind_create_kind(memkind_memtype_t memtype_flags, memkind_policy_t policy, memkind_bits_t flags, memkind_t *kind)`
:   creates kind that allocates memory with specific memory type, memory
    binding policy and flags (see [MEMORY FLAGS](#memory-flags) section).
    The *memtype_flags* (see [MEMORY TYPES](#memory-types) section) determine
    memory types to allocate, *policy* argument is policy for specifying page
    binding to memory types selected by *memtype_flags*. Returns zero if the
    specified kind is created successfully or an error code from the [ERRORS](#errors)
    section if not.

`int memkind_destroy_kind(memkind_t kind)`
:   destroys previously created kind object, which must have been returned by
    a previous call to `memkind_create_pmem()`, `memkind_create_pmem_with_config()`
    or `memkind_create_kind()`. Otherwise, or if `*memkind_destroy_kind(kind)*`
    has already been called before, undefined behavior occurs. Note that, when
    the kind was returned by `memkind_create_kind()` all allocated memory must be
    freed before kind is destroyed, otherwise this will cause memory leak. When the
    kind was returned by `memkind_create_pmem()` or `memkind_create_pmem_with_config()`
    all allocated memory will be freed after kind will be destroyed.

`int memkind_check_available(memkind_t kind)`
:   returns zero if the specified *kind* is available or an error code from the
    [ERRORS](#errors) section if it is not.

`ssize_t memkind_get_capacity(memkind_t kind)`
:   returns memory capacity of nodes available to a given kind (file size or
    filesystem capacity in case of a file-backed PMEM kind; total area size in the
    case of fixed-kind) or -1 in case of an error. Supported kinds are:
    **MEMKIND_DEFAULT, MEMKIND_HIGHEST_CAPACITY, MEMKIND_HIGHEST_CAPACITY_LOCAL, MEMKIND_LOWEST_LATENCY_LOCAL, MEMKIND_HIGHEST_BANDWIDTH_LOCAL, MEMKIND_HUGETLB, MEMKIND_INTERLEAVE, MEMKIND_HBW, MEMKIND_HBW_ALL, MEMKIND_HBW_INTERLEAVE, MEMKIND_DAX_KMEM, MEMKIND_DAX_KMEM_ALL, MEMKIND_DAX_KMEM_INTERLEAVE, MEMKIND_REGULAR**,
    file-backed PMEM and fixed-kind. *kind*. For **MEMKIND_HUGETLB** only pages with a
    default size of 2MB are supported.

`int memkind_check_dax_path(const char *pmem_dir)`
:   returns zero if file-backed kind memory is in the specified directory path
    *pmem_dir*. Otherwise, it can be created with the DAX attribute or an error code
    from the [ERRORS](#errors) section.

`void memkind_set_allow_zero_allocs(memkind_t kind, bool allow_zero_allocs)`
:   for a given *kind*, determines the behavior of malloc-like functions when size passed
    to them is equal to zero.
    These functions return a valid pointer when *allow_zero_allocs* is set to true,
    return NULL when set to false (default memkind behavior).

**MEMKIND_PMEM_MIN_SIZE** The minimum size which allows to limit the file-backed
memory partition.

#### STATISTICS ####

The functions described in this section define a way to get specific memory
allocation statistics.

`int memkind_update_cached_stats(void)`
:   is used to force an update of cached dynamic allocator statistics. Statistics
    are not updated real-time by memkind library and this method allows to force its
    update.

`int memkind_get_stat(memkind_t kind, memkind_stat stat, size_t *value)`
:   retrieves statistic of the specified type and returns it in *value*. Measured
    statistic applies to the specific *kind*, when *NULL* is given as *kind* then
    statistic applies to memory used by the whole memkind library.
    **Note:** You need to call `memkind_update_cached_stats()` before calling
    `memkind_get_stat()` because statistics are cached by the memkind library.

`int memkind_stats_print(void (*write_cb) (void *, const char *), void *cbopaque, memkind_stat_print_opt opts)`
:   prints summary statistics. This function wraps the jemalloc’s function
    `je_malloc_stats_print()`. Uses *write_cb *function to print the output.
    While providing a custom writer function, use `syscall(2)` rather than `write(2)`.
    Pass *NULL* in order to use the default *write_cb* function which prints
    the output to the stderr. Use *cbopaque* parameter in order to pass some data to
    your *write_cb* function. Pass additional options using *opts*. For more details
    on opts see the [MEMORY STATISTICS PRINT OPTIONS](#memory-statistics-print-options)
    section below. Returns MEMKIND_ERROR_INVALID when failed to parse an options string,
    MEMKIND_SUCCESS on success.

#### DECORATORS ####

The memkind library enables the user to define decorator functions that can be
called before and after each memkind heap management API. The decorators that are
called at the beginning of the function end are named after that function with
*_pre* appended to the name and those that are called at the end of the function
are named after that function with *_post* appended to the name. These are weak
symbols and if they are not present at link time they are not called. The memkind
library does not define these symbols which are reserved for user definition. These
decorators can be used to track calls to the heap management interface or to modify
parameters. The decorators that are called at the beginning of the allocator pass
all inputs by reference and the decorators that are called at the end of the allocator
pass the output by reference. This enables the modification of the input and output
of each heap management function by the decorators.

#### ENHANCEMENTS ####

`int memkind_set_bg_threads(bool state)`
:   enables/disables internal background worker threads in jemalloc.

#### LIBRARY VERSION ####

The memkind library version scheme consist major, minor and patch numbers
separated by dot. Combining those numbers, we got the following representation:

major.minor.patch, where:

+ major number is incremented whenever the API is changed (loss of
  backward compatibility),
+ minor number is incremented whenever additional extensions are introduced
  or behavior has been changed,
+ patch number is incremented whenever small bug fixes are added.

memkind library provide numeric representation of the version by exposing
the following API:

`int memkind_get_version()`
:   returns version number represented by a single integer number, obtained from the formula:\
    major * 1000000 + minor * 1000 + patch

**Note:** major < 1 means an unstable API.

#### API standards ####

+ STANDARD API, the API is considered as stable
+ NON-STANDARD API, the API is considered as stable, however this is not a standard way
  to use memkind
+ EXPERIMENTAL API, the API is considered as unstable and the subject to change

# RETURN VALUE #

`memkind_calloc()`, `memkind_malloc()`, `memkind_realloc()` and `memkind_defrag_reallocate()`
returns the pointer to the allocated memory or *NULL* if the request fails.
`memkind_malloc_usable_size()` returns the number of usable bytes in the block of
allocated memory pointed to by ptr, a pointer to a block of memory allocated by
`memkind_malloc()` or a related function. If *ptr* is *NULL*, 0 is returned.
`memkind_free()` and `memkind_error_message()` do not have return values. All other
memkind API’s return 0 upon success and an error code defined in the [ERRORS](#errors)
section upon failure. The memkind library avoids setting *errno* directly, but calls
to underlying libraries and system calls may set *errno* (e.g. `memkind_create_pmem()`).

# KINDS #

**The available kinds of memory:**

MEMKIND_DEFAULT
:   Default allocation using standard memory and default page size.

MEMKIND_HIGHEST_CAPACITY
:   Allocate from a NUMA node(s) that has the highest capacity among all nodes
    in the system.

MEMKIND_HIGHEST_CAPACITY_PREFERRED
:   Same as **MEMKIND_HIGHEST_CAPACITY** except that if there is not enough memory
    in the NUMA node that has the highest capacity in the local domain to satisfy the
    request, the allocation will fall back on other memory NUMA nodes.
    **Note:** For this kind, the allocation will not succeed if there are two or
    more NUMA nodes that have the highest capacity.

MEMKIND_HIGHEST_CAPACITY_LOCAL
:   Allocate from a NUMA node that has the highest capacity among all NUMA Nodes from
    the local domain. NUMA Nodes have the same local domain for a set of CPUs associated
    with them, e.g. socket or sub-NUMA cluster.
    **Note:** If there are multiple NUMA nodes in the same local domain that have the
    highest capacity - allocation will be done from a NUMA node with a worse latency
    attribute. This kind requires locality information described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED
:   Same as **MEMKIND_HIGHEST_CAPACITY_LOCAL** except that if there is not enough
    memory in the NUMA node that has the highest capacity to satisfy the request, the
    allocation will fall back on other memory NUMA nodes.

MEMKIND_LOWEST_LATENCY_LOCAL
:   Allocate from a NUMA node that has the lowest latency among all NUMA Nodes from
    the local domain. NUMA Nodes have the same local domain for a set of CPUs
    associated with them, e.g. socket or sub-NUMA cluster. Note: If there are
    multiple NUMA nodes in the same local domain that have the lowest latency - allocation
    will be done from a NUMA node with smaller memory capacity. This kind requires
    locality and memory performance characteristics information described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED
:   Same as **MEMKIND_LOWEST_LATENCY_LOCAL** except that if there is not enough
    memory in the NUMA node that has the lowest latency to satisfy the request,
    the allocation will fall back on other memory NUMA nodes.

MEMKIND_HIGHEST_BANDWIDTH_LOCAL
:   Allocate from a NUMA node that has the highest bandwidth among all NUMA Nodes
    from the local domain. NUMA Nodes have the same local domain for a set of CPUs
    associated with them, e.g. socket or sub-NUMA cluster. Note: If there are multiple
    NUMA nodes in the same local domain that have the highest bandwidth - allocation
    will be done from a NUMA node with smaller memory capacity. This kind requires
    locality and memory performance characteristics information described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED
:   Same as **MEMKIND_HIGHEST_BANDWIDTH_LOCAL** except that if there is not
    enough memory in the NUMA node that has the highest bandwidth to satisfy
    the request, the allocation will fall back on other memory NUMA nodes.

MEMKIND_HUGETLB
:   Allocate from standard memory using huge pages.
    **Note:** This kind requires huge pages configuration described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

MEMKIND_INTERLEAVE
:   Allocate pages interleaved across all NUMA nodes with transparent
    huge pages disabled.

MEMKIND_HBW
:   Allocate from the closest high bandwidth memory NUMA node(s) at the time of
    allocation. If there is not enough high bandwidth memory to satisfy the request
    *errno* is set to **ENOMEM** and the allocated pointer is set to NULL.
    **Note:** This kind requires memory performance characteristics information
    described in the [SYSTEM CONFIGURATION](#system-configuration) section.

MEMKIND_HBW_ALL
:   Same as **MEMKIND_HBW except** decision regarding closest NUMA node(s) is
postponed until the time of the first write.

MEMKIND_HBW_HUGETLB
:   Same as **MEMKIND_HBW** except the allocation is backed by huge pages.
    **Note:** This kind requires huge pages configuration described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

MEMKIND_HBW_ALL_HUGETLB
:   Combination of **MEMKIND_HBW_ALL** and **MEMKIND_HBW_HUGETLB** properties.
    **Note:** This kind requires huge pages configuration described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

MEMKIND_HBW_PREFERRED
:   Same as **MEMKIND_HBW** except that if there is not enough high bandwidth
    memory to satisfy the request, the allocation will fall back on standard memory.
    **Note:** For this kind, the allocation will not succeed if two or more high
    bandwidth memory NUMA nodes are in the same shortest distance to the same CPU
    on which process is eligible to run. Check on that eligibility is done upon
    starting the application.

MEMKIND_HBW_PREFERRED_HUGETLB
:   Same as **MEMKIND_HBW_PREFERRED** except the allocation is backed by huge pages.
    **Note:** This kind requires huge pages configuration described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

MEMKIND_HBW_INTERLEAVE
:   Same as **MEMKIND_HBW** except that the pages that support the allocation are
    interleaved across all high bandwidth nodes and transparent huge pages are disabled.

MEMKIND_DAX_KMEM
:   Allocate from the closest persistent memory NUMA node at the time of allocation.
    If there is not enough memory in the closest persistent memory NUMA node to satisfy
    the request *errno* is set to **ENOMEM** and the allocated pointer is set to *NULL*.

MEMKIND_DAX_KMEM_ALL
:   Allocate from the closest persistent memory NUMA node available at the time of
    allocation. If there is not enough memory on any of persistent memory NUMA nodes to
    satisfy the request *errno* is set to **ENOMEM** and the allocated pointer is set to *NULL*.

MEMKIND_DAX_KMEM_PREFERRED
:   Same as **MEMKIND_DAX_KMEM** except that if there is not enough memory in the
    closest persistent memory NUMA node to satisfy the request, the allocation will
    fall back on other memory NUMA nodes.
    **Note:** For this kind, the allocation will not succeed if two or more persistent
    memory NUMA nodes are in the same shortest distance to the same CPU on which process
    is eligible to run. Check on that eligibility is done upon starting the application.

MEMKIND_DAX_KMEM_INTERLEAVE
:   Same as **MEMKIND_DAX_KMEM** except that the pages that support the allocation are
    interleaved across all persistent memory NUMA nodes.

MEMKIND_REGULAR
:   Allocate from regular memory using the default page size. Regular means general purpose
    memory from the NUMA nodes containing CPUs.

# MEMORY TYPES #

The available types of memory:

MEMKIND_MEMTYPE_DEFAULT
:   Standard memory, the same as the process uses.

MEMKIND_MEMTYPE_HIGH_BANDWIDTH
:   High bandwidth memory (HBM). There must be at least two memory types with
    different bandwidth to determine which is the HBM.

# MEMORY BINDING POLICY #

The available types of memory binding policy:

MEMKIND_POLICY_BIND_LOCAL
:   Allocate local memory. If there is not enough memory to satisfy the request *errno*
    is set to **ENOMEM** and the allocated pointer is set to NULL.

MEMKIND_POLICY_BIND_ALL
:   Memory locality is ignored. If there is not enough memory to satisfy the request
    *errno* is set to **ENOMEM** and the allocated pointer is set to NULL.

MEMKIND_POLICY_PREFERRED_LOCAL
:   Allocate preferred memory that is local. If there is not enough preferred memory
    to satisfy the request or preferred memory is not available, the allocation will
    fall back on any other memory.

MEMKIND_POLICY_INTERLEAVE_LOCAL
:   Interleave allocation across local memory. For n memory types the allocation will
    be interleaved across all of them.

MEMKIND_POLICY_INTERLEAVE_ALL
:   Interleave allocation. Locality is ignored. For n memory types the allocation
    will be interleaved across all of them.

# MEMORY FLAGS #

The available types of memory flags:

MEMKIND_MASK_PAGE_SIZE_2MB
:   Allocation backed by 2MB page size.

# MEMORY USAGE POLICY #

The available types of memory statistics:

MEMKIND_STAT_TYPE_RESIDENT
:   Maximum number of bytes in physically resident data pages mapped.

MEMKIND_STAT_TYPE_ACTIVE
:   Total number of bytes in active pages.

MEMKIND_STAT_TYPE_ALLOCATED
:   Total number of allocated bytes.

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

# ERRORS #

`memkind_posix_memalign()`
:   returns one of the POSIX standard error codes **EINVAL** or **ENOMEM** as
    defined in the <*errno.h*> if an error occurs (these have positive values).
    If the *alignment* parameter is not a power of two or is not a multiple of
    *sizeof(void*)*, then **EINVAL** is returned. If there is insufficient memory
    to satisfy the request then **ENOMEM** is returned.

All functions other than `memkind_posix_memalign()` which have an integer return
type return one of the negative error codes as defined in the <*memkind.h*> and
described below.

MEMKIND_ERROR_UNAVAILABLE
:   Requested memory kind is not available

MEMKIND_ERROR_MBIND
:   Call to `mbind(2)` failed

MEMKIND_ERROR_MMAP
:   Call to `mmap(2)` failed

MEMKIND_ERROR_MALLOC
:   Call to jemalloc’s `malloc()` failed

MEMKIND_ERROR_ENVIRON
:   Error parsing environment variable *MEMKIND_* *

MEMKIND_ERROR_INVALID
:   Invalid input arguments to memkind routine

MEMKIND_ERROR_TOOMANY
:   Error trying to initialize more than maximum **MEMKIND_MAX_KIND**
    number of kinds

MEMKIND_ERROR_BADOPS
:   Error memkind operation structure is missing or invalid

MEMKIND_ERROR_HUGETLB
:   Unable to allocate huge pages

MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE
:   Error requested memory type is not available

MEMKIND_ERROR_OPERATION_FAILED
:   Error memkind operation failed

MEMKIND_ERROR_ARENAS_CREATE
:   Call to jemalloc’s arenas.create() failed

MEMKIND_ERROR_RUNTIME
:   Unspecified run-time error

# UTILS #

*/usr/bin/memkind-hbw-nodes*
:   Prints a comma-separated list of high bandwidth nodes.

*/usr/bin/memkind-auto-dax-kmem-nodes*
:   Prints a comma-separated list of persistent memory NUMA nodes, which are
    automatically detected.

# ENVIRONMENT #

MEMKIND_HBW_NODES
:   This environment variable is a comma-separated list of NUMA nodes that are
    treated as high bandwidth. Uses the *libnuma* routine `numa_parse_nodestring()`
    for parsing, so the syntax described in the **numa**(3) man page for this
    routine applies: e.g. 1-3,5 is a valid setting.

MEMKIND_HBW_THRESHOLD
:   This environment variable is bandwidth in MB/s that is the threshold for
    identifying high bandwidth memory. The default threshold is 204800 (200 GB/s),
    which is used if this variable is not set. When set, it must be greater
    than or equal to 0.

MEMKIND_DAX_KMEM_NODES
:   This environment variable is a comma-separated list of NUMA nodes that are
    treated as PMEM memory. Uses the *libnuma* routine `numa_parse_nodestring()`
    for parsing, so the syntax described in the **numa**(3) man page for this routine
    applies: e.g. 1-3,5 is a valid setting.

MEMKIND_ARENA_NUM_PER_KIND
:   This environment variable allows leveraging internal mechanism of the library
    for setting number of arenas per kind. Value should be a positive integer
    (not greater than **INT_MAX** defined in the <*limits.h*>). The user should
    set the value based on the characteristics of the application that is using
    the library. Higher value can provide better performance in extremely
    multithreaded applications at the cost of memory overhead. See section
    **IMPLEMENTATION NOTES** of **jemalloc**(3) for more details about arenas.

MEMKIND_HOG_MEMORY
:   Controls behavior of memkind with regards to returning memory to the
    underlying OS. Setting **MEMKIND_HOG_MEMORY** to 1 causes memkind to not
    release memory to the OS in anticipation of memory reuse soon. This will
    improve latency of ’free’ operations but increase memory usage.
    **Note:** For file-backed kind memory will be released to the OS only
    after calling `memkind_destroy_kind()`, not after ’free’ operations.
    In context of **MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE** memory usage
    policy - it will also impact memory coalescing and results that block
    pages will be often reused (better memory usage at the cost of performance).

MEMKIND_DEBUG
:   Controls logging mechanism in memkind. Setting **MEMKIND_DEBUG** to 1
    enables printing messages like errors and general information about the
    environment to the stderr.

MEMKIND_BACKGROUND_THREAD_LIMIT
:   Enable background worker threads. The Value should be in the 0 to the
    maximum number of cpus range. Setting **MEMKIND_BACKGROUND_THREAD_LIMIT**
    to the specific value will limit the maximum number of background worker
    threads to this value. Value 0 means the maximum number of background worker
    threads will be limited to the maximum number of cpus.

MEMKIND_HEAP_MANAGER
:   Controls heap management behavior in the memkind library by switching to one
    of the available heap managers.

Values:

+ JEMALLOC - sets the jemalloc heap manager
+ TBB - sets the Intel Threading Building Blocks heap manager. This option requires
  installed Intel Threading Building Blocks library.

If the **MEMKIND_HEAP_MANAGER** is not set then the jemalloc heap manager will
be used by default.

# SYSTEM CONFIGURATION #

Interfaces for obtaining 2MB (HUGETLB) memory need allocated huge pages in the
kernel’s huge page pool.

HUGETLB (huge pages)
:   Current number of "persistent" huge pages can be read from the
    */proc/sys/vm/nr_hugepages* file. Proposed way of setting hugepages is:
    `sudo sysctl vm.nr_hugepages=<number_of_hugepages>`. More information
    can be found [here](https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt)

Interfaces for obtaining locality information are provided by *libhwloc* dependency.
Functionality based on locality requires that the memkind library is configured
and built with the support of the [*libhwloc*](https://www.open-mpi.org/projects/hwloc) :\
`./configure --enable-hwloc`

Interfaces for obtaining memory performance characteristics information are based
on the *HMAT* (Heterogeneous Memory Attribute Table)
https://uefi.org/sites/default/files/resources/ACPI_6_3_final_Jan30.pdf
Functionality based on memory performance characteristics requires that
the platform configuration fully supports *HMAT* and the memkind library
is configured and built with the support of the
[*libhwloc*](https://www.open-mpi.org/projects/hwloc) :\
`./configure --enable-hwloc`

**Note:** For a given target NUMA Node, the OS exposes only the performance
characteristics of the best performing NUMA node.

# STATIC LINKING #

When linking statically against memkind, *libmemkind.a* should be used together
with its dependencies *libnuma* and *pthread*. *Pthread* can be linked by adding
*/usr/lib64/libpthread.a* as a dependency (exact path may vary). Typically
*libnuma* will need to be compiled from sources to use it as a static dependency.
*libnuma* can be reached on [GitHub](https://github.com/numactl/numactl)

# KNOWN ISSUES #

HUGETLB (huge pages)
:   There might be some overhead in huge pages consumption caused by heap management.
    If your allocation fails because of OOM, please try to allocate extra huge pages
    (e.g. 8 huge pages).

# COPYRIGHT #

Copyright (C) 2014 - 2023 Intel Corporation. All rights reserved.

# SEE ALSO #

**malloc**(3), **malloc_usable_size**(3), **numa**(3), **hwloc**(3), **numactl**(8),
**mbind**(2), **mmap**(2), **jemalloc**(3), **memkind_dax_kmem**(3), **memkind_default**(3),
**memkind_arena**(3), **memkind_fixed**(3), **memkind_hbw**(3), **memkind_hugetlb**(3),
**memkind_pmem**(3), **syscall**(2), **write**(2)
