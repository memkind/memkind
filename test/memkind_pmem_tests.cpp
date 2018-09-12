/*
 * Copyright (C) 2015 - 2018 Intel Corporation.
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

#include <memkind/internal/memkind_pmem.h>
#include <memkind/internal/memkind_private.h>
#include "allocator_perf_tool/TimerSysTime.hpp"

#include <sys/param.h>
#include <sys/mman.h>
#include <stdio.h>
#include "common.h"
#include <pthread.h>

static const size_t PMEM_PART_SIZE = MEMKIND_PMEM_MIN_SIZE + 4096;
static const size_t PMEM_NO_LIMIT = 0;
static const char*  PMEM_DIR = "/tmp/";

class MemkindPmemTests: public :: testing::Test
{

protected:
    memkind_t pmem_kind;
    void SetUp()
    {
        // create PMEM partition
        int err = memkind_create_pmem(PMEM_DIR, PMEM_PART_SIZE, &pmem_kind);
        ASSERT_EQ(0, err);
        ASSERT_TRUE(nullptr != pmem_kind);
    }

    void TearDown()
    {
        int err = memkind_destroy_kind(pmem_kind);
        ASSERT_EQ(0, err);
    }
};

class MemkindPmemTestsCalloc : public MemkindPmemTests,
    public ::testing::WithParamInterface<std::tuple<int, int>>
{
};

class MemkindPmemTestsMallocTime : public MemkindPmemTests,
    public ::testing::WithParamInterface<std::tuple<int, int>>
{
};

class MemkindPmemTestsMalloc : public MemkindPmemTests,
    public ::testing::WithParamInterface<size_t>
{
};

static void pmem_get_size(struct memkind *kind, size_t& total, size_t& free)
{
    struct memkind_pmem *priv = reinterpret_cast<struct memkind_pmem *>(kind->priv);

    total = priv->max_size;
    free = priv->max_size - priv->offset; /* rough estimation */
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemPriv)
{
    size_t total_mem = 0;
    size_t free_mem = 0;

    pmem_get_size(pmem_kind, total_mem, free_mem);

    ASSERT_TRUE(total_mem != 0);
    ASSERT_TRUE(free_mem != 0);

    EXPECT_EQ(total_mem, roundup(PMEM_PART_SIZE, MEMKIND_PMEM_CHUNK_SIZE));

    size_t offset = total_mem - free_mem;
    EXPECT_LT(offset, MEMKIND_PMEM_CHUNK_SIZE);
    EXPECT_LT(offset, total_mem);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemMalloc)
{
    const size_t size = 1024;
    char *default_str = nullptr;

    default_str = (char *)memkind_malloc(pmem_kind, size);
    EXPECT_TRUE(nullptr != default_str);

    sprintf(default_str, "memkind_malloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);

    // Out of memory
    default_str = (char *)memkind_malloc(pmem_kind, 2 * PMEM_PART_SIZE);
    EXPECT_EQ(nullptr, default_str);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemMallocZero)
{
    void *test1 = nullptr;

    test1 = memkind_malloc(pmem_kind, 0);
    ASSERT_TRUE(test1 == nullptr);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemMallocSizeMax)
{
    void *test1 = nullptr;
    errno = 0;

    test1 = memkind_malloc(pmem_kind, SIZE_MAX);
    ASSERT_TRUE(test1 == nullptr);
    ASSERT_TRUE(errno == ENOMEM);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemCalloc)
{
    const size_t size = 1024;
    const size_t num = 1;
    char *default_str = nullptr;

    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(nullptr != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);

    // allocate the buffer of the same size (likely at the same address)
    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(nullptr != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);
}

TEST_P(MemkindPmemTestsCalloc, test_TC_MEMKIND_PmemCallocSize)
{
    void *test = nullptr;
    size_t size = std::get<0>(GetParam());
    size_t num = std::get<1>(GetParam());
    errno = 0;

    test = memkind_calloc(pmem_kind, size, num);
    ASSERT_TRUE(test == nullptr);

    if(size == SIZE_MAX || num == SIZE_MAX) {
        ASSERT_TRUE(errno == ENOMEM);
    }
}

INSTANTIATE_TEST_CASE_P(
    CallocParam, MemkindPmemTestsCalloc,
    ::testing::Values(std::make_tuple(10, 0),
                      std::make_tuple(0, 0),
                      std::make_tuple(SIZE_MAX, 1),
                      std::make_tuple(10, SIZE_MAX)));

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemCallocHuge)
{
    const size_t size = MEMKIND_PMEM_CHUNK_SIZE;
    const size_t num = 1;
    char *default_str = nullptr;

    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(nullptr != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);

    // allocate the buffer of the same size (likely at the same address)
    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(nullptr != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemRealloc)
{
    const size_t size1 = 512;
    const size_t size2 = 1024;
    char *default_str = nullptr;

    default_str = (char *)memkind_realloc(pmem_kind, default_str, size1);
    EXPECT_TRUE(nullptr != default_str);

    sprintf(default_str, "memkind_realloc MEMKIND_PMEM with size %zu\n", size1);
    printf("%s", default_str);

    default_str = (char *)memkind_realloc(pmem_kind, default_str, size2);
    EXPECT_TRUE(nullptr != default_str);

    sprintf(default_str, "memkind_realloc MEMKIND_PMEM with size %zu\n", size2);
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemMallocUsableSize)
{
    const struct {
        size_t size;
        size_t spacing;
    } check_sizes[] = {
        {.size = 10, .spacing = 8},
        {.size = 100, .spacing = 16},
        {.size = 200, .spacing = 32},
        {.size = 500, .spacing = 64},
        {.size = 1000, .spacing = 128},
        {.size = 2000, .spacing = 256},
        {.size = 3000, .spacing = 512},
        {.size = 1 * 1024 * 1024, .spacing = 4 * 1024 * 1024},
        {.size = 2 * 1024 * 1024, .spacing = 4 * 1024 * 1024},
        {.size = 3 * 1024 * 1024, .spacing = 4 * 1024 * 1024},
        {.size = 4 * 1024 * 1024, .spacing = 4 * 1024 * 1024}
    };
    struct memkind *pmem_temp = nullptr;

    int err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_temp);
    EXPECT_EQ(0, err);
    EXPECT_TRUE(nullptr != pmem_temp);
    size_t usable_size = memkind_malloc_usable_size(pmem_temp, nullptr);
    EXPECT_EQ(0u, usable_size);
    for (unsigned int i = 0 ; i < (sizeof(check_sizes) / sizeof(check_sizes[0]));
         ++i) {
        size_t size = check_sizes[i].size;
        void *alloc = memkind_malloc(pmem_temp, size);
        EXPECT_TRUE(nullptr != alloc);
        usable_size = memkind_malloc_usable_size(pmem_temp, alloc);
        size_t diff = usable_size - size;
        EXPECT_GE(usable_size, size);
        EXPECT_LE(diff, check_sizes[i].spacing);
        memkind_free(pmem_temp, alloc);
    }
    err = memkind_destroy_kind(pmem_temp);
    EXPECT_EQ(0, err);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemResize)
{
    const size_t size = MEMKIND_PMEM_CHUNK_SIZE;
    char *pmem_str10 = nullptr;
    char *pmem_strX = nullptr;
    memkind_t pmem_kind_no_limit = nullptr;
    memkind_t pmem_kind_not_possible = nullptr;
    int err = 0;

    pmem_str10 = (char *)memkind_malloc(pmem_kind, MEMKIND_PMEM_MIN_SIZE);
    EXPECT_TRUE(nullptr != pmem_str10);

    // Out of memory
    pmem_strX = (char *)memkind_malloc(pmem_kind, size);
    EXPECT_TRUE(nullptr == pmem_strX);

    memkind_free(pmem_kind, pmem_str10);
    memkind_free(pmem_kind, pmem_strX);

    err = memkind_create_pmem(PMEM_DIR, PMEM_NO_LIMIT, &pmem_kind_no_limit);
    EXPECT_EQ(0, err);
    EXPECT_TRUE(nullptr != pmem_kind_no_limit);

    pmem_str10 = (char *)memkind_malloc(pmem_kind_no_limit, MEMKIND_PMEM_MIN_SIZE);
    EXPECT_TRUE(nullptr != pmem_str10);

    pmem_strX = (char *)memkind_malloc(pmem_kind_no_limit, size);
    EXPECT_TRUE(nullptr != pmem_strX);

    memkind_free(pmem_kind_no_limit, pmem_str10);
    memkind_free(pmem_kind_no_limit, pmem_strX);

    err = memkind_destroy_kind(pmem_kind_no_limit);
    EXPECT_EQ(0, err);

    err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE-1,
                              &pmem_kind_not_possible);
    EXPECT_EQ(MEMKIND_ERROR_INVALID, err);
    EXPECT_TRUE(nullptr == pmem_kind_not_possible);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemReallocZero)
{
    size_t size = 1024;
    void *test = nullptr;
    void *new_test = nullptr;

    test = memkind_malloc(pmem_kind, size);
    ASSERT_TRUE(test != nullptr);

    new_test = memkind_realloc(pmem_kind, test, 0);
    ASSERT_TRUE(new_test == nullptr);

    memkind_free(pmem_kind, new_test);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemReallocSizeMax)
{
    size_t size = 1024;
    void *test = nullptr;
    void *new_test = nullptr;

    test = memkind_malloc(pmem_kind, size);
    ASSERT_TRUE(test != nullptr);
    errno = 0;
    new_test = memkind_realloc(pmem_kind, test, SIZE_MAX);
    ASSERT_TRUE(new_test == nullptr);
    ASSERT_TRUE(errno == ENOMEM);

    memkind_free(pmem_kind, test);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemReallocNullptr)
{
    size_t size = 1024;
    void *test = nullptr;

    test = memkind_realloc(pmem_kind, test, size);
    ASSERT_TRUE(test != nullptr);

    memkind_free(pmem_kind, test);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemReallocNullptrZero)
{
    void *test = nullptr;

    test = memkind_realloc(pmem_kind, test, 0);
    ASSERT_TRUE(test == nullptr);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemReallocIncreaseSize)
{
    size_t size = 1024;
    char *test1 = nullptr;
    char *test2 = nullptr;
    const char val[] = "test_TC_MEMKIND_PmemReallocIncreaseSize";
    int status;

    test1 = (char*)memkind_malloc(pmem_kind, size);
    ASSERT_TRUE(test1 != nullptr);

    sprintf(test1, "%s", val);

    size *= 2;
    test2 = (char*)memkind_realloc(pmem_kind, test1, size);
    ASSERT_TRUE(test2 != nullptr);
    status = memcmp(val, test2, sizeof(val));
    ASSERT_TRUE(status == 0);

    memkind_free(pmem_kind, test2);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemReallocDecreaseSize)
{
    size_t size = 1024;
    char *test1 = nullptr;
    char *test2 = nullptr;
    const char val[] = "test_TC_MEMKIND_PmemReallocIncreaseSize";
    int status;

    test1 = (char*)memkind_malloc(pmem_kind, size);
    ASSERT_TRUE(test1 != nullptr);

    sprintf(test1, "%s", val);

    size = 4;
    test2 = (char*)memkind_realloc(pmem_kind, test1, size);
    ASSERT_TRUE(test2 != nullptr);
    status = memcmp(val, test2, size);
    ASSERT_TRUE(status == 0);

    memkind_free(pmem_kind, test2);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemReallocInPlace)
{
    void *test1 = memkind_malloc(pmem_kind, 10 * 1024 * 1024);
    ASSERT_TRUE(test1 != nullptr);

    void *test1r = memkind_realloc(pmem_kind, test1, 6 * 1024 * 1024);
    ASSERT_EQ(test1r, test1);

    test1r = memkind_realloc(pmem_kind, test1, 10 * 1024 * 1024);
    ASSERT_EQ(test1r, test1);

    test1r = memkind_realloc(pmem_kind, test1, 8 * 1024 * 1024);
    ASSERT_EQ(test1r, test1);

    void *test2 = memkind_malloc(pmem_kind, 4 * 1024 * 1024);
    ASSERT_TRUE(test2 != nullptr);

    /* 4MB => 16B */
    void *test2r = memkind_realloc(pmem_kind, test2, 16);
    ASSERT_TRUE(test2r != nullptr);
    /* ... but the usable size is still 4MB. */

    /* 8MB => 16B */
    test1r = memkind_realloc(pmem_kind, test1, 16);

    /*
     * If the old size of the allocation is larger than
     * the chunk size (4MB), we can reallocate it to 4MB first (in place),
     * releasing some space, which makes it possible to do the actual
     * shrinking...
     */
    ASSERT_TRUE(test1r != nullptr);
    //ASSERT_NE(test1r, test1);

    /* ... and leaves some memory for new allocations. */
    void *test3 = memkind_malloc(pmem_kind, 5 * 1024 * 1024);
    ASSERT_TRUE(test3 != nullptr);

    memkind_free(pmem_kind, test1r);
    memkind_free(pmem_kind, test2r);
    memkind_free(pmem_kind, test3);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemMaxFill)
{
    void *test1 = nullptr;
    int i=16800000;
    do {
        test1 = memkind_malloc(pmem_kind, --i);
    } while (test1 == nullptr && i>0);
    ASSERT_NE(i, 0);

    void *test2 = nullptr;
    int j=2000000;
    do {
        test2 = memkind_malloc(pmem_kind, --j);
    } while (test2 == nullptr && j>0);
    ASSERT_NE(j, 0);

    void *test3 = nullptr;
    int l=100000;
    do {
        test2 = memkind_malloc(pmem_kind, --l);
    } while (test3 == nullptr && l>0);
    ASSERT_EQ(l, 0);

    memkind_free(pmem_kind, test1);
    memkind_free(pmem_kind, test2);
    memkind_free(pmem_kind, test3);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemFreeNullptr)
{
    const double test_time = 5;

    TimerSysTime timer;
    timer.start();
    do {
        memkind_free(pmem_kind, nullptr);
    } while(timer.getElapsedTime() < test_time);
}

