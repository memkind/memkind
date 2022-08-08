---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["hbwallocator.3.html"]
title: HBWALLOCATOR
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2015-2022, Intel Corporation)

[comment]: <> (hbwallocator.3 -- man page for fixedallocator)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**ERRORS**](#errors)\
[**NOTES**](#notes)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**hbw::allocator<T>** - The C++ allocator compatible with the C++ standard library allocator concepts\
**Note:** This is EXPERIMENTAL API. The functionality and the header file itself can be changed (including non-backward compatible changes) or removed.

# SYNOPSIS #

```c
#include <hbw_allocator.h>

Link with -lmemkind

hbw::allocator();
template <class U>hbw::allocator<T>::allocator(const hbw::allocator<U>&);
hbw::allocator<T>::~allocator()`
hbw::allocator<T>::pointer hbw::allocator<T>::address(hbw::allocator<T>::reference x);
hbw::allocator<T>::const_pointer hbw::allocator<T>::address(hbw::allocator<T>::const_reference x);
hbw::allocator<T>::pointer hbw::allocator<T>::allocate(hbw::allocator<T>::size_type n, const void * = 0);
void hbw::allocator<T>::deallocate(hbw::allocator<T>::pointer p, hbw::allocator<T>::size_type n);
hbw::allocator<T>::size_type  hbw::allocator<T>::max_size();
void hbw::allocator<T>::construct(hbw::allocator<T>::pointer p, const hbw::allocator<T>::value_type& val);
void hbw::allocator<T>::destroy(hbw::allocator<T>::pointer p);
```

# DESCRIPTION #

The  **hbw::allocator<T>**  is  intended  to  be  used with STL containers to allocate high bandwidth memory. Memory management is based on hbwmalloc (**memkind** library), enabling users to gain performance in multithreaded applications. Refer **hbwmalloc**(3) and **memkind**(3) man page for more details.

All public member types and functions corresponds to standard library allocator concepts and definitions. The current implementation supports the C++03 standard.

Template arguments:
+ T is an object type aliased by value_type.
+ U is an object type.

**Note:**

`hbw::allocator<T>::pointer hbw::allocator<T>::allocate(hbw::allocator<T>::size_type n, const void * = 0)`
:   allocates high bandwidth memory using `hbw_malloc()`. Throw `std::bad_alloc` when:

+ n = 0,
+ n > max_size()
+ or there is not enough memory to satisfy the request.

`hbw::allocator<T>::deallocate(hbw::allocator<T>::pointer p, hbw::allocator<T>::size_type n)`
:   deallocates memory associated with pointer returned by allocate() using hbw_free().

## ERRORS ##

The same as described in **ERRORS** section of **hbwmalloc**(3) man page.

## NOTES ##

The `hbw::allocator<T>` behavior depends on hbwmalloc heap management policy. To get and set the policy please use `hbw_get_policy()` and `hbw_set_policy()` respectively.

## COPYRIGHT ##

Copyright (C) 2015 - 2022 Intel Corporation. All rights reserved.

## SEE ALSO ##

**hbwmalloc**(3), **numa**(3), **numactl**(8), **memkind**(3)
