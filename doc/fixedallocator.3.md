---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["fixedallocator.3.html"]
title: FIXEDALLOCATOR
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2021-2022, Intel Corporation)

[comment]: <> (fixedallocator.3 -- man page for fixedallocator)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**libmemkind::fixed::allocator<T>** - The C++ allocator
compatible with the C++ standard library allocator concepts.

**Note:** *fixed_allocator.h* functionality is considered as a
stable API (STANDARD API).

# SYNOPSIS #

```c
#include <fixed_allocator.h>

Link with -lmemkind

libmemkind::fixed::allocator(void *addr, size_t size);
template <typename U> libmemkind::fixed::allocator<T>::allocator(const libmemkind::fixed::allocator<U>&) noexcept;
template <typename U> libmemkind::fixed::allocator<T>::allocator(allocator<U>&& other) noexcept;
libmemkind::fixed::allocator<T>::~allocator();
T *libmemkind::fixed::allocator<T>::allocate(std::size_t n) const;
void libmemkind::fixed::allocator<T>::deallocate(T *p, std::size_t n) const;
template <class U, class... Args> void libmemkind::fixed::allocator<T>::construct(U *p, Args... &&args) const;
void libmemkind::fixed::allocator<T>::destroy(T *p) const;
```

# DESCRIPTION #

`libmemkind::fixed::allocator<T>`
:   is intended to be used with STL containers to allocate
    memory on specific area. Memory management is handled by
    jemalloc on the supplied area. Refer
    [**memkind**(3)](/memkind/manpages/memkind.3.html)
    man page for more details.\
    All public member types and functions correspond to standard library allocator concepts and definitions. The current implementation supports the C++11 standard.

+ Template arguments:

  + *T* is an object type aliased by value_type.
  + *U* is an object type.

**Note:**

+ **`T *libmemkind::fixed::allocator<T>::allocate(std::size_t n)`**\
  allocates memory using **memkind_malloc**() on the area
  supplied to `libmemkind::fixed::allocator()`. Throw
  *std::bad_alloc* when there is not enough memory to
  satisfy the request.
  In systems with the standard library behavior of malloc(0) returning NULL,
  *std::bad_alloc* is also thrown when size *n* is zero.

+ **`libmemkind::fixed::allocator<T>::deallocate(T *p, std::size_t n)`**\
  deallocates memory associated with a pointer returned by
  `allocate()` using `memkind_free()`.

# COPYRIGHT #

Copyright (C) 2021 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**memkind**(3)