TEST_P(MemkindPmemTestsMallocTime, test_TC_MEMKIND_PmemMallocTime)
{
    size_t size = std::get<0>(GetParam());
    size_t size_max = std::get<1>(GetParam());
    const double test_malloc_time = 5;
    void *test = nullptr;

    TimerSysTime timer;
    timer.start();
    do {
        if(size < size_max)
            ++size;
        else
            size = std::get<0>(GetParam());
        test = memkind_malloc(pmem_kind, size);
        ASSERT_TRUE(test != nullptr);
        memkind_free(pmem_kind, test);
    } while(timer.getElapsedTime() < test_malloc_time);
}

INSTANTIATE_TEST_CASE_P(
    MallocTimeParam, MemkindPmemTestsMallocTime,
    ::testing::Values(std::make_tuple(32, 32),
                      std::make_tuple(896, 896),
                      std::make_tuple(4096, 4096),
                      std::make_tuple(131072, 131072),
                      std::make_tuple(2*1024*1024, 2*1024*1024),
                      std::make_tuple(5*1024*1024, 5*1024*1024),
                      std::make_tuple(32, 4096),
                      std::make_tuple(4096, 98304),
                      std::make_tuple(114688, 196608),
                      std::make_tuple(500000, 2*1024*1024),
                      std::make_tuple(2*1024*1024, 4*1024*1024),
                      std::make_tuple(5*1024*1024, 6*1024*1024),
                      std::make_tuple(5*1024*1024, 8*1024*1024),
                      std::make_tuple(32, 9*1024*1024)));

