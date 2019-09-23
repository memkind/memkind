/*
 * Copyright (C) 2019 Intel Corporation.
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

#include <map>
#include <memory>
#include <string>
#include <exception>
#include <type_traits>
#include <cstddef>

#include "memkind.h"

/*
 * Header file for the C++ allocator compatible with the C++ standard library allocator concepts.
 * More details in memkind(3) man page.
 * Note: memory heap management is based on memkind_malloc, refer to the memkind(3) man page for more
 * information.
 *
 * Functionality defined in this header is considered as stable API (STANDARD API).
 * API standards are described in memkind(3) man page.
 */
namespace libmemkind
{
    enum class kinds {
        DEFAULT = 0,
        HUGETLB = 1,
        INTERLEAVE = 2,
        HBW = 3,
        HBW_ALL = 4,
        HBW_HUGETLB = 5,
        HBW_ALL_HUGETLB = 6,
        HBW_PREFERRED = 7,
        HBW_PREFERRED_HUGETLB = 8,
        HBW_INTERLEAVE = 9,
        REGULAR = 10,
        DAX_KMEM = 11,
    };

    namespace static_kind
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
            friend class allocator;

#if !_GLIBCXX_USE_CXX11_ABI
            /* This is a workaround for compilers (e.g GCC 4.8) that uses C++11 standard,
             * but use old - non C++11 ABI */
            template<typename V = void>
            explicit allocator()
            {
                static_assert(std::is_same<V, void>::value,
                              "libmemkind::static_kind::allocator cannot be compiled without CXX11 ABI");
            }
#endif

            explicit allocator(const libmemkind::kinds kind)
            {
                _kind = _kind_type_map[kind];
            }

            allocator(const allocator &other) = default;

            template <typename U>
            allocator(const allocator<U> &other) noexcept
            {
                _kind = other._kind;
            }

            allocator(allocator &&other) = default;

            template <typename U>
            allocator(const allocator<U> &&other) noexcept
            {
                _kind = std::move(other._kind);
            }

            allocator<T> &operator = (const allocator &other) = default;

            template <typename U>
            allocator<T> &operator = (const allocator<U> &other) noexcept
            {
                _kind = other._kind;
                return *this;
            }

            allocator<T> &operator = (allocator &&other) = default;

            template <typename U>
            allocator<T> &operator = (allocator<U> &&other) noexcept
            {
                _kind = std::move(other._kind);
                return *this;
            }

            pointer allocate(size_type n) const
            {
                pointer result = static_cast<pointer>(memkind_malloc(_kind, n*sizeof(T)));
                if (!result) {
                    throw std::bad_alloc();
                }
                return result;
            }

            void deallocate(pointer p, size_type n) const
            {
                memkind_free(_kind, static_cast<void *>(p));
            }

            template <class U, class... Args>
            void construct(U *p, Args &&... args) const
            {
                ::new((void *)p) U(std::forward<Args>(args)...);
            }

            void destroy(pointer p) const
            {
                p->~value_type();
            }

            template <typename U, typename V>
            friend bool operator ==(const allocator<U> &lhs, const allocator<V> &rhs);

            template <typename U, typename V>
            friend bool operator !=(const allocator<U> &lhs, const allocator<V> &rhs);

        private:
            memkind_t _kind;
            std::map<libmemkind::kinds, memkind_t> _kind_type_map = {
                { libmemkind::kinds::DEFAULT, MEMKIND_DEFAULT},
                { libmemkind::kinds::HUGETLB, MEMKIND_HUGETLB},
                { libmemkind::kinds::INTERLEAVE, MEMKIND_INTERLEAVE},
                { libmemkind::kinds::HBW, MEMKIND_HBW},
                { libmemkind::kinds::HBW_ALL, MEMKIND_HBW_ALL},
                { libmemkind::kinds::HBW_HUGETLB, MEMKIND_HBW_HUGETLB},
                { libmemkind::kinds::HBW_ALL_HUGETLB, MEMKIND_HBW_ALL_HUGETLB},
                { libmemkind::kinds::HBW_PREFERRED, MEMKIND_HBW_PREFERRED},
                { libmemkind::kinds::HBW_PREFERRED_HUGETLB, MEMKIND_HBW_PREFERRED_HUGETLB},
                { libmemkind::kinds::HBW_INTERLEAVE, MEMKIND_HBW_INTERLEAVE},
                { libmemkind::kinds::REGULAR, MEMKIND_REGULAR},
                { libmemkind::kinds::DAX_KMEM, MEMKIND_DAX_KMEM}
            };
        };

        template <typename U, typename V>
        bool operator ==(const allocator<U> &lhs, const allocator<V> &rhs)
        {
            return lhs._kind == rhs._kind;
        }

        template <typename U, typename V>
        bool operator !=(const allocator<U> &lhs, const allocator<V> &rhs)
        {
            return !(lhs._kind == rhs._kind);
        }
    }  // namespace static_kind
}  // namespace libmemkind
