---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["pmemallocator.3.html"]
title: PMEMALLOCATOR
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2018-2022, Intel Corporation)

[comment]: <> (pmemallocator.3 -- man page for pmemallocator)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**libmemkind::pmem::allocator<T>** - The C++ allocator compatible with the
C++ standard library allocator concepts.

**Note:** *pmem_allocator.h* functionality is considered as a stable
API (STANDARD API).

# SYNOPSIS #

```c
#include <pmem_allocator.h>

Link with -lmemkind

libmemkind::pmem::allocator(const char *dir, size_t max_size);
libmemkind::pmem::allocator(const char &dir, size_t max_size, libmemkind::allocation_policy alloc_policy);
libmemkind::pmem::allocator(const std::string &dir, size_t max_size);
libmemkind::pmem::allocator(const std::string &dir, size_t max_size, libmemkind::allocation_policy alloc_policy);
template <typename U> libmemkind::pmem::allocator<T>::allocator(const libmemkind::pmem::allocator<U>&) noexcept;
template <typename U> libmemkind::pmem::allocator(const allocator<U>&& other) noexcept;
libmemkind::pmem::allocator<T>::~allocator();
T *libmemkind::pmem::allocator<T>::allocate(std::size_t n) const;
void libmemkind::pmem::allocator<T>::deallocate(T *p, std::size_t n) const;
template <class U, class... Args> void libmemkind::pmem::allocator<T>::construct(U *p, Args... args) const;
void libmemkind::pmem::allocator<T>::destroy(T *p) const;
```

# DESCRIPTION #

`libmemkind::pmem::allocator<T>`
:   is intended to be used with STL containers to allocate persistent memory.
  Memory management is based on memkind_pmem (memkind library). Refer to
  the **memkind_pmem**(3) and the [**memkind**(3)](/memkind/manpages/memkind.3.html)
  man page for more details.

`libmemkind::allocation_policy`
:   specifies allocator memory usage policy, which allows to tune up memory utilization.
  The available types of allocator usage policy:

+ **`libmemkind::allocation_policy::DEFAULT`**\
  Default allocator memory usage policy.

+ **`libmemkind::allocation_policy::CONSERVATIVE`**\
  Conservative allocator memory usage policy - prioritize memory usage at the
  cost of performance.\
  All public member types and functions correspond to standard library allocator
  concepts and definitions. The current implementation supports the C++11 standard.

  + Template arguments:

    + *T* is an object type aliased by value_type.
    + *U* is an object type.

**Note:**

+ **`T *libmemkind::pmem::allocator<T>::allocate(std::size_t n)`**\
  allocates persistent memory using `memkind_malloc()`. Throw *std::bad_alloc*
  when *n = 0* or there is not enough memory to satisfy the request.

+ **`libmemkind::pmem::allocator<T>::deallocate(T *p, std::size_t n)`**\
  deallocates memory associated with pointer returned by `allocate()` using
  `memkind_free()`.

# COPYRIGHT #

Copyright (C) 2018 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**memkind_pmem**(3), **memkind**(3)
