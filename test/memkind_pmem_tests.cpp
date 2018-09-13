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

#include <sys/param.h>
#include <sys/mman.h>
#include <stdio.h>
#include <pthread.h>
#include "common.h"

static const size_t PMEM_PART_SIZE = MEMKIND_PMEM_MIN_SIZE + 4096;
static const size_t PMEM_NO_LIMIT = 0;
extern const char*  PMEM_DIR;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

class MemkindPmemTests: public :: testing::Test
{

protected:
    memkind_t pmem_kind;
    void SetUp()
    {
        // create PMEM partition
        int err = memkind_create_pmem(PMEM_DIR, PMEM_PART_SIZE, &pmem_kind);
        ASSERT_EQ(0, err);
        ASSERT_TRUE(NULL != pmem_kind);
    }

    void TearDown()
    {
        int err = memkind_destroy_kind(pmem_kind);
        ASSERT_EQ(0, err);
    }
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
    char *default_str = NULL;

    default_str = (char *)memkind_malloc(pmem_kind, size);
    EXPECT_TRUE(NULL != default_str);

    sprintf(default_str, "memkind_malloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);

    // Out of memory
    default_str = (char *)memkind_malloc(pmem_kind, 2 * PMEM_PART_SIZE);
    EXPECT_EQ(NULL, default_str);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemCalloc)
{
    const size_t size = 1024;
    const size_t num = 1;
    char *default_str = NULL;

    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(NULL != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);

    // allocate the buffer of the same size (likely at the same address)
    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(NULL != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemCallocHuge)
{
    const size_t size = MEMKIND_PMEM_CHUNK_SIZE;
    const size_t num = 1;
    char *default_str = NULL;

    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(NULL != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);

    // allocate the buffer of the same size (likely at the same address)
    default_str = (char *)memkind_calloc(pmem_kind, num, size);
    EXPECT_TRUE(NULL != default_str);
    EXPECT_EQ(*default_str, 0);

    sprintf(default_str, "memkind_calloc MEMKIND_PMEM\n");
    printf("%s", default_str);

    memkind_free(pmem_kind, default_str);
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemRealloc)
{
    const size_t size1 = 512;
    const size_t size2 = 1024;
    char *default_str = NULL;

    default_str = (char *)memkind_realloc(pmem_kind, default_str, size1);
    EXPECT_TRUE(NULL != default_str);

    sprintf(default_str, "memkind_realloc MEMKIND_PMEM with size %zu\n", size1);
    printf("%s", default_str);

    default_str = (char *)memkind_realloc(pmem_kind, default_str, size2);
    EXPECT_TRUE(NULL != default_str);

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

TEST_F(MemkindPmemTests,
       test_TC_MEMKIND_PmemCreateCheckErrorCodeArenaCreate)
{
    const size_t pmem_number = 25000;
    struct memkind *pmem_temp[pmem_number] = { nullptr };
    unsigned i, j = 0;
    int err = 0;

    for (i = 0; i < pmem_number; ++i) {
        err = memkind_create_pmem(PMEM_DIR, MEMKIND_PMEM_MIN_SIZE, &pmem_temp[i]);
        if ( err ) {
            EXPECT_EQ(err, MEMKIND_ERROR_ARENAS_CREATE);
            break;
        }
    }
    for (j = 0; j < i; ++j) {
        err = memkind_destroy_kind(pmem_temp[j]);
        EXPECT_EQ(err, 0);
    }
}

static void* thread_func_kinds(void* arg)
{
    memkind_t pmem_thread_kind;
    int err = 0;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    err = memkind_create_pmem(PMEM_DIR, PMEM_PART_SIZE, &pmem_thread_kind);

    if(err == 0) {
        void *test = memkind_malloc(pmem_thread_kind, sizeof(void *));
        EXPECT_TRUE(test != nullptr);

        memkind_free(pmem_thread_kind, test);
        err = memkind_destroy_kind(pmem_thread_kind);
        EXPECT_EQ(0, err);
    }

    return nullptr;
}

TEST_F(MemkindPmemTests, test_TC_MEMKIND_PmemMultithreadsStressKindsCreate)
{
    const int nthreads = 50;
    int i, t, err;
    int max_possible_kind = 0;
    memkind_t pmem_kinds[MEMKIND_MAX_KIND] = {nullptr};
    pthread_t *threads = (pthread_t*)calloc(nthreads, sizeof(pthread_t));
    ASSERT_TRUE(threads != nullptr);

    // This loop will create as many kinds as possible
    // to obtain a real kind limit
    for(i=0; i<MEMKIND_MAX_KIND; i++) {
        err = memkind_create_pmem(PMEM_DIR, PMEM_PART_SIZE, &pmem_kinds[i]);
        if(err != 0) {
            ASSERT_TRUE(i>0);
            max_possible_kind = i;
            --i;
            break;
        }
        ASSERT_TRUE(nullptr != pmem_kinds[i]);
    }

    // destroy last kind so it will be possible
    // to create only one pmem kind in threads
    err = memkind_destroy_kind(pmem_kinds[i]);
    ASSERT_EQ(0, err);

    for (t = 0; t < nthreads; t++) {
        err = pthread_create(&threads[t], nullptr, thread_func_kinds, nullptr);
        ASSERT_EQ(0, err);
    }

    sleep(1);
    pthread_cond_broadcast(&cond);

    for (t = 0; t < nthreads; t++) {
        err = pthread_join(threads[t], nullptr);
        ASSERT_EQ(0, err);
    }

    for (i = 0; i < max_possible_kind - 1; i++) {
        err = memkind_destroy_kind(pmem_kinds[i]);
        ASSERT_EQ(0, err);
    }

    free(threads);
}
