// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2020 Intel Corporation. */
#pragma once

#include <assert.h>
#include <iterator>
#include <map>
#include <stdio.h>

class Allocator
{
public:
    virtual void *malloc(size_t size) = 0;
    virtual void *calloc(size_t num, size_t size) = 0;
    virtual void *realloc(void *ptr, size_t size) = 0;
    virtual int memalign(void **ptr, size_t alignment, size_t size) = 0;
    virtual void free(void *ptr) = 0;

    /* get_numa_policy() returns MPOL_INTERLEAVE, MPOL_BIND, MPOL_PREFERRED
     or -1 when the allocator is not providing NUMA policy */
    virtual int get_numa_policy() = 0;
    virtual bool is_high_bandwidth() = 0;
    virtual size_t get_page_size() = 0;

    virtual ~Allocator(void)
    {}
};

class MemkindAllocator: public Allocator
{
public:
    MemkindAllocator(memkind_memtype_t memtype, memkind_policy_t policy,
                     memkind_bits_t flags)
        : memtype(memtype), flags(flags)
    {
        int ret = memkind_create_kind(memtype, policy, flags, &kind);
        assert(ret == MEMKIND_SUCCESS);
        assert(kind != NULL);
    }

    MemkindAllocator(memkind_t kind) : kind(kind)
    {
        assert(kind != NULL);
    }

    virtual void *malloc(size_t size)
    {
        return memkind_malloc(kind, size);
    }

    virtual void *calloc(size_t num, size_t size)
    {
        return memkind_calloc(kind, num, size);
    }

    virtual void *realloc(void *ptr, size_t size)
    {
        return memkind_realloc(kind, ptr, size);
    }

    virtual int memalign(void **ptr, size_t alignment, size_t size)
    {
        return memkind_posix_memalign(kind, ptr, alignment, size);
    }

    virtual void free(void *ptr)
    {
        memkind_free(kind, ptr);
    }

    virtual int get_numa_policy()
    {
        static std::map<memkind_t, int> kind_policy = {
            std::make_pair(MEMKIND_HBW_INTERLEAVE, MPOL_INTERLEAVE),
            std::make_pair(MEMKIND_INTERLEAVE, MPOL_INTERLEAVE),
            std::make_pair(MEMKIND_HBW_PREFERRED, MPOL_PREFERRED),
            std::make_pair(MEMKIND_HBW_PREFERRED_HUGETLB, MPOL_PREFERRED),
            std::make_pair(MEMKIND_HBW, MPOL_BIND),
            std::make_pair(MEMKIND_HBW_HUGETLB, MPOL_BIND),
            std::make_pair(MEMKIND_REGULAR, MPOL_BIND),
            std::make_pair(MEMKIND_DEFAULT, MPOL_DEFAULT),
            std::make_pair(MEMKIND_HUGETLB, MPOL_DEFAULT),
            std::make_pair(MEMKIND_HBW_ALL_HUGETLB, MPOL_BIND),
            std::make_pair(MEMKIND_HBW_ALL, MPOL_BIND)};
        auto it = kind_policy.find(kind);

        if (it != std::end(kind_policy)) {
            return it->second;
        }
        return -1;
    }

    virtual bool is_high_bandwidth()
    {
        return memtype == MEMKIND_MEMTYPE_HIGH_BANDWIDTH ||
            kind == MEMKIND_HBW || kind == MEMKIND_HBW_HUGETLB ||
            kind == MEMKIND_HBW_PREFERRED || kind == MEMKIND_HBW_INTERLEAVE;
    }

    virtual size_t get_page_size()
    {
        if (kind == MEMKIND_HUGETLB || kind == MEMKIND_HBW_HUGETLB ||
            kind == MEMKIND_HBW_PREFERRED_HUGETLB ||
            (flags & MEMKIND_MASK_PAGE_SIZE_2MB)) {
            return 2 * MB;
        }
        return 4 * KB;
    }

    virtual ~MemkindAllocator()
    {
        int ret = memkind_destroy_kind(kind);
        assert(ret == MEMKIND_SUCCESS);
        kind = NULL;
    }

private:
    memkind_t kind = NULL;
    memkind_memtype_t memtype = memkind_memtype_t();
    memkind_bits_t flags = memkind_bits_t();
};

class HbwmallocAllocator: public Allocator
{
public:
    HbwmallocAllocator(hbw_policy_t hbw_policy)
    {
        hbw_set_policy(hbw_policy);
    }

    virtual void *malloc(size_t size)
    {
        return hbw_malloc(size);
    }

    virtual void *calloc(size_t num, size_t size)
    {
        return hbw_calloc(num, size);
    }

    virtual void *realloc(void *ptr, size_t size)
    {
        return hbw_realloc(ptr, size);
    }

    virtual void set_memalign_page_size(hbw_pagesize_t psize)
    {
        page_size = psize;
    }

    virtual int memalign(void **ptr, size_t alignment, size_t size)
    {
        if (page_size == HBW_PAGESIZE_4KB) {
            return hbw_posix_memalign(ptr, alignment, size);
        } else {
            return hbw_posix_memalign_psize(ptr, alignment, size, page_size);
        }
    }

    virtual void free(void *ptr)
    {
        hbw_free(ptr);
    }

    virtual int get_numa_policy()
    {
        if (hbw_get_policy() == HBW_POLICY_INTERLEAVE) {
            return MPOL_INTERLEAVE;
        } else if (hbw_get_policy() == HBW_POLICY_PREFERRED) {
            return MPOL_PREFERRED;
        } else if (hbw_get_policy() == HBW_POLICY_BIND ||
                   hbw_get_policy() == HBW_POLICY_BIND_ALL) {
            return MPOL_BIND;
        }
        return -1;
    }

    virtual bool is_high_bandwidth()
    {
        return hbw_check_available() == 0;
    }

    virtual size_t get_page_size()
    {
        if (page_size == HBW_PAGESIZE_2MB) {
            return 2 * MB;
        }
        return 4 * KB;
    }

private:
    hbw_pagesize_t page_size = HBW_PAGESIZE_4KB;
};