TEST_P(MemkindPmemTestsMalloc, test_TC_MEMKIND_PmemMallocSize)
{
    void *test[1000000];
    int i, max;

    for(int j=0; j<10; j++) {
        i =0;
        do {
            test[i] = memkind_malloc(pmem_kind, GetParam());
        } while(test[i] != nullptr && i++<1000000);

        if(j == 0)
            max = i;
        else
            ASSERT_TRUE(i > 0.98*max);

        while(i > 0) {
            memkind_free(pmem_kind, test[--i]);
            test[i] = nullptr;
        }
    }
}

INSTANTIATE_TEST_CASE_P(
    MallocParam, MemkindPmemTestsMalloc,
    ::testing::Values(32, 60, 80, 100, 128, 150, 160, 250, 256, 300, 320,
                      500, 512, 800, 896, 3000, 4096, 6000, 10000, 60000,
                      98304, 114688, 131072, 163840, 196608, 500000,
                      2*1024*1024, 5*1024*1024));

TEST_F(MemkindPmemTests,
       test_TC_MEMKIND_PmemPosixMemalignWrongAlignmentLessThanVoidAndNotPowerOfTwo)
{
    void *test = nullptr;
    size_t size = 32;
    size_t wrong_alignment = 3;
    int ret;

    ret = memkind_posix_memalign(pmem_kind, &test, wrong_alignment, size);
    ASSERT_TRUE(ret == EINVAL);
    ASSERT_TRUE(test == nullptr);
}

