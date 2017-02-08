/*
* Copyright (C) 2017 Intel Corporation.
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

#include <stdio.h>
#include <assert.h>

class Allocator
{
public:

    virtual void* malloc(size_t size) = 0;
    virtual void* calloc(size_t num, size_t size) = 0;
    virtual void* realloc(void* ptr, size_t size) = 0;
    virtual int memalign(void** ptr, size_t alignment, size_t size) = 0;
    virtual void free(void* ptr) = 0;
    virtual bool is_interleave() = 0;
    virtual bool is_preferred() = 0;
    virtual bool is_bind() = 0;
    virtual bool is_high_bandwidth() = 0;
    virtual size_t get_page_size() = 0;

    virtual ~Allocator(void) {}
};

class MemkindAllocator : public Allocator
{
public:

    MemkindAllocator(memkind_memtype_t memtype, memkind_policy_t policy, memkind_bits_t flags)
    {
        memtype = memtype;
        policy = policy;
        flags = flags;
        int ret = memkind_create_kind(memtype, policy, flags, &kind);
        assert(ret == MEMKIND_SUCCESS);
        assert(kind != NULL);
    }

    MemkindAllocator(memkind_t kind) : kind(kind) {}

    virtual void* malloc(size_t size)
    {
        return memkind_malloc(kind, size);
    }

    virtual void* calloc(size_t num, size_t size)
    {
        return memkind_calloc(kind, num, size);
    }

    virtual void* realloc(void* ptr, size_t size)
    {
        return memkind_realloc(kind, ptr, size);
    }

    virtual int memalign(void** ptr, size_t alignment, size_t size)
    {
        return memkind_posix_memalign(kind, ptr, alignment, size);
    }

    virtual void free(void* ptr)
    {
        memkind_free(kind, ptr);
    }

    virtual bool is_interleave()
    {
        return policy == MEMKIND_POLICY_INTERLEAVE_ALL || kind == MEMKIND_HBW_INTERLEAVE || kind == MEMKIND_INTERLEAVE;
    }

    virtual bool is_preferred()
    {
        return policy == MEMKIND_POLICY_PREFERRED_LOCAL || kind == MEMKIND_HBW_PREFERRED || kind == MEMKIND_HBW_PREFERRED_HUGETLB;
    }

    virtual bool is_bind()
    {
        return policy == MEMKIND_POLICY_BIND_LOCAL || kind == MEMKIND_HBW || kind == MEMKIND_HBW_HUGETLB;
    }

    virtual bool is_high_bandwidth()
    {
        return  memtype == MEMKIND_MEMTYPE_HIGH_BANDWIDTH ||
                kind == MEMKIND_HBW ||
                kind == MEMKIND_HBW_HUGETLB ||
                kind == MEMKIND_HBW_PREFERRED ||
                kind == MEMKIND_HBW_INTERLEAVE;
    }

    virtual size_t get_page_size()
    {
        if (kind == MEMKIND_HUGETLB ||
            kind == MEMKIND_HBW_HUGETLB ||
            kind == MEMKIND_HBW_PREFERRED_HUGETLB ||
            (flags & MEMKIND_MASK_PAGE_SIZE_2MB)) {
            return 2*MB;
        }
        return 4*MB;
    }

    virtual ~MemkindAllocator()
    {
        int ret = memkind_destroy_kind(kind);
        assert(ret == MEMKIND_SUCCESS);
        kind = NULL;
    }

private:
    memkind_t kind = NULL;
    memkind_bits_t flags = memkind_bits_t();
    memkind_policy_t policy = MEMKIND_POLICY_MAX_VALUE;
    memkind_memtype_t memtype = memkind_memtype_t();
};

class HbwmallocAllocator : public Allocator
{
public:

    HbwmallocAllocator(hbw_policy_t hbw_policy)
    {
        hbw_set_policy(hbw_policy);
    }

    virtual void* malloc(size_t size)
    {
        return hbw_malloc(size);
    }

    virtual void* calloc(size_t num, size_t size)
    {
        return hbw_calloc(num, size);
    }

    virtual void* realloc(void* ptr, size_t size)
    {
        return hbw_realloc(ptr, size);
    }

    virtual int memalign(void** ptr, size_t alignment, size_t size)
    {
        return hbw_posix_memalign(ptr, alignment, size);
    }

    virtual void free(void* ptr)
    {
        hbw_free(ptr);
    }

    virtual bool is_interleave()
    {
        return hbw_get_policy() == HBW_POLICY_INTERLEAVE;
    }

    virtual bool is_preferred()
    {
        return hbw_get_policy() == HBW_POLICY_PREFERRED;
    }

    virtual bool is_bind()
    {
        return hbw_get_policy() == HBW_POLICY_BIND;
    }

    virtual bool is_high_bandwidth()
    {
        return  hbw_check_available() == 0;
    }

    virtual size_t get_page_size()
    {
        return sysconf(_SC_PAGESIZE);
    }

};

