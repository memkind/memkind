/*
 * Copyright (c) 2018 - 2019 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memkind.h>
#include <memkind/internal/memkind_default.h>
#include <stdio.h>
#include "common.h"

#define PMEM_MAX_SIZE (1024 * 1024 * 32)
extern const char *PMEM_DIR;

class DefragTest : public ::testing::Test
{};

TEST_F(DefragTest, test_TC_hintFalse)
{
    struct memkind *kind = NULL;

    int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
    if (err != 0)
        printf("err = %d\n", err);

    int bin = -1;
    int run = -1;
    int hint = -1;
    int alloc = 50000;
    void* ptr[alloc];

    for (int i = 0; i < alloc; i++) {
        ptr[i] = memkind_malloc(kind, 128);
    }

    hint = memkind_default_get_defrag_hint(ptr[alloc-1], &bin, &run);
    ASSERT_EQ(hint, 0);

    for (int i = 0; i < alloc; i++) {
        memkind_free(kind, ptr[i]);
    }

    ptr[0] = memkind_malloc(kind, 50 * KB);
    hint = memkind_default_get_defrag_hint(ptr[0], &bin, &run);
    ASSERT_EQ(hint, 0);
    memkind_free(kind, ptr[0]);

    memkind_destroy_kind(kind);
}

TEST_F(DefragTest, test_TC_hintTrue)
{
    struct memkind *kind = NULL;

    int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
    if (err != 0)
        printf("err = %d\n", err);

    int bin = -1;
    int run = -1;
    int hint = -1;
    int alloc = 50000;
    void* ptr[alloc];

    for (int i = 0; i < alloc; i++) {
        ptr[i] = memkind_malloc(kind, 128);
    }

    hint = memkind_default_get_defrag_hint(ptr[alloc / 2], &bin, &run);
    ASSERT_EQ(hint, 1);

    for (int i = 0; i < alloc; i++) {
        memkind_free(kind, ptr[i]);
    }

    memkind_destroy_kind(kind);
}

TEST_F(DefragTest, test_TC_redisStartDefragFalse)
{
    struct memkind *kind = NULL;

    int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
    if (err != 0)
        printf("err = %d\n", err);

    int bin = -1;
    int run = -1;
    int hint = -1;
    int alloc = 50000;
    void* ptr[alloc];
    bool startDefrag;

    for (int i = 0; i < alloc; i++) {
        ptr[i] = memkind_malloc(kind, 1 * KB);
    }

    hint = memkind_default_get_defrag_hint(ptr[alloc / 2], &bin, &run);
    ASSERT_EQ(hint, 1);
    // condition from Redis
    if (run > bin || run == 1 << 16) {
        startDefrag = false;
    }
    else {
        startDefrag = true;
    }
    ASSERT_FALSE(startDefrag);

    for (int i = 0; i < alloc; i++) {
        memkind_free(kind, ptr[i]);
    }

    memkind_destroy_kind(kind);
}

TEST_F(DefragTest, test_TC_redisStartDefragTrue)
{
    struct memkind *kind = NULL;

    int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
    if (err != 0)
        printf("err = %d\n", err);

    int bin = -1;
    int run = -1;
    int hint = -1;
    int alloc = 50000;
    void* ptr[alloc];
    bool startDefrag;

    for (int i = 0; i < alloc; i++) {
        ptr[i] = memkind_malloc(kind, 1 * KB);
    }

    for (int i = 1; i < alloc;) {
        memkind_free(kind, ptr[i]);
        ptr[i] = NULL;
        // Free memory in irregular pattern
        if (i % 2 == 0)
            i += 3;
        else
            i += 5;
    }

    hint = memkind_default_get_defrag_hint(ptr[0], &bin, &run);
    ASSERT_EQ(hint, 1);
    // Condition from Redis
    if (run > bin || run == 1 << 16) {
        startDefrag = false;
    }
    else {
        startDefrag = true;
    }
    ASSERT_TRUE(startDefrag);

    for (int i = 0; i < alloc; i++) {
        memkind_free(kind, ptr[i]);
    }

    memkind_destroy_kind(kind);
}