TEST_F(MemkindPmemTests,
       test_TC_MEMKIND_PmemPosixMemalignWrongAlignmentLessThanVoidAndPowerOfTwo)
{
    void *test = nullptr;
    size_t size = 32;
    size_t wrong_alignment = sizeof(void*)/2;
    int ret;

    ret = memkind_posix_memalign(pmem_kind, &test, wrong_alignment, size);
    ASSERT_TRUE(ret == EINVAL);
    ASSERT_TRUE(test == nullptr);
}

TEST_F(MemkindPmemTests,
       test_TC_MEMKIND_PmemPosixMemalignWrongAlignmentNotPowerOfTwo)
{
    void *test = nullptr;
    size_t size = 32;
    size_t wrong_alignment = sizeof(void*)+1;
    int ret;

    ret = memkind_posix_memalign(pmem_kind, &test, wrong_alignment, size);
    ASSERT_TRUE(ret == EINVAL);
    ASSERT_TRUE(test == nullptr);
}

TEST_F(MemkindPmemTests,
       test_TC_MEMKIND_PmemPosixMemalignLowestCorrectAlignment)
{
    void *test = nullptr;
    size_t size = 32;
    size_t alignment = sizeof(void*);
    int ret;

    ret = memkind_posix_memalign(pmem_kind, &test, alignment, size);
    ASSERT_TRUE(ret == 0);
    ASSERT_TRUE(test != nullptr);

    memkind_free(pmem_kind, test);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemPosixMemalignSizeZero)
{
    void *test = nullptr;
    size_t alignment = sizeof(void*);
    int ret;

    ret = memkind_posix_memalign(pmem_kind, &test, alignment, 0);
    ASSERT_TRUE(ret != 0);
    ASSERT_TRUE(test == nullptr);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemPosixMemalignSizeMax)
{
    void *test = nullptr;
    size_t alignment = 64;
    int ret;

    ret = memkind_posix_memalign(pmem_kind, &test, alignment, SIZE_MAX);
    ASSERT_TRUE(ret == ENOMEM);
    ASSERT_TRUE(test == nullptr);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemPosixMemalign)
{
    const int max_allocs = 1000000;
    const int test_value = 123456;
    uintptr_t alignment;
    unsigned i;
    int *ptrs[max_allocs];
    void *ptr;
    int ret;

    for(alignment = 1024; alignment < 140000; alignment *= 2) {
        for (int j = 0; j < 10; j++) {
            memset(ptrs, 0, max_allocs * sizeof(ptrs[0]));
            for (i = 0; i < max_allocs; ++i) {
                errno = 0;
                ret = memkind_posix_memalign(pmem_kind, &ptr, alignment, sizeof(int *));
                if(ret != 0)
                    break;

                EXPECT_EQ(ret, 0);
                EXPECT_EQ(errno, 0);

                ptrs[i] = (int *)ptr;

                //at least one allocation must succeed
                ASSERT_TRUE(i != 0 || ptr != nullptr);
                if (ptr == nullptr)
                    break;

                //ptr should be usable
                *(int*)ptr = test_value;
                ASSERT_EQ(*(int*)ptr, test_value);

                //check for correct address alignment
                ASSERT_EQ((uintptr_t)(ptr) & (alignment - 1), (uintptr_t)0);
            }

            for (i = 0; i < max_allocs; ++i) {
                if (ptrs[i] == nullptr)
                    break;

                memkind_free(pmem_kind, ptrs[i]);
            }
        }
    }
}

