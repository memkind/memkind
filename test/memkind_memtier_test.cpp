// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind_memtier.h>

#include <random>
#include <thread>

#include "common.h"

class MemkindMemtierTest: public ::testing::Test
{
protected:
    struct memtier_builder *m_builder;
    struct memtier_memory *m_tier_memory;

private:
    void SetUp()
    {
        m_tier_memory = NULL;
        m_builder = memtier_builder_new();
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

class MemkindMemtierMemoryTest: public ::testing::Test
{
protected:
    struct memtier_memory *m_tier_memory;

    const int MEMKIND_DEFAULT_ratio = 1;
    const int MEMKIND_REGULAR_ratio = 4;

    const float tier_regular_normalized_ratio =
        MEMKIND_REGULAR_ratio / MEMKIND_DEFAULT_ratio;

    size_t allocation_sum()
    {
        return memtier_kind_allocated_size(MEMKIND_DEFAULT) +
            memtier_kind_allocated_size(MEMKIND_REGULAR);
    }

    void SetUp()
    {
        struct memtier_builder *builder = memtier_builder_new();
        ASSERT_NE(nullptr, builder);

        int res = memtier_builder_add_tier(builder, MEMKIND_DEFAULT,
                                           MEMKIND_DEFAULT_ratio);
        ASSERT_EQ(0, res);
        res = memtier_builder_add_tier(builder, MEMKIND_REGULAR,
                                       MEMKIND_REGULAR_ratio);
        ASSERT_EQ(0, res);
        res = memtier_builder_set_policy(builder,
                                         MEMTIER_POLICY_STATIC_THRESHOLD);
        ASSERT_EQ(0, res);
        m_tier_memory = memtier_builder_construct_memtier_memory(builder);
        ASSERT_NE(nullptr, m_tier_memory);
        memtier_builder_delete(builder);
    }

    void TearDown()
    {
        memtier_delete_memtier_memory(m_tier_memory);
    }
};

class MemkindMemtier3KindsTest: public ::testing::Test
{
protected:
    struct memtier_memory *m_tier_memory;

    const int MEMKIND_DEFAULT_ratio = 3;
    const int MEMKIND_REGULAR_ratio = 1;
    const int MEMKIND_HIGHEST_CAPACITY_ratio = 5;

    const float tier_regular_normalized_ratio =
        MEMKIND_REGULAR_ratio / MEMKIND_DEFAULT_ratio;
    const float tier_hi_cap_normalized_ratio =
        MEMKIND_HIGHEST_CAPACITY_ratio / MEMKIND_DEFAULT_ratio;

    size_t allocation_sum()
    {
        return memtier_kind_allocated_size(MEMKIND_DEFAULT) +
            memtier_kind_allocated_size(MEMKIND_REGULAR) +
            memtier_kind_allocated_size(MEMKIND_HIGHEST_CAPACITY);
    }

    void SetUp()
    {
        struct memtier_builder *builder = memtier_builder_new();
        ASSERT_NE(nullptr, builder);

        int res = memtier_builder_add_tier(builder, MEMKIND_DEFAULT,
                                           MEMKIND_DEFAULT_ratio);
        ASSERT_EQ(0, res);
        res = memtier_builder_add_tier(builder, MEMKIND_REGULAR,
                                       MEMKIND_REGULAR_ratio);
        ASSERT_EQ(0, res);
        res = memtier_builder_add_tier(builder, MEMKIND_HIGHEST_CAPACITY,
                                       MEMKIND_HIGHEST_CAPACITY_ratio);
        ASSERT_EQ(0, res);
        res = memtier_builder_set_policy(builder,
                                         MEMTIER_POLICY_STATIC_THRESHOLD);
        ASSERT_EQ(0, res);
        m_tier_memory = memtier_builder_construct_memtier_memory(builder);
        ASSERT_NE(nullptr, m_tier_memory);
        memtier_builder_delete(builder);
    }

    void TearDown()
    {
        memtier_delete_memtier_memory(m_tier_memory);
    }
};

class MemkindMemtierThresholdTest: public ::testing::Test
{
protected:
    struct memtier_memory *m_tier_memory;

    const int MEMKIND_DEFAULT_ratio = 3;
    const int MEMKIND_REGULAR_ratio = 1;

    const float m_tier_regular_normalized_ratio =
        (float)MEMKIND_REGULAR_ratio / MEMKIND_DEFAULT_ratio;

    size_t allocation_sum()
    {
        return memtier_kind_allocated_size(MEMKIND_DEFAULT) +
            memtier_kind_allocated_size(MEMKIND_REGULAR);
    }

    void SetUp()
    {
        struct memtier_builder *builder = memtier_builder_new();
        ASSERT_NE(nullptr, builder);

        int res = memtier_builder_add_tier(builder, MEMKIND_DEFAULT,
                                           MEMKIND_DEFAULT_ratio);
        ASSERT_EQ(0, res);
        res = memtier_builder_add_tier(builder, MEMKIND_REGULAR,
                                       MEMKIND_REGULAR_ratio);
        ASSERT_EQ(0, res);
        res = memtier_builder_set_policy(builder,
                                         MEMTIER_POLICY_DYNAMIC_THRESHOLD);
        ASSERT_EQ(0, res);
        m_tier_memory = memtier_builder_construct_memtier_memory(builder);
        ASSERT_NE(nullptr, m_tier_memory);
        memtier_builder_delete(builder);
    }

    void TearDown()
    {
        memtier_delete_memtier_memory(m_tier_memory);
    }
};

TEST_F(MemkindMemtierTest, test_tier_size_after_destroy)
{
    const size_t size = 512;
    struct memkind *kind = nullptr;
    int res = memkind_create_pmem("/tmp/", 0, &kind);
    ASSERT_EQ(0, res);
    memtier_kind_malloc(kind, size);
    ASSERT_EQ(size, memtier_kind_allocated_size(kind));
    memkind_destroy_kind(kind);
    ASSERT_EQ(0U, memtier_kind_allocated_size(kind));
}

TEST_F(MemkindMemtierTest, test_tier_allocate)
{
    const size_t size = 512;
    void *ptr = memtier_kind_malloc(MEMKIND_DEFAULT, size);
    ASSERT_NE(nullptr, ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    memtier_free(ptr);
    ptr = memtier_kind_calloc(MEMKIND_DEFAULT, size, size);
    ASSERT_NE(nullptr, ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    void *new_ptr = memtier_kind_realloc(MEMKIND_DEFAULT, ptr, size);
    ASSERT_NE(nullptr, new_ptr);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    memtier_free(new_ptr);
    int err = memtier_kind_posix_memalign(MEMKIND_DEFAULT, &ptr, 64, 32);
    ASSERT_EQ(0, err);
    ASSERT_EQ(MEMKIND_DEFAULT, memkind_detect_kind(ptr));
    memtier_free(ptr);
}

TEST_F(MemkindMemtierTest, test_tier_builder_failure)
{
    int res = memtier_builder_add_tier(m_builder, nullptr, 1);
    ASSERT_NE(0, res);
}

TEST_F(MemkindMemtierTest, test_tier_construct_failure_zero_tiers)
{
    int res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_STATIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_EQ(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest, test_tier_builder_set_policy_failure)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    memtier_policy_t fake_policy = static_cast<memtier_policy_t>(-1);
    res = memtier_builder_set_policy(m_builder, fake_policy);
    ASSERT_NE(0, res);
}

TEST_F(MemkindMemtierTest, test_tier_free_nullptr)
{
    for (int i = 0; i < 512; i++) {
        memtier_free(nullptr);
    }
}

TEST_F(MemkindMemtierTest, test_tier_policy_dynamic_threshold_two_kinds)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 1);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_NE(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest, test_tier_policy_dynamic_threshold_three_kinds)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_HIGHEST_CAPACITY, 3);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_NE(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest, test_tier_policy_dynamic_threshold_failure_one_tier)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_EQ(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest, test_tier_policy_dynamic_threshold_ctl_set)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    size_t val = 64;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].val", &val);
    ASSERT_EQ(0, res);
    val = 16;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].min", &val);
    ASSERT_EQ(0, res);
    val = 95;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].max", &val);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_HIGHEST_CAPACITY, 3);
    ASSERT_EQ(0, res);
    val = 512;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[1].val", &val);
    ASSERT_EQ(0, res);
    val = 96;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[1].min", &val);
    ASSERT_EQ(0, res);
    val = 4096;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[1].max", &val);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_NE(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest, test_tier_policy_dynamic_threshold_ctl_set_only_val)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    size_t val = 1024;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].val", &val);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_NE(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest, test_tier_policy_dynamic_threshold_ctl_set_internals)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    unsigned int check_cnt = 5;
    res = memtier_ctl_set(m_builder, "policy.dynamic_threshold.check_cnt",
                          &check_cnt);
    ASSERT_EQ(0, res);
    float change = 0.25;
    res =
        memtier_ctl_set(m_builder, "policy.dynamic_threshold.change", &change);
    ASSERT_EQ(0, res);
    float trigger = 0.1;
    res = memtier_ctl_set(m_builder, "policy.dynamic_threshold.trigger",
                          &trigger);
    ASSERT_EQ(0, res);
    size_t step = 1024;
    res = memtier_ctl_set(m_builder, "policy.dynamic_threshold.step", &step);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_NE(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest,
       test_tier_policy_dynamic_threshold_ctl_failure_min_max)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    size_t val = 1000;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].min", &val);
    ASSERT_EQ(0, res);
    val = 1;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].max", &val);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_EQ(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest,
       test_tier_policy_dynamic_threshold_ctl_failure_min_val)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    size_t val = 1000;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].min", &val);
    ASSERT_EQ(0, res);
    val = 1;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].val", &val);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_EQ(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest,
       test_tier_policy_dynamic_threshold_ctl_failure_val_max)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    size_t val = 1000;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].val", &val);
    ASSERT_EQ(0, res);
    val = 1;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].max", &val);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_EQ(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest,
       test_tier_policy_dynamic_threshold_ctl_failure_max_min)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_HIGHEST_CAPACITY, 3);
    ASSERT_EQ(0, res);
    size_t val = 1000;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[0].max", &val);
    val = 1;
    res = memtier_ctl_set(m_builder,
                          "policy.dynamic_threshold.thresholds[1].min", &val);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_EQ(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest,
       test_tier_policy_dynamic_threshold_ctl_failure_negative_thres_change)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    float change = -1.5;
    res =
        memtier_ctl_set(m_builder, "policy.dynamic_threshold.change", &change);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_EQ(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierTest,
       test_tier_policy_dynamic_threshold_ctl_failure_negative_thres_trigger)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    float trigger = -1.5;
    res = memtier_ctl_set(m_builder, "policy.dynamic_threshold.trigger",
                          &trigger);
    ASSERT_EQ(0, res);
    res =
        memtier_builder_set_policy(m_builder, MEMTIER_POLICY_DYNAMIC_THRESHOLD);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_EQ(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierMemoryTest, test_tier_builder_allocation_test_success)
{
    const size_t size = 512;
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

TEST_F(MemkindMemtierMemoryTest, test_tier_check_size_empty_tier)
{
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));
}

