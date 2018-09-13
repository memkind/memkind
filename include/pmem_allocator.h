/*
 * Copyright (C) 2018 Intel Corporation.
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

#pragma once

#include <memory>
#include <string>
#include <exception>
#include <type_traits>
#include <atomic>

#include "memkind.h"

/*
 * Header file for the C++ allocator compatible with the C++ standard library allocator concepts.
 * More details in pmemallocator(3) man page.
 * Note: memory heap management is based on memkind_malloc, refer to the memkind(3) man page for more
 * information.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */
namespace pmem
{
    template<typename T>
    class allocator
    {
    public:
        using value_type = T;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        template<class U>
        struct rebind {
            using other = allocator<U>;
        };

        template<typename U>
        friend struct allocator;

    private:
        memkind_t kind_ptr;
        std::atomic<size_t> * ref_count;

        void clean_up()
        {
            if (ref_count->fetch_sub(1) == 1) {
                memkind_destroy_kind(kind_ptr);
                delete ref_count;
            }
        }

        template <typename U>
        inline void assign(U&& other)
        {
            kind_ptr = other.kind_ptr;
            ref_count = other.ref_count;
            ++(*ref_count);
        }

    public:
#ifndef _GLIBCXX_USE_CXX11_ABI
        /* This is a workaround for compilers (e.g GCC 4.8) that uses C++11 standard,
         * but use old - non C++11 ABI */
        template<typename V = void>
        explicit allocator()
        {
            static_assert(std::is_same<V, void>::value,
                          "pmem::allocator cannot be compiled without CXX11 ABI");
        }
#endif

        explicit allocator(const char *dir, size_t max_size)
        {
            int err_c  = memkind_create_pmem(dir, max_size, &kind_ptr);
            if (err_c) {
                throw std::runtime_error(
                    std::string("An error occured while creating pmem kind; error code: ") +
                    std::to_string(err_c));
            }
            ref_count = new std::atomic<size_t>(1);
        }

        explicit allocator(const std::string& dir,
                           size_t max_size) : allocator(dir.c_str(), max_size)
        {
        }

        allocator(const allocator& other) noexcept
        {
            assign(other);
        }

        template <typename U>
        allocator(const allocator<U>& other) noexcept
        {
            assign(other);
        }

        allocator(const allocator&& other) noexcept
        {
            assign(other);
        }

        template <typename U>
        allocator(const allocator<U>&& other) noexcept
        {
            assign(other);
        }

        void operator = (const allocator& other) noexcept
        {
            clean_up();
            assign(other);
        }

        template <typename U>
        void operator = (const allocator<U>& other) noexcept
        {
            clean_up();
            assign(other);
        }

        void operator = (const allocator&& other) noexcept
        {
            clean_up();
            assign(other);
        }

        template <typename U>
        void operator = (const allocator<U>&& other) noexcept
        {
            clean_up();
            assign(other);
        }

        template <typename U>
        bool operator ==(const allocator<U>& other) const
        {
            return kind_ptr == other.kind_ptr;
        }

        template <typename U>
        bool operator !=(const allocator<U>& other) const
        {
            return !(*this ==other);
        }

        T* allocate(std::size_t n) const
        {
            T* result = static_cast<T*>(memkind_malloc(kind_ptr, n*sizeof(T)));
            if (!result) {
                throw std::bad_alloc();
            }
            return result;
        }

        void deallocate(T* p, std::size_t n) const
        {
            memkind_free(kind_ptr, static_cast<void*>(p));
        }

        template <class U, class... Args>
        void construct(U* p, Args... args) const
        {
            ::new((void *)p) U(std::forward<Args>(args)...);
        }

        void destroy(T* p) const
        {
            p->~value_type();
        }

        ~allocator()
        {
            clean_up();
        }
    };
}
