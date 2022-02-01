// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021-2022 Intel Corporation. */

#include <memkind/internal/memkind_memtier.h>
#include <memkind/internal/slab_allocator.h>
#include <mtt_allocator.h>

#include <gtest/gtest.h>
#include <test/proc_stat.h>

class MemkindMemtierDataMovementTest: public ::testing::Test
{
protected:
    struct memtier_builder *m_builder;
    struct memtier_memory *m_tier_memory;

private:
    void SetUp()
    {
        m_tier_memory = nullptr;
        m_builder = memtier_builder_new(MEMTIER_POLICY_DATA_MOVEMENT);
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

TEST_F(MemkindMemtierDataMovementTest, test_tier_policy_data_movement_two_kinds)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 1);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_NE(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierDataMovementTest,
       test_tier_policy_data_movement_three_kinds)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_REGULAR, 2);
    ASSERT_EQ(0, res);
    res = memtier_builder_add_tier(m_builder, MEMKIND_HIGHEST_CAPACITY, 3);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_NE(nullptr, m_tier_memory);
}

TEST_F(MemkindMemtierDataMovementTest,
       test_tier_policy_data_movement_failure_one_tier)
{
    int res = memtier_builder_add_tier(m_builder, MEMKIND_DEFAULT, 1);
    ASSERT_EQ(0, res);
    m_tier_memory = memtier_builder_construct_memtier_memory(m_builder);
    ASSERT_EQ(nullptr, m_tier_memory);
}

#define struct_bar(size)                                                       \
    typedef struct bar##size {                                                 \
        char boo[(size)];                                                      \
    } bar##size

struct_bar(7);

// TODO refactor this test to use gtest functionality instead of macros