TEST_F(MemkindMemtierMemoryTest, test_tier_memory_check_size_nullptr)
{
    const size_t test_size = 512;
    void *ptr = memtier_malloc(m_tier_memory, test_size);
    ASSERT_NE(ptr, nullptr);
    size_t ptr_usable_size = memtier_usable_size(ptr);

    ASSERT_EQ(ptr_usable_size, test_size);
    ASSERT_EQ(ptr_usable_size, allocation_sum());
    memtier_free(nullptr);
    ASSERT_EQ(ptr_usable_size, allocation_sum());

    void *no_ptr = memtier_malloc(m_tier_memory, SIZE_MAX);
    ASSERT_EQ(no_ptr, nullptr);
    ASSERT_EQ(ptr_usable_size, allocation_sum());

    void *memkind_ptr = memkind_malloc(MEMKIND_DEFAULT, 1024);
    ASSERT_NE(memkind_ptr, nullptr);
    ASSERT_EQ(ptr_usable_size, allocation_sum());

    memkind_free(MEMKIND_DEFAULT, memkind_ptr);
    ASSERT_EQ(ptr_usable_size, allocation_sum());

    memtier_free(ptr);
    ASSERT_EQ(0ULL, allocation_sum());
}

TEST_F(MemkindMemtierMemoryTest, test_tier_check_size_malloc)
{
    unsigned i;
    const size_t size_default = 1024;
    const size_t size_regular = 4096;
    const size_t alloc_no = 512;
    size_t alloc_counter = 0;
    size_t def_counter = 0;
    size_t reg_counter = 0;
    std::vector<void *> kind_vec;
    std::vector<void *> def_vec;
    std::vector<void *> reg_vec;
    const size_t alloc_sum = (size_default + size_regular) * alloc_no;

    for (i = 0; i < alloc_no; ++i) {
        void *ptr = memtier_malloc(m_tier_memory, size_default);
        ASSERT_NE(ptr, nullptr);
        kind_vec.push_back(ptr);
        alloc_counter += memtier_usable_size(ptr);

        ptr = memtier_malloc(m_tier_memory, size_regular);
        ASSERT_NE(ptr, nullptr);
        kind_vec.push_back(ptr);
        alloc_counter += memtier_usable_size(ptr);
    }

    ASSERT_EQ(alloc_counter, alloc_sum);
    ASSERT_EQ(alloc_counter, allocation_sum());

    for (auto const &ptr : kind_vec) {
        memtier_free(ptr);
    }

    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (i = 0; i < alloc_no; ++i) {
        void *ptr = memtier_kind_malloc(MEMKIND_DEFAULT, size_default);
        ASSERT_NE(ptr, nullptr);
        def_vec.push_back(ptr);
        def_counter += memtier_usable_size(ptr);

        ptr = memtier_kind_malloc(MEMKIND_REGULAR, size_regular);
        ASSERT_NE(ptr, nullptr);
        reg_vec.push_back(ptr);
        reg_counter += memtier_usable_size(ptr);
    }

    ASSERT_EQ(reg_counter, size_regular * alloc_no);
    ASSERT_EQ(def_counter, size_default * alloc_no);
    ASSERT_EQ(def_counter, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(reg_counter, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (auto const &ptr : def_vec) {
        memtier_free(ptr);
    }
    for (auto const &ptr : reg_vec) {
        memtier_free(ptr);
    }
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));
}

