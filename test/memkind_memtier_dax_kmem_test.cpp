// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind_memtier.h>

#include "TestPrereq.hpp"
#include "common.h"

class MemkindMemtierDaxKmemTest: public ::testing::Test
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

        memory_kind = MEMKIND_DAX_KMEM;
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

TEST_F(MemkindMemtierDaxKmemTest, test_single_kind_allocations)
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

TEST_F(MemkindMemtierDaxKmemTest, test_tier_builder_allocation_test_success)
{
    const size_t size = 512;

    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, memory_kind, 1);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_NE(nullptr, m_tier_memory);

    void *ptr = memtier_malloc(m_tier_memory, size);
    ASSERT_NE(nullptr, ptr);
    memtier_free(ptr);
    ptr = memtier_calloc(m_tier_memory, size, size);
    ASSERT_NE(nullptr, ptr);
    memkind_t kind = memkind_detect_kind(ptr);
    void *new_ptr = memtier_realloc(m_tier_memory, ptr, size);
    ASSERT_NE(nullptr, new_ptr);
    ASSERT_EQ(kind, memkind_detect_kind(ptr));
    memtier_free(new_ptr);
    int err = memtier_posix_memalign(m_tier_memory, &ptr, 64, 32);
    ASSERT_EQ(0, err);
    memtier_free(ptr);
}
