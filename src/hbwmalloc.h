/*
 * Copyright (C) 2014, 2015 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef hbw_malloc_include_h
#define hbw_malloc_include_h
#ifdef __cplusplus
extern "C" {
#endif
/*!
 *  \file hbwmalloc.h
 *  \brief Header file for the high bandwidth memory interface.
 *
 *  #include <hbwmalloc.h>
 *
 *  Link with -lnuma -lmemkind
 *
 *  This file defines the external API's and enumerations for the
 *  hbwmalloc library.  These interfaces define a heap manager that
 *  targets the high bandwidth memory numa nodes.
 *
 *  \section ENVIRONMENT
 *  \subsection MEMKIND_HBW_NODES
 *  Comma separated list of NUMA nodes that are treated as high
 *  bandwidth.  Can be used if pmtt file is not present.  Uses same
 *  parser as numactl, so logic defined in numactl man pages applies:
 *  e.g 1-3,5 is a valid setting.
 */

/*!
 *  \brief Fallback policy.
 *
 *  Policy that determines behavior when there is not enough free high
 *  bandwidth memory to satisfy a user request.  This enum is used with
 *  hbw_get_policy() and hbw_set_policy().
 */
typedef enum {
    /*!
     *  If insufficient high bandwidth memory pages are available seg
     *  fault when memory is touched (default).
     */
    HBW_POLICY_BIND = 1,
    /*!
     *  If insufficient high bandwidth memory pages are available fall
     *  back on standard memory pages.
     */
    HBW_POLICY_PREFERRED = 2
} hbw_policy_t;

/*!
 *  \brief Page size selection.
 *
 *  The hbw_posix_memalign_psize() API gives the user the option to
 *  select the page size from this enumerated list.
 */
typedef enum {
    /*!
     *  The 4 kilobyte page size option, this enables the same behavior
     *  from hbw_posix_memalign_psize() as the hbw_posix_memalign()
     *  API.  Note with transparent huge pages enabled, these allocations
     *  may be promoted by the operating system to 2 megabyte pages.
     */
    HBW_PAGESIZE_4KB           = 1,
    HBW_PAGESIZE_2MB           = 2,
    HBW_PAGESIZE_1GB_STRICT    = 3,
    HBW_PAGESIZE_1GB           = 4
} hbw_pagesize_t;

int hbw_get_policy(void);
void hbw_set_policy(int mode);
int hbw_check_available(void);
void *hbw_malloc(size_t size);
void *hbw_calloc(size_t num, size_t size);
int hbw_posix_memalign(void **memptr, size_t alignment, size_t size);
int hbw_posix_memalign_psize(void **memptr, size_t alignment, size_t size,
                             int pagesize);
void *hbw_realloc(void *ptr, size_t size);
void hbw_free(void *ptr);

#ifdef __cplusplus
}

#include <stddef.h>
#include <new>

namespace hbwmalloc
{
template <class T>
class hbwmalloc_allocator
{
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    template <class U>
    struct rebind {
        typedef hbwmalloc_allocator<U> other;
    };

    hbwmalloc_allocator() throw() { }

    hbwmalloc_allocator(const hbwmalloc_allocator&) throw() { }

    template <class U>
    hbwmalloc_allocator(const hbwmalloc_allocator<U>&) throw() { }

    ~hbwmalloc_allocator() throw() { }

    pointer
    address(reference x) const
    {
        return &x;
    }

    const_pointer
    address(const_reference x) const
    {
        return &x;
    }

    pointer allocate(size_type n, const void * = 0)
    {
        if (n > this->max_size()) {
            throw std::bad_alloc();
        }
        pointer result = static_cast<T*>(hbw_malloc(n * sizeof(T)));
        if (!result) {
            throw std::bad_alloc();
        }
        return result;
    }

    void deallocate(pointer p, size_type n)
    {
        hbw_free(static_cast<void*>(p));
    }

    size_type max_size() const throw()
    {
        return size_t(-1) / sizeof(T);
    }

    void construct(pointer p, const T& val)
    {
        ::new(p) value_type(val);
    }

    void destroy(pointer p)
    {
        p->~T();
    }
};

}

#endif
#endif
