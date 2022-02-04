// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021-2022 Intel Corporation. */

#include <memkind/internal/hasher.h>
#include <memkind/internal/memkind_memtier.h>
#include <memkind/internal/pebs.h>
#include <memkind/internal/pool_allocator.h>
#include <memkind/internal/slab_allocator.h>
#include <mtt_allocator.h>

#include <gtest/gtest.h>
#include <mutex>
#include <set>

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

static uint16_t calculate_hash_with_stack_variation(size_t size_rank,
                                                    size_t added_stack)
{
    // prevent optimisations
    static std::mutex mut;
    std::unique_lock<std::mutex> lock(mut);
    volatile uint8_t buffer[added_stack];
    for (size_t i = 0; i < added_stack; ++i)
        buffer[i] = 0;
    (void)buffer;
    lock.unlock();
    return hasher_calculate_hash(size_rank);
}

TEST(Hasher, Basic)
{
#define SIZE_RANK(hash)       ((hash)&0b1111111)
#define LSB_9(val)            ((val)&0b111111111)
#define STACK_SIZE_HASH(hash) ((uint16_t)(LSB_9((hash) >> 7)))

    ASSERT_EQ(SIZE_RANK(hasher_calculate_hash(0)), 0);
    ASSERT_EQ(SIZE_RANK(hasher_calculate_hash(16)), 16);
    ASSERT_EQ(SIZE_RANK(hasher_calculate_hash(127)), 127);

    const size_t MIN_DIFF = 128u; // multiple of a cacheline
    std::vector<std::pair<size_t, size_t>> vals = {
        {127, 0},
        {127, MIN_DIFF},
        {127, MIN_DIFF * 2},
        {127, MIN_DIFF * 3},
        {127, MIN_DIFF * 2},
        {127, 0},
        {28, 0},
        {28, MIN_DIFF},
        {128, 0},
    };
    // hash, expected offset
    std::vector<std::pair<uint16_t, uint16_t>> hashes;
    hashes.reserve(vals.size());
    for (auto &val : vals) {
        auto size_rank = val.first;
        auto expected_offset = val.second;
        uint16_t hash_temp =
            calculate_hash_with_stack_variation(size_rank, expected_offset);
        hashes.push_back(std::make_pair((uint16_t)hash_temp, expected_offset));
    }
    for (auto &thash : hashes) {
        auto hash = thash.first;
        auto expected_offset = thash.second;
        ASSERT_EQ(LSB_9(STACK_SIZE_HASH(hash) + expected_offset),
                  STACK_SIZE_HASH(hashes[0].first));
    }
}

class PoolAllocTest: public ::testing::Test
{
protected:
    static void check_add(std::map<slab_alloc_t *, size_t> &slabs,
                          PoolAllocator &pool, size_t expected_size)
    {
        for (volatile size_t i = 0; i < UINT16_MAX; ++i) {
            volatile slab_alloc_t *slab = pool.pool[i];
            if (slab) {
                auto temp = slabs.find((slab_alloc_t *)slab);
                volatile bool found = temp != slabs.end();
                if (!found || (temp->first->used != temp->second)) {
                    ASSERT_EQ(slab->elementSize, expected_size);
                    slabs[(slab_alloc_t *)slab] = slab->used;
                    return;
                }
            }
        }
        ASSERT_TRUE(false && "element was not found");
    }
};

TEST_F(PoolAllocTest, Basic)
{
    PoolAllocator pool;
    int ret = pool_allocator_create(&pool);
    ASSERT_EQ(ret, 0);
    for (size_t i = 0; i < UINT16_MAX; ++i)
        ASSERT_EQ(pool.pool[i], nullptr);

    std::map<slab_alloc_t *, size_t> created_slabs;

    uint8_t *a16 = (uint8_t *)pool_allocator_malloc(&pool, 16);
    check_add(created_slabs, pool, 16 + sizeof(freelist_node_meta_t));

    uint8_t *a8 = (uint8_t *)pool_allocator_malloc(&pool, 8);
    check_add(created_slabs, pool, 16 + sizeof(freelist_node_meta_t));

    uint8_t *a17 = (uint8_t *)pool_allocator_malloc(&pool, 17);
    check_add(created_slabs, pool, 24 + sizeof(freelist_node_meta_t));

    uint8_t *a24 = (uint8_t *)pool_allocator_malloc(&pool, 24);
    check_add(created_slabs, pool, 24 + sizeof(freelist_node_meta_t));

    uint8_t *a25 = (uint8_t *)pool_allocator_malloc(&pool, 25);
    check_add(created_slabs, pool, 32 + sizeof(freelist_node_meta_t));

    uint8_t *a32 = (uint8_t *)pool_allocator_malloc(&pool, 32);
    check_add(created_slabs, pool, 32 + sizeof(freelist_node_meta_t));

    uint8_t *a33 = (uint8_t *)pool_allocator_malloc(&pool, 33);
    check_add(created_slabs, pool, 48 + sizeof(freelist_node_meta_t));

    pool_allocator_free(a16);
    pool_allocator_free(a8);
    pool_allocator_free(a17);
    pool_allocator_free(a24);
    pool_allocator_free(a25);
    pool_allocator_free(a32);
    pool_allocator_free(a33);

    pool_allocator_destroy(&pool);
}

TEST(DataMovementBgThreadTest, test_bg_thread_lifecycle)
{
    ProcStat proc_stat;
    MTTAllocator mtt_allocator;

    unsigned threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(threads_count, 1U);

    mtt_allocator_create(&mtt_allocator);

    threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(threads_count, 2U);

    mtt_allocator_destroy(&mtt_allocator);

    sleep(1);
    threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(threads_count, 1U);
}

TEST(PEBS, Basic)
{
    // do some copying - L3 misses are expected here
    const int size = 1024 * 1024 * 1024;
    volatile int total_sum = 0;
    volatile int *tab = (int *)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        tab[i] = i;
    }

    pebs_debug_check_low = (void *)&tab[0];
    pebs_debug_check_hi = (void *)&tab[size - 1];

    // TODO remove after POC - debug only
    // fprintf(stderr, "array start - end: %p - %p\n", pebs_debug_check_low,
    //    pebs_debug_check_hi);

    // PEBS is enabled during MTT allocator creation
    MTTAllocator mtt_allocator;
    mtt_allocator_create(&mtt_allocator);

    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < size / 2; i++) {
            tab[i] += tab[i * 2] + j;
            total_sum += tab[i];
        }
    }

    // this is done to avoid compiler optimizations
    ASSERT_EQ(total_sum, total_sum);

    // check debug value defined in pebs.c
    ASSERT_GT(pebs_debug_num_samples, 0);
    fprintf(stderr, "got %d samples\n", pebs_debug_num_samples);

    mtt_allocator_destroy(&mtt_allocator);
}
