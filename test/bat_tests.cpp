/*
 * Copyright (C) 2014 - 2017 Intel Corporation.
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

#include "memkind.h"

#include <fstream>
#include <algorithm>

#include "common.h"
#include "check.h"
#include "omp.h"
#include "trial_generator.h"

/*
 * Set of basic acceptance tests.
 */
class BATest: public TGTest
{
};

class BasicAllocTest
{
public:
    memkind_bits_t object_flags;

    void init(memkind_memtype_t memtype, memkind_policy_t policy, memkind_bits_t flags)
    {
        if(!kind) {
            int ret = memkind_create_kind(memtype, policy, flags, &kind);
            ASSERT_EQ(MEMKIND_SUCCESS, ret);
            ASSERT_TRUE(kind != NULL);
        }
        object_flags = flags;
    }

    void record_page_association(void* ptr, size_t size)
    {
        size_t page_size = object_flags == MEMKIND_MASK_PAGE_SIZE_2MB ? 2*MB : 4*KB;

        Check check(ptr, size, page_size);
        check.record_page_association();
    }

    void malloc(size_t size)
    {
        void* ptr = memkind_malloc(kind, size);
        ASSERT_TRUE(ptr != NULL) << "memkind_malloc() returns NULL";
        void* memset_ret = memset(ptr, 3, size);
        EXPECT_TRUE(memset_ret != NULL);
        record_page_association(ptr, size);
        memkind_free(kind, ptr);
    }

    void calloc(size_t num, size_t size)
    {
        void* ptr = memkind_calloc(kind, num, size);
        ASSERT_TRUE(ptr != NULL) << "memkind_calloc() returns NULL";
        void* memset_ret = memset(ptr, 3, size);
        ASSERT_TRUE(memset_ret != NULL);
        record_page_association(ptr, size);
        memkind_free(kind, ptr);
    }

    void realloc(size_t size)
    {
        void* ptr = memkind_malloc(kind, size);
        ASSERT_TRUE(ptr != NULL) << "memkind_malloc() returns NULL";
        size_t realloc_size = size+128;
        ptr = memkind_realloc(kind, ptr, realloc_size);
        ASSERT_TRUE(ptr != NULL) << "memkind_realloc() returns NULL";
        void* memset_ret = memset(ptr, 3, realloc_size);
        ASSERT_TRUE(memset_ret != NULL);
        record_page_association(ptr, size);
        memkind_free(kind, ptr);
    }

    void memalign(size_t alignment, size_t size)
    {
        void* ptr = NULL;
        int ret = memkind_posix_memalign(kind, &ptr, alignment, size);
        ASSERT_EQ(0, ret) << "memkind_posix_memalign() != 0";
        ASSERT_TRUE(ptr != NULL) << "memkind_posix_memalign() returns NULL pointer";
        void* memset_ret = memset(ptr, 3, size);
        ASSERT_TRUE(memset_ret != NULL);
        record_page_association(ptr, size);
        memkind_free(kind, ptr);
    }

    void free(size_t size)
    {
        void* ptr = memkind_malloc(kind, size);
        ASSERT_TRUE(ptr != NULL) << "memkind_malloc() returns NULL";
        memkind_free(kind, ptr);
        ptr = memkind_malloc(kind, size);
        ASSERT_TRUE(ptr != NULL) << "memkind_malloc() returns NULL";
        memkind_free(0, ptr);
    }

    void destroy_kind()
    {
        int ret = memkind_destroy_kind(kind);
        ASSERT_EQ(MEMKIND_SUCCESS, ret);
        kind = NULL;
    }

    virtual ~BasicAllocTest() {destroy_kind();}
private:
    memkind_t kind = NULL;
};

static void test_malloc(memkind_memtype_t memtype, memkind_policy_t policy, memkind_bits_t flags, size_t size)
{
    BasicAllocTest test;
    test.init(memtype, policy, flags);
    test.malloc(size);
}

static void test_calloc(memkind_memtype_t memtype, memkind_policy_t policy, memkind_bits_t flags, size_t size)
{
    BasicAllocTest test;
    test.init(memtype, policy, flags);
    test.calloc(1, size);
}

static void test_realloc(memkind_memtype_t memtype, memkind_policy_t policy, memkind_bits_t flags, size_t size)
{
    BasicAllocTest test;
    test.init(memtype, policy, flags);
    test.realloc(size);
}


static void test_memalign(memkind_memtype_t memtype, memkind_policy_t policy, memkind_bits_t flags, size_t size)
{
    BasicAllocTest test;
    test.init(memtype, policy, flags);
    test.memalign(4096, size);
}