#define test_slab_alloc(size, nof_elements)                                    \
    do {                                                                       \
        struct_bar(size);                                                      \
        slab_alloc_t temp;                                                     \
        int ret = slab_alloc_init(&temp, size, nof_elements);                  \
        ASSERT_TRUE(ret == 0 && "mutex creation failed!");                     \
        slab_alloc_destroy(&temp);                                             \
        ret = slab_alloc_init(&temp, size, nof_elements);                      \
        ASSERT_TRUE(ret == 0 && "mutex creation failed!");                     \
        bar##size *elements[nof_elements];                                     \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            elements[i] = (bar##size *)slab_alloc_malloc(&temp);               \
            ASSERT_TRUE(elements[i] && "slab returned NULL!");                 \
            memset(elements[i], i, size);                                      \
        }                                                                      \
        for (int i = 0; i < nof_elements; ++i) {                               \
            for (size_t j = 0; j < size; j++)                                  \
                ASSERT_TRUE(elements[i]->boo[j] ==                             \
                            (char)((unsigned)(i)) % 255);                      \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        for (int i = 0; i < nof_elements; ++i) {                               \
            slab_alloc_free(elements[i]);                                      \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            elements[i] = (bar##size *)slab_alloc_malloc(&temp);               \
            ASSERT_TRUE(elements[i] && "slab returned NULL!");                 \
            memset(elements[i], i + 15, size);                                 \
        }                                                                      \
        for (int i = 0; i < nof_elements; ++i) {                               \
            for (size_t j = 0; j < size; j++)                                  \
                ASSERT_TRUE(elements[i]->boo[j] ==                             \
                            (char)((unsigned)(i + 15)) % 255);                 \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        for (int i = 0; i < nof_elements; ++i) {                               \
            slab_alloc_free(elements[i]);                                      \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        slab_alloc_destroy(&temp);                                             \
    } while (0)

#define struct_bar_align(size)                                                 \
    typedef struct bar_align##size {                                           \
        uint64_t boo[(size)];                                                  \
    } bar_align##size

struct_bar_align(7);

#define test_slab_alloc_alignment(size, nof_elements)                          \
    do {                                                                       \
        struct_bar_align(size);                                                \
        size_t bar_align_size = sizeof(bar_align##size);                       \
        slab_alloc_t temp;                                                     \
        int ret = slab_alloc_init(&temp, bar_align_size, nof_elements);        \
        ASSERT_TRUE(ret == 0 && "mutex creation failed!");                     \
        bar_align##size *elements[nof_elements];                               \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            elements[i] = (bar_align##size *)slab_alloc_malloc(&temp);         \
            ASSERT_TRUE(elements[i] && "slab returned NULL!");                 \
            for (size_t j = 0; j < size; ++j)                                  \
                elements[i]->boo[j] = i * nof_elements + j;                    \
        }                                                                      \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            for (size_t j = 0; j < size; j++)                                  \
                ASSERT_TRUE(elements[i]->boo[j] == i * nof_elements + j);      \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            slab_alloc_free(elements[i]);                                      \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            elements[i] = (bar_align##size *)slab_alloc_malloc(&temp);         \
            ASSERT_TRUE(elements[i] && "slab returned NULL!");                 \
            for (size_t j = 0; j < size; ++j)                                  \
                elements[i]->boo[j] = 7 * i * nof_elements + j + 5;            \
        }                                                                      \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            for (size_t j = 0; j < size; j++)                                  \
                ASSERT_TRUE(elements[i]->boo[j] ==                             \
                            7 * i * nof_elements + j + 5);                     \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            slab_alloc_free(elements[i]);                                      \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        slab_alloc_destroy(&temp);                                             \
    } while (0)

static void test_slab_alloc_static3(void)
{
    struct_bar(3);
    size_t NOF_ELEMENTS = 1024;
    size_t SIZE = 3;
    slab_alloc_t temp;
    int ret = slab_alloc_init(&temp, SIZE, NOF_ELEMENTS);
    ASSERT_TRUE(ret == 0 && "slab alloc init failed!");
    slab_alloc_destroy(&temp);
    ret = slab_alloc_init(&temp, SIZE, NOF_ELEMENTS);
    ASSERT_TRUE(ret == 0 && "slab alloc init failed!");
    bar3 *elements[NOF_ELEMENTS];
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        elements[i] = (bar3 *)slab_alloc_malloc(&temp);
        ASSERT_TRUE(elements[i] && "slab returned NULL!");
        memset(elements[i], i, SIZE);
    }
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        for (size_t j = 0; j < SIZE; j++)
            ASSERT_TRUE(elements[i]->boo[j] == (char)((unsigned)(i)) % 255);
    }
    ASSERT_TRUE(temp.used == NOF_ELEMENTS);
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        slab_alloc_free(elements[i]);
    }
    ASSERT_TRUE(temp.used == NOF_ELEMENTS);
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        elements[i] = (bar3 *)slab_alloc_malloc(&temp);
        ASSERT_TRUE(elements[i] && "slab returned NULL!");
        memset(elements[i], i + 15, SIZE);
    }
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        for (size_t j = 0; j < SIZE; j++)
            ASSERT_TRUE(elements[i]->boo[j] ==
                        (char)((unsigned)(i + 15)) % 255);
    }
    ASSERT_TRUE(temp.used == NOF_ELEMENTS);
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        slab_alloc_free(elements[i]);
    }
    ASSERT_TRUE(temp.used == NOF_ELEMENTS);
    slab_alloc_destroy(&temp);
}

TEST(SlabAlloc, Basic)
{
    test_slab_alloc_static3();
    test_slab_alloc(1, 1000000);
    test_slab_alloc(2, 1002300);
    test_slab_alloc(4, 798341);
    test_slab_alloc(8, 714962);
    test_slab_alloc(8, 1000000);
    test_slab_alloc(7, 942883);
    test_slab_alloc(17, 71962);
    test_slab_alloc(58, 214662);
}

TEST(SlabAlloc, Alignment)
{
    test_slab_alloc_alignment(1, 100000);
    test_slab_alloc_alignment(1, 213299);
    test_slab_alloc_alignment(2, 912348);
    test_slab_alloc_alignment(4, 821429);
    test_slab_alloc_alignment(8, 814322);
    test_slab_alloc_alignment(7, 291146);
    test_slab_alloc_alignment(7, 291);
}

// TODO add a test for SLAB_ALLOC where we handle allocations of total
// size bigger than init mapping

TEST(DataMovementBgThreadTest, test_bg_thread_lifecycle)
{
    ProcStat proc_stat;
    MTTAllocator mtt_allocator;

    unsigned threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(threads_count, 1U);

    mtt_allocator_create(&mtt_allocator);

    threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(threads_count, 2U);

    int ret = mtt_allocator_destroy(&mtt_allocator);
    ASSERT_EQ(ret, 0);

    threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(threads_count, 1U);
}