TEST_F(MemkindMemtierMemoryTest, test_tier_check_size_calloc)
{
    unsigned i;
    const size_t size_default = 1024;
    const size_t num = 10;
    const size_t size_regular = 4096;
    const size_t alloc_no = 512;
    size_t alloc_counter = 0;
    size_t def_counter = 0;
    size_t reg_counter = 0;
    std::vector<void *> kind_vec;
    std::vector<void *> def_vec;
    std::vector<void *> reg_vec;
    const size_t alloc_sum = (size_default + size_regular) * alloc_no * num;

    for (i = 0; i < alloc_no; ++i) {
        void *ptr = memtier_calloc(m_tier_memory, num, size_default);
        ASSERT_NE(ptr, nullptr);
        kind_vec.push_back(ptr);
        alloc_counter += memtier_usable_size(ptr);

        ptr = memtier_calloc(m_tier_memory, num, size_regular);
        ASSERT_NE(ptr, nullptr);
        kind_vec.push_back(ptr);
        alloc_counter += memtier_usable_size(ptr);
    }

    ASSERT_EQ(alloc_counter, alloc_sum);
    ASSERT_EQ(alloc_counter, allocation_sum());

    for (auto const &ptr : kind_vec) {
        void *new_ptr = memtier_realloc(m_tier_memory, ptr, 0);
        ASSERT_EQ(new_ptr, nullptr);
    }

    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (i = 0; i < alloc_no; ++i) {
        void *ptr = memtier_kind_calloc(MEMKIND_DEFAULT, num, size_default);
        ASSERT_NE(ptr, nullptr);
        def_vec.push_back(ptr);
        def_counter += memtier_usable_size(ptr);

        ptr = memtier_kind_calloc(MEMKIND_REGULAR, num, size_regular);
        ASSERT_NE(ptr, nullptr);
        reg_vec.push_back(ptr);
        reg_counter += memtier_usable_size(ptr);
    }

    ASSERT_EQ(reg_counter, size_regular * alloc_no * num);
    ASSERT_EQ(def_counter, size_default * alloc_no * num);
    ASSERT_EQ(def_counter, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(reg_counter, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (auto const &ptr : def_vec) {
        void *new_ptr = memtier_kind_realloc(MEMKIND_DEFAULT, ptr, 0);
        ASSERT_EQ(new_ptr, nullptr);
    }

    for (auto const &ptr : reg_vec) {
        void *new_ptr = memtier_kind_realloc(MEMKIND_REGULAR, ptr, 0);
        ASSERT_EQ(new_ptr, nullptr);
    }

    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));
}