static void test_free(memkind_memtype_t memtype, memkind_policy_t policy, memkind_bits_t flags, size_t size)
{
    BasicAllocTest test;
    test.init(memtype, policy, flags);
    test.free(size);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_DEFAULT_PREFERRED_LOCAL_4096_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_DEFAULT_PREFERRED_LOCAL_4194305_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_DEFAULT_PREFERRED_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_DEFAULT_PREFERRED_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_BIND_LOCAL_4096_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_BIND_LOCAL_4194305_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_BIND_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_BIND_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_4096_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_4194305_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_INTERLEAVE_ALL_4096_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_INTERLEAVE_ALL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_HIGH_BANDWIDTH_INTERLEAVE_ALL_4194305_bytes)
{
    test_malloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_INTERLEAVE_ALL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_malloc_DEFAULT_HIGH_BANDWIDTH_INTERLEAVE_ALL_4194305_bytes)
{
    test_malloc((memkind_memtype_t)(MEMKIND_MEMTYPE_DEFAULT | MEMKIND_MEMTYPE_HIGH_BANDWIDTH), MEMKIND_POLICY_INTERLEAVE_ALL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_DEFAULT_PREFERRED_LOCAL_4096_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_DEFAULT_PREFERRED_LOCAL_4194305_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_DEFAULT_PREFERRED_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_DEFAULT_PREFERRED_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_BIND_LOCAL_4096_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_BIND_LOCAL_4194305_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_BIND_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_BIND_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_4096_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_4194305_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_INTERLEAVE_ALL_4096_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_INTERLEAVE_ALL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_calloc_HIGH_BANDWIDTH_INTERLEAVE_ALL_4194305_bytes)
{
    test_calloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_INTERLEAVE_ALL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_DEFAULT_PREFERRED_LOCAL_4096_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_DEFAULT_PREFERRED_LOCAL_4194305_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_DEFAULT_PREFERRED_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_DEFAULT_PREFERRED_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_BIND_LOCAL_4096_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_BIND_LOCAL_4194305_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_BIND_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_BIND_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_4096_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_4194305_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_PREFERRED_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_INTERLEAVE_ALL_4096_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_INTERLEAVE_ALL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_realloc_HIGH_BANDWIDTH_INTERLEAVE_ALL_4194305_bytes)
{
    test_realloc(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_INTERLEAVE_ALL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_DEFAULT_PREFERRED_LOCAL_4096_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_DEFAULT_PREFERRED_LOCAL_4194305_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_DEFAULT_PREFERRED_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_DEFAULT_PREFERRED_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_BIND_LOCAL_4096_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_BIND_LOCAL_4194305_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_BIND_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_BIND_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_BIND_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_PREFERRED_LOCAL_4096_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_PREFERRED_LOCAL_4194305_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_PREFERRED_LOCAL_PAGE_SIZE_2MB_4096_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4096);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_PREFERRED_LOCAL_PAGE_SIZE_2MB_4194305_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_PREFERRED_LOCAL, MEMKIND_MASK_PAGE_SIZE_2MB, 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_INTERLEAVE_ALL_4096_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_INTERLEAVE_ALL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_memalign_HIGH_BANDWIDTH_INTERLEAVE_ALL_4194305_bytes)
{
    test_memalign(MEMKIND_MEMTYPE_HIGH_BANDWIDTH, MEMKIND_POLICY_INTERLEAVE_ALL, memkind_bits_t(), 4194305);
}

TEST_F(BATest, test_TC_MEMKIND_free_DEFAULT_PREFERRED_LOCAL_4096_bytes)
{
    test_free(MEMKIND_MEMTYPE_DEFAULT, MEMKIND_POLICY_PREFERRED_LOCAL, memkind_bits_t(), 4096);
}

TEST_F(BATest, test_TC_MEMKIND_HBW_Pref_CheckAvailable)
{
    ASSERT_EQ(0, hbw_check_available());
}

TEST_F(BATest, test_TC_MEMKIND_HBW_Pref_Policy)
{
    hbw_set_policy(HBW_POLICY_PREFERRED);
    EXPECT_EQ(HBW_POLICY_PREFERRED, hbw_get_policy());
}

TEST_F(BATest, test_TC_MEMKIND_HBW_Pref_MallocIncremental)
{
    tgen->generate_incremental(HBW_MALLOC);
    tgen->run(num_bandwidth, bandwidth);
}

TEST_F(BATest, test_TC_MEMKIND_HBW_Pref_CallocIncremental)
{
    tgen->generate_incremental(HBW_CALLOC);
    tgen->run(num_bandwidth, bandwidth);
}


TEST_F(BATest, test_TC_MEMKIND_HBW_Pref_ReallocIncremental)
{
    tgen->generate_incremental(HBW_REALLOC);
    tgen->run(num_bandwidth, bandwidth);
}

TEST_F(BATest, test_TC_MEMKIND_HBW_Pref_MemalignIncremental)
{
    tgen->generate_incremental(HBW_MEMALIGN);
    tgen->run(num_bandwidth, bandwidth);
}

TEST_F(BATest, test_TC_MEMKIND_2MBPages_HBW_Pref_MemalignPsizeIncremental)
{
    tgen->generate_incremental(HBW_MEMALIGN_PSIZE);
    tgen->run(num_bandwidth, bandwidth);
}

TEST_F(BATest, test_TC_MEMKIND_HBW_Pref_MallocRecycle)
{
    tgen->generate_recycle_incremental(MEMKIND_MALLOC);
    tgen->run(num_bandwidth, bandwidth);
}

TEST_F(BATest, test_TC_MEMKIND_2MBPages_HBW_Pref_MallocRecyclePsize)
{
    tgen->generate_recycle_psize_incremental(MEMKIND_MALLOC);
    tgen->run(num_bandwidth, bandwidth);
}
