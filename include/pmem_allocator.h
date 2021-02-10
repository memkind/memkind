// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2018 - 2020 Intel Corporation. */

#pragma once

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "memkind.h"

/*
 * Header file for the C++ allocator compatible with the C++ standard library
 * allocator concepts. More details in pmemallocator(3) man page. Note: memory
 * heap management is based on memkind_malloc, refer to the memkind(3) man page
 * for more information.
 *
 * Functionality defined in this header is considered as stable API (STANDARD
 * API). API standards are described in memkind(3) man page.
 */
namespace libmemkind
{
enum class allocation_policy
{
    DEFAULT = MEMKIND_MEM_USAGE_POLICY_DEFAULT,
    CONSERVATIVE = MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE,
    MAX
};

namespace pmem
{
namespace internal
{
class kind_wrapper_t
{
public:
    kind_wrapper_t(const char *dir, std::size_t max_size,
                   libmemkind::allocation_policy alloc_policy =
                       libmemkind::allocation_policy::DEFAULT)
    {
        cfg = memkind_config_new();
        if (!cfg) {
            throw std::runtime_error(
                std::string("An error occurred while creating pmem config"));
        }
        memkind_config_set_path(cfg, dir);
        memkind_config_set_size(cfg, max_size);
        memkind_config_set_memory_usage_policy(
            cfg, static_cast<memkind_mem_usage_policy>(alloc_policy));
        int err_c = memkind_create_pmem_with_config(cfg, &kind);
        memkind_config_delete(cfg);
        if (err_c) {
            throw std::runtime_error(
                std::string(
                    "An error occurred while creating pmem kind; error code: ") +
                std::to_string(err_c));
        }
    }

    kind_wrapper_t(const kind_wrapper_t &) = delete;
    void operator=(const kind_wrapper_t &) = delete;

    ~kind_wrapper_t()
    {
        memkind_destroy_kind(kind);
    }

    memkind_t get() const
    {
        return kind;
    }

private:
    memkind_t kind;
    struct memkind_config *cfg;
};
} // namespace internal

template <typename T>
class allocator
{
    using kind_wrapper_t = internal::kind_wrapper_t;
    std::shared_ptr<kind_wrapper_t> kind_wrapper_ptr;

public:
    using value_type = T;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    template <class U>
    struct rebind {
        using other = allocator<U>;
    };

    template <typename U>
    friend class allocator;

#if !_GLIBCXX_USE_CXX11_ABI
    /* This is a workaround for compilers (e.g GCC 4.8) that uses C++11
     * standard, but use old - non C++11 ABI */
    template <typename V = void>
    explicit allocator()
    {
        static_assert(
            std::is_same<V, void>::value,
            "libmemkind::pmem::allocator cannot be compiled without CXX11 ABI");
    }
#endif

    explicit allocator(const char *dir, size_t max_size)
        : kind_wrapper_ptr(std::make_shared<kind_wrapper_t>(dir, max_size))
    {}

    explicit allocator(const std::string &dir, size_t max_size)
        : allocator(dir.c_str(), max_size)
    {}

    explicit allocator(const char *dir, size_t max_size,
                       libmemkind::allocation_policy alloc_policy)
        : kind_wrapper_ptr(
              std::make_shared<kind_wrapper_t>(dir, max_size, alloc_policy))
    {}

    explicit allocator(const std::string &dir, size_t max_size,
                       libmemkind::allocation_policy alloc_policy)
        : allocator(dir.c_str(), max_size, alloc_policy)
    {}

    allocator(const allocator &other) = default;

    template <typename U>
    allocator(const allocator<U> &other) noexcept
        : kind_wrapper_ptr(other.kind_wrapper_ptr)
    {}

    allocator(allocator &&other) = default;

    template <typename U>
    allocator(allocator<U> &&other) noexcept
        : kind_wrapper_ptr(std::move(other.kind_wrapper_ptr))
    {}

    allocator<T> &operator=(const allocator &other) = default;

    template <typename U>
    allocator<T> &operator=(const allocator<U> &other) noexcept
    {
        kind_wrapper_ptr = other.kind_wrapper_ptr;
        return *this;
    }

    allocator<T> &operator=(allocator &&other) = default;

    template <typename U>
    allocator<T> &operator=(allocator<U> &&other) noexcept
    {
        kind_wrapper_ptr = std::move(other.kind_wrapper_ptr);
        return *this;
    }

    pointer allocate(size_type n) const
    {
        pointer result = static_cast<pointer>(
            memkind_malloc(kind_wrapper_ptr->get(), n * sizeof(T)));
        if (!result) {
            throw std::bad_alloc();
        }
        return result;
    }

    void deallocate(pointer p, size_type n) const
    {
        memkind_free(kind_wrapper_ptr->get(), static_cast<void *>(p));
    }

    template <class U, class... Args>
    void construct(U *p, Args &&... args) const
    {
        ::new ((void *)p) U(std::forward<Args>(args)...);
    }

    void destroy(pointer p) const
    {
        p->~value_type();
    }

    template <typename U, typename V>
    friend bool operator==(const allocator<U> &lhs, const allocator<V> &rhs);

    template <typename U, typename V>
    friend bool operator!=(const allocator<U> &lhs, const allocator<V> &rhs);
};

template <typename U, typename V>
bool operator==(const allocator<U> &lhs, const allocator<V> &rhs)
{
    return lhs.kind_wrapper_ptr->get() == rhs.kind_wrapper_ptr->get();
}

template <typename U, typename V>
bool operator!=(const allocator<U> &lhs, const allocator<V> &rhs)
{
    return !(lhs == rhs);
}
} // namespace pmem
} // namespace libmemkind
