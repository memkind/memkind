// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind_memtier.h>

#include "common.h"

class MemkindMemtierTest: public ::testing::Test
{
private:
    void SetUp()
    {}

    void TearDown()
    {}
};

class MemkindMemtierBuilderTest: public ::testing::Test
{
private:
    struct memtier_tier *m_tier_default;
    struct memtier_tier *m_tier_regular;

protected:
    struct memtier_kind *m_tier_kind;

    void SetUp()
    {
        m_tier_default = memtier_tier_new(MEMKIND_DEFAULT);
        m_tier_regular = memtier_tier_new(MEMKIND_REGULAR);
        struct memtier_builder *builder = memtier_builder();
        ASSERT_NE(nullptr, m_tier_default);
        ASSERT_NE(nullptr, m_tier_regular);
        ASSERT_NE(nullptr, builder);

        int res = memtier_builder_add_tier(builder, m_tier_default, 1);
        ASSERT_EQ(0, res);
        res = memtier_builder_add_tier(builder, m_tier_regular, 1000);
        ASSERT_EQ(0, res);
        res = memtier_builder_set_policy(builder, MEMTIER_DUMMY_VALUE);
        ASSERT_EQ(0, res);
        res = memtier_builder_construct_kind(builder, &m_tier_kind);
        ASSERT_EQ(0, res);
    }

    void TearDown()
    {
        memtier_delete_kind(m_tier_kind);
        memtier_tier_delete(m_tier_default);
        memtier_tier_delete(m_tier_regular);
    }
};

TEST_F(MemkindMemtierTest, test_tier_create_fatal)
{
    struct memtier_tier *tier = memtier_tier_new(nullptr);
    ASSERT_EQ(nullptr, tier);
}

TEST_F(MemkindMemtierTest, test_tier_create_success)
{
    struct memtier_tier *tier = memtier_tier_new(MEMKIND_DEFAULT);
    ASSERT_NE(nullptr, tier);
    memtier_tier_delete(tier);
}

TEST_F(MemkindMemtierTest, test_tier_create_duplicate_fatal)
{
    struct memtier_tier *tier = memtier_tier_new(MEMKIND_DEFAULT);
    ASSERT_NE(nullptr, tier);
    struct memtier_tier *tier2 = memtier_tier_new(MEMKIND_DEFAULT);
    ASSERT_EQ(nullptr, tier2);
    memtier_tier_delete(tier);
}

TEST_F(MemkindMemtierTest, test_tier_create_duplicate_success)
{
    struct memtier_tier *tier = memtier_tier_new(MEMKIND_DEFAULT);
    ASSERT_NE(nullptr, tier);
    memtier_tier_delete(tier);
    struct memtier_tier *tier2 = memtier_tier_new(MEMKIND_DEFAULT);
    ASSERT_NE(nullptr, tier2);
    memtier_tier_delete(tier2);
}

TEST_F(MemkindMemtierTest, test_tier_allocate)
{
    const size_t size = 512;
    struct memtier_tier *tier = memtier_tier_new(MEMKIND_DEFAULT);
    ASSERT_NE(nullptr, tier);
    void *ptr = memtier_tier_malloc(tier, size);
    ASSERT_NE(nullptr, ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    memtier_free(ptr);
    ptr = memtier_tier_calloc(tier, size, size);
    ASSERT_NE(nullptr, ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    void *new_ptr = memtier_tier_realloc(tier, ptr, size);
    ASSERT_NE(nullptr, new_ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    memtier_free(new_ptr);
    int err = memtier_tier_posix_memalign(tier, &ptr, 64, 32);
    ASSERT_EQ(0, err);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    memtier_free(ptr);
    memtier_tier_delete(tier);
}

TEST_F(MemkindMemtierTest, test_tier_builder_failure)
{
    struct memtier_builder *builder = memtier_builder();
    ASSERT_NE(nullptr, builder);
    int res = memtier_builder_add_tier(builder, NULL, 1);
    ASSERT_NE(0, res);
}

TEST_F(MemkindMemtierTest, test_tier_builder_set_policy_failure)
{
    struct memtier_tier *tier = memtier_tier_new(MEMKIND_DEFAULT);
    struct memtier_builder *builder = memtier_builder();
    ASSERT_NE(nullptr, tier);
    ASSERT_NE(nullptr, builder);
    int res = memtier_builder_add_tier(builder, tier, 1);
    ASSERT_EQ(0, res);
    memtier_policy_t fake_policy = static_cast<memtier_policy_t>(-1);
    res = memtier_builder_set_policy(builder, fake_policy);
    ASSERT_NE(0, res);
    memtier_tier_delete(tier);
}

TEST_F(MemkindMemtierTest, test_tier_free_nullptr)
{
    for (int i = 0; i < 512; i++) {
        memtier_free(nullptr);
    }
}

TEST_F(MemkindMemtierBuilderTest, test_tier_builder_allocation_test_success)
{
    const size_t size = 512;
    void *ptr = memtier_kind_malloc(m_tier_kind, size);
    ASSERT_NE(nullptr, ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    memtier_free(ptr);
    ptr = memtier_kind_calloc(m_tier_kind, size, size);
    ASSERT_NE(nullptr, ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    void *new_ptr = memtier_kind_realloc(m_tier_kind, ptr, size);
    ASSERT_NE(nullptr, new_ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    memtier_free(new_ptr);
    int err = memtier_kind_posix_memalign(m_tier_kind, &ptr, 64, 32);
    ASSERT_EQ(0, err);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    memtier_free(ptr);
}
