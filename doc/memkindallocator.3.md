---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkindallocator.3.html"]
title: MEMKINDALLOCATOR
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2019-2022, Intel Corporation)

[comment]: <> (memkindallocator.3 -- man page for memkindallocator)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**SYSTEM CONFIGURATION**](#system-configuration)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**libmemkind::static_kind::allocator<T>** - The C++ allocator compatible with the
C++ standard library allocator concepts.

**Note:** *memkind_allocator.h* functionality is considered as a stable
API (STANDARD API).

# SYNOPSIS #

```c
#include <memkind_allocator.h>

Link with -lmemkind

libmemkind::static_kind::allocator(libmemkind::kinds kind);
template <typename U> libmemkind::static_kind::allocator<T>::allocator(const libmemkind::static_kind::allocator<U>&) noexcept;
template <typename U> libmemkind::static_kind::allocator(const allocator<U>&& other) noexcept;
libmemkind::static_kind::allocator<T>::~allocator();
T *libmemkind::static_kind::allocator<T>::allocate(std::size_t n) const;
void libmemkind::static_kind::allocator<T>::deallocate(T *p, std::size_t n) const;
template <class U, class... Args> void libmemkind::static_kind::allocator<T>::construct(U *p, Args... args) const;
void libmemkind::static_kind::allocator<T>::destroy(T *p) const;
```

# DESCRIPTION #

`libmemkind::static_kind::allocator<T>`
:   is intended to be used with STL containers to allocate from static kinds
    of memory. All public member types and functions correspond to standard
    library allocator concepts and definitions. The current implementation supports
    the C++11 standard.

+ Template arguments:
  + T is an object type aliased by value_type,
  + U is an object type. Memory management is based on the memkind library.
    Refer to the [**memkind**](/memkind/manpages/memkind.3/)(3) man page for more details.

`T *libmemkind::static_kind::allocator<T>::allocate(std::size_t n)`
:   allocates uninitialized memory of size *n* bytes of the specified kind using
    `memkind_malloc()`. Throw **std::bad_alloc** when n = 0 or there is not enough
    memory to satisfy the request.

`libmemkind::static_kind::allocator<T>::deallocate(T *p, std::size_t n)`
:   deallocates memory associated with pointer returned by `allocate()` using `memkind_free()`.

`libmemkind::kinds`
:   specifies allocator static kinds of memory, representing type of memory which offers
    different characteristics. The available types of allocator kinds of memory:

#### Types of allocator kinds of memory ####

`libmemkind::kinds::DEFAULT`
:   The default allocation using standard memory and the default page size.
    The allocation can be made using any NUMA node containing memory.

`libmemkind::kinds::HIGHEST_CAPACITY`
:   Allocate from a NUMA node(s) that has the highest capacity among all nodes in the system.

`libmemkind::kinds::HIGHEST_CAPACITY_PREFERRED`
:   Same as `libmemkind::kinds::HIGHEST_CAPACITY` except that if there is not enough
    memory in the NUMA node that has the highest capacity in the local domain to satisfy
    the request, the allocation will fall back on other memory NUMA nodes.
    **Note:** For this kind, the allocation will not succeed if there are two or more
    NUMA nodes that have the highest capacity.

`libmemkind::kinds::HIGHEST_CAPACITY_LOCAL
:   Allocate from a NUMA node that has the highest capacity among all NUMA Nodes from
    the local domain. NUMA Nodes have the same local domain for a set of CPUs associated
    with them, e.g. socket or sub-NUMA cluster.
    **Note:** If there are multiple NUMA nodes in the same local domain that have the
    highest capacity - the allocation will be done from a NUMA node with worse latency
    attribute. This kind requires locality information described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

`libmemkind::kinds::HIGHEST_CAPACITY_LOCAL_PREFERRED`
:   Same as `libmemkind::kinds::HIGHEST_CAPACITY_LOCAL` except that if there is not enough
    memory in the NUMA node that has the highest capacity to satisfy the request, the
    allocation will fall back on other memory NUMA nodes.

`libmemkind::kinds::LOWEST_LATENCY_LOCAL`
:   Allocate from a NUMA node that has the lowest latency among all NUMA Nodes
    from the local domain. NUMA Nodes have the same local domain for a set of CPUs
    associated with them, e.g. socket or sub-NUMA cluster.
    **Note:** If there are multiple NUMA nodes in the same local domain that
    have the lowest latency - the allocation will be done from a NUMA node with
    smaller memory capacity. This kind requires locality and memory performance
    characteristics information described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

`libmemkind::kinds::LOWEST_LATENCY_LOCAL_PREFERRED`
:   Same as `libmemkind::kinds::LOWEST_LATENCY_LOCAL` except that if there is not
    enough memory in the NUMA node that has the lowest latency to satisfy the request,
    the allocation will fall back on other memory NUMA nodes.

`libmemkind::kinds::HIGHEST_BANDWIDTH_LOCAL`
:   Allocate from a NUMA node that has the highest bandwidth among all NUMA Nodes
    from the local domain. NUMA Nodes have the same local domain for a set of CPUs
    associated with them, e.g. socket or sub-NUMA cluster.
    **Note:** If there are multiple NUMA nodes in the same local domain that have
    the highest bandwidth - the allocation will be done from a NUMA node with
    smaller memory capacity. This kind requires locality and memory performance
    characteristics information described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

`libmemkind::kinds::HIGHEST_BANDWIDTH_LOCAL_PREFERRED`
:   Same as `libmemkind::kinds::HIGHEST_BANDWIDTH_LOCAL` except that if there
    is not enough memory in the NUMA node that has the highest bandwidth to satisfy
    the request, the allocation will fall back on other memory NUMA nodes.

`libmemkind::kinds::HUGETLB`
:   Allocate from standard memory using huge pages.
    **Note:** This kind requires huge pages configuration described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

`libmemkind::kinds::INTERLEAVE`
:   Allocate pages interleaved across all NUMA nodes with transparent
    huge pages disabled.

`libmemkind::kinds::HBW`
:   Allocate from the closest high bandwidth memory NUMA node at the time of
    allocation. If there is not enough high bandwidth memory to satisfy the request,
    *errno* is set to **ENOMEM** and the allocated pointer is set to NULL.
    **Note:** This kind requires memory performance characteristics information
    described in the [SYSTEM CONFIGURATION](#system-configuration) section.

`libmemkind::kinds::HBW_ALL`
:   Same as `libmemkind::kinds::HBW` except decision regarding closest NUMA node
    is postponed until the time of the first write.

`libmemkind::kinds::HBW_HUGETLB`
:   Same as `libmemkind::kinds::HBW` except the allocation is backed by huge pages.
    Note: This kind requires huge pages configuration described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

`libmemkind::kinds::HBW_ALL_HUGETLB`
:   Combination of `libmemkind::kinds::HBW_ALL` and `libmemkind::kinds::HBW_HUGETLB`
    properties.
    **Note:** This kind requires huge pages configuration described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

`libmemkind::kinds::HBW_PREFERRED`
:   Same as `libmemkind::kinds::HBW` except that if there is not enough high
    bandwidth memory to satisfy the request, the allocation will fall back on
    standard memory.

`libmemkind::kinds::HBW_PREFERRED_HUGETLB`
:   Same as `libmemkind::kinds::HBW_PREFERRED` except the allocation is backed by
    huge pages.
    **Note:** This kind requires huge pages configuration described in the
    [SYSTEM CONFIGURATION](#system-configuration) section.

`libmemkind::kinds::HBW_INTERLEAVE`
:   Same as `libmemkind::kinds::HBW` except that the pages that support the
    allocation are interleaved across all high bandwidth nodes and transparent huge
    pages are disabled.

`libmemkind::kinds::REGULAR`
:   Allocate from regular memory using the default page size. Regular means
    general purpose memory from the NUMA nodes containing CPUs.

`libmemkind::kinds::DAX_KMEM`
:   Allocate from the closest persistent memory NUMA node at the time of allocation.
    If there is not enough memory in the closest persistent memory NUMA node to
    satisfy the request, *errno* is set to **ENOMEM** and the allocated pointer is set
    to NULL.

`libmemkind::kinds::DAX_KMEM_ALL`
:   Allocate from the closest persistent memory NUMA node available at the time of
    allocation. If there is not enough memory on any of persistent memory NUMA nodes
    to satisfy the request, *errno* is set to **ENOMEM** and the allocated pointer is set
    to NULL.

`libmemkind::kinds::DAX_KMEM_PREFERRED`
:   Same as `libmemkind::kinds::DAX_KMEM` except that if there is not enough memory
    in the closest persistent memory NUMA node to satisfy the request, the allocation
    will fall back on other memory NUMA nodes.
    **Note:** For this kind, the allocation will not succeed if two or more persistent
    memory NUMA nodes are in the same shortest distance to the same CPU on which process
    is eligible to run. Check on that eligibility is done upon starting the application.

`libmemkind::kinds::DAX_KMEM_INTERLEAVE`
:   Same as `libmemkind::kinds::DAX_KMEM` except that the pages that support the
    allocation are interleaved across all persistent memory NUMA nodes.

# SYSTEM CONFIGURATION #

HUGETLB (huge pages)
:   Interfaces for obtaining 2MB (**HUGETLB**) memory need allocated huge pages in the
    kernel’s huge page pool.
    Current number of "persistent" huge pages can be read from the */proc/sys/vm/nr_hugepages* file.
    Proposed way of setting hugepages is: `sudo sysctl vm.nr_hugepages=<number_of_hugepages>`.
    More information can be found here: <https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt>

Locality information
:   Interfaces for obtaining locality information are provided by *libhwloc* dependency.
    Functionality based on locality requires that memkind library is configured
    and built with the support of the [*libhwloc*](https://www.open-mpi.org/projects/hwloc) :\
    `./configure --enable-hwloc`

Memory performance characteristics information
:   Interfaces for obtaining memory performance characteristics information are based on *HMAT*
    (Heterogeneous Memory Attribute Table). See
    <https://uefi.org/sites/default/files/resources/ACPI_6_3_final_Jan30.pdf> for more information.
    Functionality based on memory performance characteristics requires that platform configuration
    fully supports *HMAT* and memkind library is configured and built with the support of the
    [*libhwloc*](https://www.open-mpi.org/projects/hwloc) :\
    `./configure --enable-hwloc`

**Note:** For a given target NUMA Node, the OS exposes only the performance
characteristics of the best performing NUMA node.

*libhwloc* can be reached on: <https://www.open-mpi.org/projects/hwloc>

# COPYRIGHT #

Copyright (C) 2019 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

[**memkind**](/memkind/manpages/memkind.3/)(3)
