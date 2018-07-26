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
#include "common.h"

static const size_t PMEM_PART_SIZE = MEMKIND_PMEM_MIN_SIZE + 4096;
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
        ASSERT_TRUE(NULL != pmem_kind);
    }

    void TearDown()
    {}
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
    for (unsigned int i = 0 ; i < (sizeof(check_sizes) / sizeof(check_sizes[0])); i++) {
        size_t size = check_sizes[i].size;
        void *alloc = memkind_malloc(pmem_temp, size);
        EXPECT_TRUE(nullptr != alloc);
        usable_size = memkind_malloc_usable_size(pmem_temp, alloc);
        size_t diff = usable_size - size;
        EXPECT_GE(usable_size, size);
        EXPECT_LE(diff, check_sizes[i].spacing);
        memkind_free(pmem_temp, alloc);
    }
}