static memkind_t *pools;
static int npools = 3;
static void* thread_func(void* arg)
{
    int start_idx = *(int *)arg;
    int err = 0;
    for (int idx = 0; idx < npools; ++idx) {
        int pool_id = start_idx + idx;

        if (pools[pool_id] == nullptr) {
            err = memkind_create_pmem(PMEM_DIR, PMEM_PART_SIZE, &pools[pool_id]);
            EXPECT_EQ(0, err);
        }

        if (err == 0) {
            void *test = memkind_malloc(pools[pool_id], sizeof(void *));
            EXPECT_TRUE(test != nullptr);
            memkind_free(pools[pool_id], test);
            memkind_destroy_kind(pools[pool_id]);
        }
    }

    return nullptr;
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemMultithreads)
{
    int nthreads = 10;
    pthread_t *threads = (pthread_t*)calloc(nthreads, sizeof(pthread_t));
    ASSERT_TRUE(threads != nullptr);
    int *pool_idx = (int*)calloc(nthreads, sizeof(int));
    ASSERT_TRUE(pool_idx != nullptr);
    pools = (memkind_t*)calloc(npools * nthreads, sizeof(memkind_t));
    ASSERT_TRUE(pools != nullptr);

    for (int t = 0; t < nthreads; t++) {
        pool_idx[t] = npools * t;
        pthread_create(&threads[t], nullptr, thread_func, &pool_idx[t]);
    }

    for (int t = 0; t < nthreads; t++) {
        pthread_join(threads[t], nullptr);
    }

    free(pools);
    free(threads);
    free(pool_idx);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemDestroyKind)
{
    const size_t pmem_array_size = 10;
    struct memkind *pmem_kind_array[pmem_array_size] = {nullptr};

    int err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE,
                                  &pmem_kind_array[0]);
    EXPECT_EQ(err, 0);

    err = memkind_destroy_kind(pmem_kind_array[0]);
    EXPECT_EQ(err, 0);

    for (unsigned int i = 0; i < pmem_array_size; ++i) {
        err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_kind_array[i]);
        EXPECT_EQ(err, 0);
    }

    char *pmem_middle_name = pmem_kind_array[5]->name;
    err = memkind_destroy_kind(pmem_kind_array[5]);
    EXPECT_EQ(err, 0);

    err = memkind_destroy_kind(pmem_kind_array[6]);
    EXPECT_EQ(err, 0);

    err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_kind_array[5]);
    EXPECT_EQ(err, 0);

    char *pmem_new_middle_name = pmem_kind_array[5]->name;

    EXPECT_STREQ(pmem_middle_name, pmem_new_middle_name);

    for (unsigned int i = 0; i < pmem_array_size; ++i) {
        if (i != 6) {
            err = memkind_destroy_kind(pmem_kind_array[i]);
            EXPECT_EQ(err, 0);
        }
    }
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemDestroyKindArenaZero)
{
    struct memkind *pmem_temp_1 = nullptr;
    struct memkind *pmem_temp_2 = nullptr;
    unsigned int arena_zero = 0;
    int err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_temp_1);
    EXPECT_EQ(err, 0);

    arena_zero = pmem_temp_1->arena_zero;
    err = memkind_destroy_kind(pmem_temp_1);
    EXPECT_EQ(err, 0);
    err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_temp_2);
    EXPECT_EQ(err, 0);

    EXPECT_EQ(arena_zero,pmem_temp_2->arena_zero);

    err = memkind_destroy_kind(pmem_temp_2);
    EXPECT_EQ(err, 0);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemCreateDestroyKindLoop)
{
    struct memkind *pmem_temp = nullptr;
    const size_t pmem_number = 25000;

    for (unsigned int i = 0; i < pmem_number; ++i) {
        int err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_temp);
        EXPECT_EQ(err, 0);
        err = memkind_destroy_kind(pmem_temp);
        EXPECT_EQ(err, 0);
    }
}