TEST_F(MemkindMemtierMemoryTest, test_tier_kind_check_size_realloc_kind)
{
    unsigned i;
    const size_t size_default = 1024;
    const size_t size_regular = 4096;
    const size_t new_less_size = 512;
    const size_t new_bigger_size = 5120;
    const size_t alloc_no = 512;
    size_t alloc_counter = 0;
    size_t def_counter = 0;
    size_t reg_counter = 0;
    std::vector<void *> kind_vec;
    std::vector<void *> def_vec;
    std::vector<void *> reg_vec;
    size_t alloc_sum = (size_default + size_regular) * alloc_no;
    size_t def_sum, reg_sum;

    for (i = 0; i < alloc_no; ++i) {
        void *ptr = memtier_realloc(m_tier_memory, nullptr, size_default);
        ASSERT_NE(ptr, nullptr);
        kind_vec.push_back(ptr);
        alloc_counter += memtier_usable_size(ptr);

        ptr = memtier_realloc(m_tier_memory, nullptr, size_regular);
        ASSERT_NE(ptr, nullptr);
        kind_vec.push_back(ptr);
        alloc_counter += memtier_usable_size(ptr);
    }

    ASSERT_EQ(alloc_counter, alloc_sum);
    ASSERT_EQ(alloc_counter, allocation_sum());

    for (i = 0; i < kind_vec.size(); ++i) {
        alloc_counter -= memtier_usable_size(kind_vec[i]);
        void *new_ptr =
            memtier_realloc(m_tier_memory, kind_vec[i], new_less_size);
        ASSERT_NE(new_ptr, nullptr);
        alloc_counter += memtier_usable_size(new_ptr);
        kind_vec[i] = new_ptr;
    }

    alloc_sum = new_less_size * 2 * alloc_no;
    ASSERT_EQ(alloc_counter, alloc_sum);
    ASSERT_EQ(alloc_counter, allocation_sum());

    for (i = 0; i < kind_vec.size(); ++i) {
        alloc_counter -= memtier_usable_size(kind_vec[i]);
        void *new_ptr =
            memtier_realloc(m_tier_memory, kind_vec[i], new_bigger_size);
        ASSERT_NE(new_ptr, nullptr);
        alloc_counter += memtier_usable_size(new_ptr);
        kind_vec[i] = new_ptr;
    }

    alloc_sum = new_bigger_size * 2 * alloc_no;
    ASSERT_EQ(alloc_counter, alloc_sum);
    ASSERT_EQ(alloc_counter, allocation_sum());

    for (auto const &ptr : kind_vec) {
        memtier_free(ptr);
    }

    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (i = 0; i < alloc_no; ++i) {
        void *ptr =
            memtier_kind_realloc(MEMKIND_DEFAULT, nullptr, size_default);
        ASSERT_NE(ptr, nullptr);
        def_vec.push_back(ptr);
        def_counter += memtier_usable_size(ptr);

        ptr = memtier_kind_realloc(MEMKIND_REGULAR, nullptr, size_regular);
        ASSERT_NE(ptr, nullptr);
        reg_vec.push_back(ptr);
        reg_counter += memtier_usable_size(ptr);
    }

    def_sum = size_default * alloc_no;
    reg_sum = size_regular * alloc_no;
    ASSERT_EQ(def_counter, def_sum);
    ASSERT_EQ(reg_counter, reg_sum);
    ASSERT_EQ(def_counter, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(reg_counter, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (i = 0; i < def_vec.size(); ++i) {
        def_counter -= memtier_usable_size(def_vec[i]);
        void *new_ptr =
            memtier_kind_realloc(MEMKIND_DEFAULT, def_vec[i], new_less_size);
        ASSERT_NE(new_ptr, nullptr);
        def_counter += memtier_usable_size(new_ptr);
        def_vec[i] = new_ptr;
    }

    for (i = 0; i < reg_vec.size(); ++i) {
        reg_counter -= memtier_usable_size(reg_vec[i]);
        void *new_ptr =
            memtier_kind_realloc(MEMKIND_REGULAR, reg_vec[i], new_bigger_size);
        ASSERT_NE(new_ptr, nullptr);
        reg_counter += memtier_usable_size(new_ptr);
        reg_vec[i] = new_ptr;
    }

    def_sum = new_less_size * alloc_no;
    reg_sum = new_bigger_size * alloc_no;
    ASSERT_EQ(def_counter, def_sum);
    ASSERT_EQ(reg_counter, reg_sum);
    ASSERT_EQ(def_counter, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(reg_counter, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (auto const &ptr : def_vec) {
        memtier_free(ptr);
    }
    for (auto const &ptr : reg_vec) {
        memtier_free(ptr);
    }
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));
}

TEST_F(MemkindMemtierMemoryTest,
       test_tier_memory_check_size_posix_memalign_kind)
{
    unsigned i;
    const size_t size_default = 1024;
    const size_t alignment = 64;
    const size_t size_regular = 4096;
    const size_t alloc_no = 512;
    size_t alloc_counter = 0;
    size_t def_counter = 0;
    size_t reg_counter = 0;
    std::vector<void *> kind_vec;
    std::vector<void *> def_vec;
    std::vector<void *> reg_vec;
    void *ptr;
    const size_t alloc_sum = (size_default + size_regular) * alloc_no;

    for (i = 0; i < alloc_no; ++i) {
        int res = memtier_posix_memalign(m_tier_memory, &ptr, alignment,
                                         size_default);
        ASSERT_EQ(0, res);
        kind_vec.push_back(ptr);
        alloc_counter += memtier_usable_size(ptr);

        res = memtier_posix_memalign(m_tier_memory, &ptr, alignment,
                                     size_regular);
        ASSERT_EQ(0, res);
        kind_vec.push_back(ptr);
        alloc_counter += memtier_usable_size(ptr);
    }

    ASSERT_EQ(alloc_counter, alloc_sum);
    ASSERT_EQ(alloc_counter, allocation_sum());

    for (auto const &ptr : kind_vec) {
        memtier_free(ptr);
    }
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (i = 0; i < alloc_no; ++i) {
        int res = memtier_kind_posix_memalign(MEMKIND_DEFAULT, &ptr, alignment,
                                              size_default);
        ASSERT_EQ(0, res);
        def_vec.push_back(ptr);
        def_counter += memtier_usable_size(ptr);

        res = memtier_kind_posix_memalign(MEMKIND_REGULAR, &ptr, alignment,
                                          size_regular);
        ASSERT_EQ(0, res);
        reg_vec.push_back(ptr);
        reg_counter += memtier_usable_size(ptr);
    }

    ASSERT_EQ(def_counter, size_default * alloc_no);
    ASSERT_EQ(reg_counter, size_regular * alloc_no);
    ASSERT_EQ(def_counter, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(reg_counter, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (auto const &ptr : def_vec) {
        memtier_free(ptr);
    }
    for (auto const &ptr : reg_vec) {
        memtier_free(ptr);
    }

    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(0ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));
}

TEST_F(MemkindMemtierMemoryTest, test_tier_memory_thread_safety_calc_size)
{
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    const size_t num_threads = 1000;
    const size_t iteration_count = 5000;
    const size_t alloc_size = 16;
    const size_t max_size = alloc_size * num_threads * iteration_count;
    std::vector<std::thread> thds;
    std::vector<void *> alloc_vec;
    alloc_vec.reserve(num_threads * iteration_count);

    for (size_t i = 0; i < num_threads; ++i) {
        thds.push_back(std::thread([&]() {
            for (size_t j = 0; j < iteration_count; ++j) {
                void *ptr = memtier_malloc(m_tier_memory, alloc_size);
                pthread_mutex_lock(&mutex);
                alloc_vec.push_back(ptr);
                pthread_mutex_unlock(&mutex);
            }
        }));
    }
    for (size_t i = 0; i < num_threads; ++i) {
        thds[i].join();
    }

    thds.clear();

    ASSERT_EQ(max_size, allocation_sum());

    for (size_t i = 0; i < num_threads; ++i) {
        thds.push_back(std::thread([&]() {
            for (size_t j = 0; j < iteration_count; ++j) {
                pthread_mutex_lock(&mutex);
                void *ptr = alloc_vec.back();
                alloc_vec.pop_back();
                pthread_mutex_unlock(&mutex);
                memtier_free(ptr);
            }
        }));
    }

    for (size_t i = 0; i < num_threads; ++i) {
        thds[i].join();
    }

    ASSERT_EQ(0ULL, allocation_sum());
}

TEST_F(MemkindMemtierMemoryTest, test_ratio_basic)
{
    std::vector<void *> allocs;

    allocs.push_back(memtier_malloc(m_tier_memory, 16));
    allocs.push_back(memtier_malloc(m_tier_memory, 16));
    // actual ratio 1:1, desired 1:4
    ASSERT_EQ(16ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(16ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 16));
    // actual ratio 1:2, desired 1:4
    ASSERT_EQ(16ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(32ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 16));
    // actual ratio 1:3, desired 1:4
    ASSERT_EQ(16ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(48ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 8));
    // actual ratio 1:3.5, desired 1:4
    ASSERT_EQ(16ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(56ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 16));
    // actual ratio 1:4.5, desired 1:4
    ASSERT_EQ(16ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(72ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 16));
    // actual ratio 1:2.25, desired 1:4
    ASSERT_EQ(32ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(72ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 36));
    // actual ratio 1:3.75, desired 1:4
    ASSERT_EQ(32ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(120ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 8));
    // actual ratio 1:4, desired 1:4
    ASSERT_EQ(32ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(128ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 16));
    // actual ratio 1:2.6, desired 1:4
    ASSERT_EQ(48ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(128ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 128));
    // actual ratio 1:5.3, desired 1:4
    ASSERT_EQ(48ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(256ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 256));
    // actual ratio 1:0.842, desired 1:4
    ASSERT_EQ(304ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(256ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 512));
    // actual ratio 1:2.526, desired 1:4
    ASSERT_EQ(304ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(768ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 1024));
    // actual ratio 1:5.894, desired 1:4
    ASSERT_EQ(304ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(1792ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 160));
    // actual ratio 1:3.862, desired 1:4
    ASSERT_EQ(464ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(1792ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 256));
    // actual ratio 1:4.413, desired 1:4
    ASSERT_EQ(464ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(2048ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    allocs.push_back(memtier_malloc(m_tier_memory, 48));
    // actual ratio 1:4, desired 1:4
    ASSERT_EQ(512ULL, memtier_kind_allocated_size(MEMKIND_DEFAULT));
    ASSERT_EQ(2048ULL, memtier_kind_allocated_size(MEMKIND_REGULAR));

    for (auto const &ptr : allocs) {
        memtier_free(ptr);
    }
}

TEST_F(MemkindMemtierMemoryTest, test_ratio_malloc_only)
{
    const int sizes_num = 5;
    const size_t sizes[sizes_num] = {16, 32, 64, 128, 256};
    const unsigned num_allocs = 100;
    std::vector<void *> allocs;
    unsigned i;

    for (i = 0; i < num_allocs; ++i) {
        size_t regular_alloc_size =
            memtier_kind_allocated_size(MEMKIND_REGULAR);
        float tier_regular_actual_ratio = (float)regular_alloc_size /
            memtier_kind_allocated_size(MEMKIND_DEFAULT);

        size_t size = sizes[i % sizes_num];
        void *ptr = memtier_malloc(m_tier_memory, size);
        ASSERT_NE(nullptr, ptr);
        allocs.push_back(ptr);

        // if actual ratio between default and regular is lower than desired,
        // new allocation should go to regular tier
        if (tier_regular_actual_ratio < tier_regular_normalized_ratio) {
            ASSERT_GE(memtier_kind_allocated_size(MEMKIND_REGULAR),
                      regular_alloc_size);
        }
    }

    for (auto const &ptr : allocs) {
        memtier_free(ptr);
    }
}

TEST_F(MemkindMemtierMemoryTest, test_ratio_malloc_free)
{
    const int sizes_num = 5;
    const size_t sizes[sizes_num] = {16, 32, 64, 128, 256};

    // 1 = malloc, -1 = free
    // NOTE that there are the same num of mallocs/free so at the end all
    // allocations should be freed
    const int operations_num = 14;
    const int operations[operations_num] = {1, 1, -1, 1,  -1, -1, 1,
                                            1, 1, -1, -1, 1,  -1, -1};

    const unsigned num_allocs = operations_num * sizes_num;
    std::vector<void *> allocs;
    unsigned i;

    for (i = 0; i < num_allocs; ++i) {
        size_t regular_alloc_size =
            memtier_kind_allocated_size(MEMKIND_REGULAR);
        float tier_regular_actual_ratio = (float)regular_alloc_size /
            memtier_kind_allocated_size(MEMKIND_DEFAULT);

        size_t size = sizes[i % sizes_num];
        int op = operations[i % operations_num];
        if (op == 1) {
            void *ptr = memtier_malloc(m_tier_memory, size);
            ASSERT_NE(nullptr, ptr);
            allocs.push_back(ptr);
        } else {
            memtier_free(allocs.back());
            allocs.pop_back();
        }

        // if actual ratio between default and regular is lower than desired,
        // new allocation should go to regular tier
        if ((op == 1) &&
            (tier_regular_actual_ratio < tier_regular_normalized_ratio)) {
            ASSERT_GE(memtier_kind_allocated_size(MEMKIND_REGULAR),
                      regular_alloc_size);
        }
    }

    ASSERT_EQ(0ULL, allocs.size());
}

TEST_F(MemkindMemtier3KindsTest, test_ratio_malloc_only)
{
    const int sizes_num = 10;
    const size_t sizes[sizes_num] = {16, 32, 16, 64, 16, 128, 32, 256, 64, 128};
    const unsigned num_allocs = 200;
    std::vector<void *> allocs;
    unsigned i;

    for (i = 0; i < num_allocs; ++i) {
        size_t regular_alloc_size =
            memtier_kind_allocated_size(MEMKIND_REGULAR);
        float tier_regular_actual_ratio = (float)regular_alloc_size /
            memtier_kind_allocated_size(MEMKIND_DEFAULT);

        size_t hi_cap_alloc_size =
            memtier_kind_allocated_size(MEMKIND_HIGHEST_CAPACITY);
        float tier_hi_cap_actual_ratio = (float)hi_cap_alloc_size /
            memtier_kind_allocated_size(MEMKIND_DEFAULT);

        size_t size = sizes[i % sizes_num];
        void *ptr = memtier_malloc(m_tier_memory, size);
        ASSERT_NE(nullptr, ptr);
        allocs.push_back(ptr);

        // if actual ratio between default and regular is lower than desired,
        // new allocation should go to regular tier,
        // otherwise check if actual ratio between default and highest capacity
        // is lower than desired, new allocation should go to highest capacity
        // tier
        if (tier_regular_actual_ratio < tier_regular_normalized_ratio) {
            ASSERT_GE(memtier_kind_allocated_size(MEMKIND_REGULAR),
                      regular_alloc_size);
        } else if (tier_hi_cap_actual_ratio < tier_hi_cap_normalized_ratio) {
            ASSERT_GE(memtier_kind_allocated_size(MEMKIND_HIGHEST_CAPACITY),
                      hi_cap_alloc_size);
        }
    }

    for (auto const &ptr : allocs) {
        memtier_free(ptr);
    }
}

TEST_F(MemkindMemtier3KindsTest, test_ratio_malloc_free)
{
    const int sizes_num = 10;
    const size_t sizes[sizes_num] = {16, 32, 16, 64, 16, 128, 32, 256, 64, 128};

    // 1 = malloc, 2 = alloc from specified tier, -1 = free
    // NOTE that there are the same num of mallocs/free so at the end all
    // allocations should be freed
    const int operations_num = 22;
    const int operations[operations_num] = {1, 2,  1, -1, 1,  -1, -1, 1,
                                            2, -1, 1, 1,  -1, 2,  -1, -1,
                                            1, -1, 2, -1, -1, -1};

    const memkind_t tiers[] = {
        MEMKIND_DEFAULT,
        MEMKIND_REGULAR,
        MEMKIND_HIGHEST_CAPACITY,
    };
    int tier_it = 0;

    const unsigned num_allocs = operations_num * sizes_num * 4;
    std::vector<void *> allocs;
    unsigned i;

    for (i = 0; i < num_allocs; ++i) {
        size_t regular_alloc_size =
            memtier_kind_allocated_size(MEMKIND_REGULAR);
        float tier_regular_actual_ratio = (float)regular_alloc_size /
            memtier_kind_allocated_size(MEMKIND_DEFAULT);

        size_t hi_cap_alloc_size =
            memtier_kind_allocated_size(MEMKIND_HIGHEST_CAPACITY);
        float tier_hi_cap_actual_ratio = (float)hi_cap_alloc_size /
            memtier_kind_allocated_size(MEMKIND_DEFAULT);

        size_t size = sizes[i % sizes_num];
        int op = operations[i % operations_num];
        if (op == 1) {
            void *ptr = memtier_malloc(m_tier_memory, size);
            ASSERT_NE(nullptr, ptr);
            allocs.push_back(ptr);
        } else if (op == 2) {
            void *ptr = memtier_kind_malloc(tiers[tier_it++ % 3], size);
            ASSERT_NE(nullptr, ptr);
            allocs.push_back(ptr);
        } else {
            memtier_free(allocs.back());
            allocs.pop_back();
        }

        // for memtier_kind_mallocs, if actual ratio between default and
        // regular is lower than desired, new allocation should go to
        // regular tier - otherwise do the same check for highest capacity tier
        if (op == 1) {
            if (tier_regular_actual_ratio < tier_regular_normalized_ratio) {
                ASSERT_GE(memtier_kind_allocated_size(MEMKIND_REGULAR),
                          regular_alloc_size);
            } else if (tier_hi_cap_actual_ratio <
                       tier_hi_cap_normalized_ratio) {
                ASSERT_GE(memtier_kind_allocated_size(MEMKIND_HIGHEST_CAPACITY),
                          hi_cap_alloc_size);
            }
        }
    }

    ASSERT_EQ(0ULL, allocs.size());
}

TEST_F(MemkindMemtierThresholdTest, test_const_alloc_size)
{
    std::vector<void *> allocs;
    const unsigned num_allocs = 10000;
    const size_t alloc_size = 1024;

    // Start checking distance between actual and desired ratio
    // after "ratio_check_skip" allocations
    const unsigned ratio_check_skip = 1000;
    const float max_ratio_distance = 0.35; // 35%

    for (unsigned i = 0; i < num_allocs; ++i) {
        void *ptr = memtier_malloc(m_tier_memory, alloc_size);
        ASSERT_NE(nullptr, ptr);
        allocs.push_back(ptr);

        if (i < ratio_check_skip) {
            continue;
        }

        size_t default_alloc_size =
            memtier_kind_allocated_size(MEMKIND_DEFAULT);
        size_t regular_alloc_size =
            memtier_kind_allocated_size(MEMKIND_REGULAR);

        float actual_ratio = (float)regular_alloc_size / default_alloc_size;
        float dist = abs(m_tier_regular_normalized_ratio - actual_ratio) /
            m_tier_regular_normalized_ratio;
        ASSERT_LE(dist, max_ratio_distance);
    }

    for (auto const &ptr : allocs) {
        memtier_free(ptr);
    }
}

TEST_F(MemkindMemtierThresholdTest, test_various_alloc_size)
{
    size_t i;
    std::mt19937 gen{1}; // set seed = 1
    std::exponential_distribution<double> exd(3.5);
    const int alloc_sizes_num = 10;
    const size_t alloc_sizes[alloc_sizes_num] = {4,   8,   16,  32,   64,
                                                 128, 256, 512, 1024, 1024 * 2};
    const int alloc_num = 10000;
    size_t sizes[alloc_num];
    std::vector<void *> allocs;

    // initialize sizes array
    for (i = 0; i < alloc_num; ++i) {
        double g;
        while ((g = exd(gen)) >= 1.0)
            ;
        int alloc_size = alloc_sizes[int(g * alloc_sizes_num)];
        sizes[i] = alloc_size;
    }

    // do allocations
    for (i = 0; i < alloc_num; ++i) {
        void *ptr = memtier_malloc(m_tier_memory, sizes[i]);
        ASSERT_NE(nullptr, ptr);
        allocs.push_back(ptr);
    }

    // check if actual ratios between tiers are close to desired
    const float max_ratio_distance = 0.25; // 25%

    size_t default_alloc_size = memtier_kind_allocated_size(MEMKIND_DEFAULT);
    size_t regular_alloc_size = memtier_kind_allocated_size(MEMKIND_REGULAR);

    float reg_def_ratio = (float)regular_alloc_size / default_alloc_size;

    float def_reg_dist = abs(m_tier_regular_normalized_ratio - reg_def_ratio) /
        (m_tier_regular_normalized_ratio);
    ASSERT_LE(def_reg_dist, max_ratio_distance);

    for (auto const &ptr : allocs) {
        memtier_free(ptr);
    }
}
