// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#pragma once

#include <hbwmalloc.h>

#include <new>
#include <stddef.h>
/*
 * Header file for the C++ allocator compatible with the C++ standard library
 * allocator concepts. More details in hbwallocator(3) man page. Note: memory
 * heap management is based on hbwmalloc, refer to the hbwmalloc man page for
 * more information.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */
namespace hbw
{

template <class T>
class allocator
{
public:
    /*
     *  Public member types required and defined by the standard library
     * allocator concepts.
     */
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T value_type;

    template <class U>
    struct rebind {
        typedef hbw::allocator<U> other;
    };

    /*
     *  Public member functions required and defined by the standard library
     * allocator concepts.
     */
    allocator() throw()
    {}

    template <class U>
    allocator(const allocator<U> &) throw()
    {}

    ~allocator() throw()
    {}

    pointer address(reference x) const
    {
        return &x;
    }

    const_pointer address(const_reference x) const
    {
        return &x;
    }

    /*
     *  Allocates n*sizeof(T) bytes of high bandwidth memory using hbw_malloc().
     *  Throws std::bad_alloc when cannot allocate memory.
     */
    pointer allocate(size_type n, const void * = 0)
    {
        if (n > this->max_size()) {
            throw std::bad_alloc();
        }
        pointer result = static_cast<pointer>(hbw_malloc(n * sizeof(T)));
        if (!result) {
            throw std::bad_alloc();
        }
        return result;
    }

    /*
     *  Deallocates memory associated with pointer returned by allocate() using
     * hbw_free().
     */
    void deallocate(pointer p, size_type n)
    {
        hbw_free(static_cast<void *>(p));
    }

    size_type max_size() const throw()
    {
        return size_t(-1) / sizeof(T);
    }

    void construct(pointer p, const_reference val)
    {
        ::new (p) value_type(val);
    }

    void destroy(pointer p)
    {
        p->~T();
    }
};

template <class T, class U>
bool operator==(const allocator<T> &, const allocator<U> &)
{
    return true;
}

template <class T, class U>
bool operator!=(const allocator<T> &, const allocator<U> &)
{
    return false;
}

} // namespace hbw