TEST_F(MemkindPmemTests,
       test_TC_MEMKIND_PmemCreateDestroyKindLoopWithMallocSmallSize)
{
    struct memkind *pmem_temp = nullptr;
    const size_t pmem_number = 25000;
    const size_t size = 1024;

    for (unsigned int i = 0; i < pmem_number; ++i) {
        int err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_temp);
        EXPECT_EQ(err, 0);
        void *ptr = memkind_malloc(pmem_temp, size);
        EXPECT_TRUE(nullptr != ptr);
        memkind_free(pmem_temp, ptr);
        err = memkind_destroy_kind(pmem_temp);
        EXPECT_EQ(err, 0);
    }
}

TEST_F(MemkindPmemTests,
       test_TC_MEMKIND_PmemCreateDestroyKindLoopWithMallocChunkSize)
{
    struct memkind *pmem_temp = nullptr;
    const size_t pmem_number = 25000;
    const size_t size = MEMKIND_PMEM_CHUNK_SIZE;

    for (unsigned int i = 0; i < pmem_number; ++i) {
        int err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_temp);
        EXPECT_EQ(err, 0);
        void *ptr = memkind_malloc(pmem_temp, size);
        EXPECT_TRUE(nullptr != ptr);
        memkind_free(pmem_temp, ptr);
        err = memkind_destroy_kind(pmem_temp);
        EXPECT_EQ(err, 0);
    }
}

TEST_F(MemkindPmemTests,
       test_TC_MEMKIND_PmemCreateDestroyKindLoopWithRealloc)
{
    struct memkind *pmem_temp = nullptr;
    const size_t pmem_number = 25000;
    const size_t size_1 = 512;
    const size_t size_2 = 1024;

    for (unsigned int i = 0; i < pmem_number; ++i) {
        int err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_temp);
        EXPECT_EQ(err, 0);
        void *ptr = memkind_malloc(pmem_temp, size_1);
        EXPECT_TRUE(nullptr != ptr);
        void *ptr_2 = memkind_realloc(pmem_temp, ptr, size_2);
        EXPECT_TRUE(nullptr != ptr_2);
        memkind_free(pmem_temp, ptr_2);
        err = memkind_destroy_kind(pmem_temp);
        EXPECT_EQ(err, 0);
    }
}
