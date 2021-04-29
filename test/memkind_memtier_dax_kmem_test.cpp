// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind_memtier.h>

#include "TestPrereq.hpp"
#include "common.h"

class MemkindMemtierDaxKmemTest: public ::testing::Test,
                                 public ::testing::WithParamInterface<memkind_t>
{
protected:
    TestPrereq tp;
    memkind_t memory_kind;
    struct memtier_builder *m_builder;
    struct memtier_memory *m_tier_memory;

    void SetUp()
    {
        if (!tp.is_DAX_KMEM_supported()) {
            GTEST_SKIP() << "DAX KMEM is required." << std::endl;
        }

        memory_kind = GetParam();
        m_tier_memory = nullptr;
        m_builder = memtier_builder_new(MEMTIER_POLICY_STATIC_THRESHOLD);
        ASSERT_NE(nullptr, m_builder);
    }

    void TearDown()
    {
        memtier_builder_delete(m_builder);
        if (m_tier_memory) {
            memtier_delete_memtier_memory(m_tier_memory);
        }
    }
};

INSTANTIATE_TEST_CASE_P(KindParam, MemkindMemtierDaxKmemTest,
                        ::testing::Values(MEMKIND_DAX_KMEM,
                                          MEMKIND_DAX_KMEM_ALL,
                                          MEMKIND_DAX_KMEM_PREFERRED,
                                          MEMKIND_DAX_KMEM_INTERLEAVE));

TEST_P(MemkindMemtierDaxKmemTest,
       test_MEMKIND_MEMTIER_DAX_KMEM_single_kind_allocations)
{
    const size_t size = 512;

    void *ptr = memtier_kind_malloc(memory_kind, size);
    ASSERT_NE(nullptr, ptr);
    ASSERT_EQ(memory_kind, memkind_detect_kind(ptr));
    memtier_free(ptr);
    ptr = memtier_kind_calloc(memory_kind, size, size);
    ASSERT_NE(nullptr, ptr);
    ASSERT_EQ(memory_kind, memkind_detect_kind(ptr));
    void *new_ptr = memtier_kind_realloc(memory_kind, ptr, size);
    ASSERT_NE(nullptr, new_ptr);
    ASSERT_EQ(memory_kind, memkind_detect_kind(ptr));
    memtier_free(new_ptr);
    int err = memtier_kind_posix_memalign(memory_kind, &ptr, 64, 32);
    ASSERT_EQ(0, err);
    ASSERT_EQ(memory_kind, memkind_detect_kind(ptr));
    memtier_free(ptr);
}
