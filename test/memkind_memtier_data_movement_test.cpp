// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021-2022 Intel Corporation. */

#include <memkind/internal/hasher.h>
#include <memkind/internal/memkind_memtier.h>
#include <memkind/internal/pagesizes.h>
#include <memkind/internal/pebs.h>
#include <memkind/internal/pool_allocator.h>
#include <memkind/internal/ranking.h>
#include <memkind/internal/ranking_internals.hpp>
#include <memkind/internal/slab_allocator.h>
#include <mtt_allocator.h>

#include <cmath>
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
        SlabAllocator temp;                                                    \
        int ret = slab_allocator_init(&temp, size, nof_elements);              \
        ASSERT_TRUE(ret == 0 && "mutex creation failed!");                     \
        slab_allocator_destroy(&temp);                                         \
        ret = slab_allocator_init(&temp, size, nof_elements);                  \
        ASSERT_TRUE(ret == 0 && "mutex creation failed!");                     \
        bar##size *elements[nof_elements];                                     \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            elements[i] = (bar##size *)slab_allocator_malloc(&temp);           \
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
            slab_allocator_free(elements[i]);                                  \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            elements[i] = (bar##size *)slab_allocator_malloc(&temp);           \
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
            slab_allocator_free(elements[i]);                                  \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        slab_allocator_destroy(&temp);                                         \
    } while (0)

#define struct_bar_align(size)                                                 \
    typedef struct bar_align##size {                                           \
        uint64_t boo[(size)];                                                  \
    } bar_align##size

struct_bar_align(7);

#define test_slab_allocator_alignment(size, nof_elements)                      \
    do {                                                                       \
        struct_bar_align(size);                                                \
        size_t bar_align_size = sizeof(bar_align##size);                       \
        SlabAllocator temp;                                                    \
        int ret = slab_allocator_init(&temp, bar_align_size, nof_elements);    \
        ASSERT_TRUE(ret == 0 && "mutex creation failed!");                     \
        bar_align##size *elements[nof_elements];                               \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            elements[i] = (bar_align##size *)slab_allocator_malloc(&temp);     \
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
            slab_allocator_free(elements[i]);                                  \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        for (size_t i = 0; i < nof_elements; ++i) {                            \
            elements[i] = (bar_align##size *)slab_allocator_malloc(&temp);     \
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
            slab_allocator_free(elements[i]);                                  \
        }                                                                      \
        ASSERT_TRUE(temp.used == nof_elements);                                \
        slab_allocator_destroy(&temp);                                         \
    } while (0)

static void test_slab_allocator_static3(void)
{
    struct_bar(3);
    size_t NOF_ELEMENTS = 1024;
    size_t SIZE = 3;
    SlabAllocator temp;
    int ret = slab_allocator_init(&temp, SIZE, NOF_ELEMENTS);
    ASSERT_TRUE(ret == 0 && "slab alloc init failed!");
    slab_allocator_destroy(&temp);
    ret = slab_allocator_init(&temp, SIZE, NOF_ELEMENTS);
    ASSERT_TRUE(ret == 0 && "slab alloc init failed!");
    bar3 *elements[NOF_ELEMENTS];
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        elements[i] = (bar3 *)slab_allocator_malloc(&temp);
        ASSERT_TRUE(elements[i] && "slab returned NULL!");
        memset(elements[i], i, SIZE);
    }
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        for (size_t j = 0; j < SIZE; j++)
            ASSERT_TRUE(elements[i]->boo[j] == (char)((unsigned)(i)) % 255);
    }
    ASSERT_TRUE(temp.used == NOF_ELEMENTS);
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        slab_allocator_free(elements[i]);
    }
    ASSERT_TRUE(temp.used == NOF_ELEMENTS);
    for (size_t i = 0; i < NOF_ELEMENTS; ++i) {
        elements[i] = (bar3 *)slab_allocator_malloc(&temp);
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
        slab_allocator_free(elements[i]);
    }
    ASSERT_TRUE(temp.used == NOF_ELEMENTS);
    slab_allocator_destroy(&temp);
}

TEST(SlabAlloc, Basic)
{
    test_slab_allocator_static3();
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
    test_slab_allocator_alignment(1, 100000);
    test_slab_allocator_alignment(1, 213299);
    test_slab_allocator_alignment(2, 912348);
    test_slab_allocator_alignment(4, 821429);
    test_slab_allocator_alignment(8, 814322);
    test_slab_allocator_alignment(7, 291146);
    test_slab_allocator_alignment(7, 291);
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
    static void check_add(std::map<SlabAllocator *, size_t> &slabs,
                          PoolAllocator &pool, size_t expected_size)
    {
        for (volatile size_t i = 0; i < UINT16_MAX; ++i) {
            volatile SlabAllocator *slab = pool.pool[i];
            if (slab) {
                auto temp = slabs.find((SlabAllocator *)slab);
                volatile bool found = temp != slabs.end();
                if (!found || (temp->first->used != temp->second)) {
                    ASSERT_EQ(slab->elementSize, expected_size);
                    slabs[(SlabAllocator *)slab] = slab->used;
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

    std::map<SlabAllocator *, size_t> created_slabs;

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

TEST_F(PoolAllocTest, BigAllocationTest)
{
    size_t alloc_size =
        ((size_t)512) * 1024 * 1024 * sizeof(int); // 512*sizeof(int) MB
    size_t big_alloc_size = ((size_t)16u) * 1024u * 1024u * 1024u; // 16 GB
    PoolAllocator pool;
    int ret = pool_allocator_create(&pool);
    ASSERT_EQ(ret, 0);
    for (size_t i = 0; i < UINT16_MAX; ++i)
        ASSERT_EQ(pool.pool[i], nullptr);

    std::map<SlabAllocator *, size_t> created_slabs;

    uint8_t *temp1 = (uint8_t *)pool_allocator_malloc(&pool, alloc_size);
    check_add(created_slabs, pool, alloc_size + sizeof(freelist_node_meta_t));

    uint8_t *temp2 = (uint8_t *)pool_allocator_malloc(&pool, big_alloc_size);
    check_add(created_slabs, pool,
              big_alloc_size + sizeof(freelist_node_meta_t));

    pool_allocator_free(temp1);
    pool_allocator_free(temp2);
    pool_allocator_destroy(&pool);
}

TEST(DataMovementBgThreadTest, test_bg_thread_lifecycle)
{
    ProcStat proc_stat;
    MTTAllocator mtt_allocator;

    unsigned threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(threads_count, 1U);

    MTTInternalsLimits limits;
    limits.lowLimit = 1024u * 1024u * 1024u * 1024u;
    limits.softLimit = 1024u * 1024u * 1024u * 1024u;
    limits.hardLimit = 1024u * 1024u * 1024u * 1024u;

    mtt_allocator_create(&mtt_allocator, &limits);

    threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(threads_count, 2U);

    mtt_allocator_destroy(&mtt_allocator);

    sleep(1);
    threads_count = proc_stat.get_threads_count();
    ASSERT_EQ(threads_count, 1U);
}

class PEBSTest: public ::testing::Test
{
protected:
    static bool get_highest_hotness(MTTAllocator *alloc, double &hotness)
    {
        // 1. Find a ranking with pages
        // 2. Check page hotness and distribution TODO
        return ranking_get_hottest(alloc->internals.dramRanking, &hotness);
    }
};

TEST_F(PEBSTest, Basic)
{
    // do some copying - L3 misses are expected here
    const int size = 1024 * 1024;
    volatile int total_sum = 0;

    MTTInternalsLimits limits;
    limits.lowLimit = 1024u * 1024u * 1024u;
    limits.softLimit = 1024u * 1024u * 1024u;
    limits.hardLimit = 1024u * 1024u * 1024u;
    // PEBS is enabled during MTT allocator creation
    MTTAllocator mtt_allocator;
    mtt_allocator_create(&mtt_allocator, &limits);

    volatile int *tab =
        (int *)mtt_allocator_malloc(&mtt_allocator, size * sizeof(int));

    double hotness = -1.0;
    bool success = get_highest_hotness(&mtt_allocator, hotness);

    ASSERT_TRUE(success);
    ASSERT_EQ(hotness, 0.0);

    for (int i = 0; i < size; i++) {
        tab[i] = i;
    }

    for (int j = 0; j < 30; j++) {
        for (int i = 0; i < size / 2; i++) {
            tab[i] += tab[i * 2] + j;
            total_sum += tab[i];
        }
    }

    success = get_highest_hotness(&mtt_allocator, hotness);

    ASSERT_TRUE(success);
    ASSERT_GT(hotness, 0.0);

    // check that there are no pages on pmem
    success =
        ranking_get_hottest(mtt_allocator.internals.pmemRanking, &hotness);
    ASSERT_FALSE(success);

    // compiler optimizations
    ASSERT_EQ(total_sum, total_sum);

    mtt_allocator_destroy(&mtt_allocator);
}

TEST(HotnessCoeffTest, Basic)
{
    HotnessCoeff c0(0, 0, 0.5);
    ASSERT_EQ(c0.Get(), 0.0);
    c0.Update(3.0, 0.0);
    ASSERT_EQ(c0.Get(), 1.5);
    c0.Update(5.0, 0.0);
    ASSERT_EQ(c0.Get(), 4.0);
    c0.Update(5.0, 1.0);
    ASSERT_EQ(c0.Get(), 2.5);

    HotnessCoeff c1(10, 0.3, 2.0);
    ASSERT_EQ(c1.Get(), 10.0);
    c1.Update(0.0, 0.0);
    ASSERT_EQ(c1.Get(), 10.0);
    c1.Update(45.0, 0.0);
    ASSERT_EQ(c1.Get(), 100.0);
    c1.Update(0.0, 0.0);
    ASSERT_EQ(c1.Get(), 100.0);
    c1.Update(0.0, 1.0);
    ASSERT_EQ(c1.Get(), 30.0);
    c1.Update(3.0, 2.0);
    ASSERT_EQ(c1.Get(), 2.7 + 6.0);

    // check double overflow
    c1.Update(std::numeric_limits<double>::max() / 2, 0.5);
    c1.Update(std::numeric_limits<double>::max() / 2, 0.1);
    c1.Update(std::numeric_limits<double>::max() / 2, 0.2);
    c1.Update(std::numeric_limits<double>::max() / 2, 0.01);
    c1.Update(std::numeric_limits<double>::max() / 2, 0.02);
    ASSERT_EQ(c1.Get(), std::numeric_limits<double>::max());

    // check if value still decreases with time
    c1.Update(0, 2);
    ASSERT_LT(c1.Get(), std::numeric_limits<double>::max());
    ASSERT_GT(c1.Get(), 0);
}

TEST(HotnessTest, Basic)
{
    const double ACCURACY = 1e-9;
    const uint64_t SECOND = 1000000000u;
    Hotness hotness0_0(0, 0);
    Hotness hotness1_0(1, 0);
    Hotness hotness3_2(3, 2 * SECOND);
    Hotness hotness10_10_0(10, 10 * SECOND);
    Hotness hotness10_10_1(10, 10 * SECOND);
    Hotness hotness10_10_2(10, 10 * SECOND);
    Hotness hotness10_10_3(10, 10 * SECOND);
    Hotness hotness10_10_4(10, 10 * SECOND);

    // instead of checking exact values, we are checking some properties

    // check initialized values
    ASSERT_EQ(hotness0_0.GetTotalHotness(), 0.0);
    ASSERT_EQ(hotness1_0.GetTotalHotness(), 1.0);
    ASSERT_EQ(hotness3_2.GetTotalHotness(), 3.0);
    ASSERT_EQ(hotness10_10_0.GetTotalHotness(), 10.0);
    ASSERT_EQ(hotness10_10_1.GetTotalHotness(), 10.0);
    ASSERT_EQ(hotness10_10_2.GetTotalHotness(), 10.0);
    ASSERT_EQ(hotness10_10_3.GetTotalHotness(), 10.0);
    ASSERT_EQ(hotness10_10_4.GetTotalHotness(), 10.0);

    // check proportions
    ASSERT_EQ(hotness0_0.GetTotalHotness(), 0.0);
    ASSERT_EQ(hotness1_0.GetTotalHotness(), 1.0);
    ASSERT_EQ(hotness3_2.GetTotalHotness() / hotness1_0.GetTotalHotness(), 3.0);
    ASSERT_EQ(hotness10_10_0.GetTotalHotness() / hotness3_2.GetTotalHotness(),
              10.0 / 3.0);

    // check add
    hotness0_0.Update(1.0, 0);
    double nhotness0_0_0 = hotness0_0.GetTotalHotness();
    ASSERT_GT(nhotness0_0_0, 0.0);
    hotness0_0.Update(2.0, 0);
    double nhotness0_0_1 = hotness0_0.GetTotalHotness();
    ASSERT_LT(abs(3.0 * nhotness0_0_0 - nhotness0_0_1), ACCURACY);

    // check if hotness decreases with time
    hotness3_2.Update(0, 4 * SECOND);
    ASSERT_LT(hotness3_2.GetTotalHotness(), 3.0);
    ASSERT_GT(hotness3_2.GetTotalHotness(), 0.0);

    // cross-check with hotness coeffs
    hotness10_10_0.Update(0, 12 * SECOND);
    ASSERT_LT(
        abs(hotness10_10_0.GetTotalHotness() / hotness3_2.GetTotalHotness() -
            10.0 / 3.0),
        ACCURACY);
    hotness10_10_1.Update(0, 14 * SECOND);
    hotness10_10_2.Update(0, 16 * SECOND);
    hotness10_10_3.Update(7, 19 * SECOND);

    HotnessCoeff c0(2.5, EXPONENTIAL_COEFFS_VALS[0],
                    EXPONENTIAL_COEFFS_CONMPENSATION_COEFFS[0]);
    HotnessCoeff c1(2.5, EXPONENTIAL_COEFFS_VALS[1],
                    EXPONENTIAL_COEFFS_CONMPENSATION_COEFFS[1]);
    HotnessCoeff c2(2.5, EXPONENTIAL_COEFFS_VALS[2],
                    EXPONENTIAL_COEFFS_CONMPENSATION_COEFFS[2]);
    HotnessCoeff c3(2.5, EXPONENTIAL_COEFFS_VALS[3],
                    EXPONENTIAL_COEFFS_CONMPENSATION_COEFFS[3]);
    double hotness_0 = c0.Get() + c1.Get() + c2.Get() + c3.Get();
    ASSERT_EQ(10.0, hotness_0);
    c0.Update(0, 2);
    c1.Update(0, 2);
    c2.Update(0, 2);
    c3.Update(0, 2);
    double hotness_1 = c0.Get() + c1.Get() + c2.Get() + c3.Get();
    ASSERT_EQ(hotness10_10_0.GetTotalHotness(), hotness_1);
    c0.Update(0, 2);
    c1.Update(0, 2);
    c2.Update(0, 2);
    c3.Update(0, 2);
    double hotness_2 = c0.Get() + c1.Get() + c2.Get() + c3.Get();
    ASSERT_LE(abs(hotness10_10_1.GetTotalHotness() - hotness_2), ACCURACY);
    c0.Update(0, 2);
    c1.Update(0, 2);
    c2.Update(0, 2);
    c3.Update(0, 2);
    double hotness_3 = c0.Get() + c1.Get() + c2.Get() + c3.Get();
    ASSERT_LE(abs(hotness10_10_2.GetTotalHotness() - hotness_3), ACCURACY);
    c0.Update(7, 3);
    c1.Update(7, 3);
    c2.Update(7, 3);
    c3.Update(7, 3);
    double hotness_4 = c0.Get() + c1.Get() + c2.Get() + c3.Get();
    ASSERT_LE(abs(hotness10_10_3.GetTotalHotness() - hotness_4), ACCURACY);

    // check for double overflow
    hotness10_10_4.Update(std::numeric_limits<double>::max() / 5, 20 * SECOND);
    ASSERT_LT(hotness10_10_4.GetTotalHotness(),
              std::numeric_limits<double>::max());
    ASSERT_GT(hotness10_10_4.GetTotalHotness(), 0);
    hotness10_10_4.Update(std::numeric_limits<double>::max() / 5, 21 * SECOND);
    hotness10_10_4.Update(std::numeric_limits<double>::max() / 5, 22 * SECOND);
    hotness10_10_4.Update(std::numeric_limits<double>::max() / 5, 23 * SECOND);
    hotness10_10_4.Update(std::numeric_limits<double>::max() / 5, 24 * SECOND);
    hotness10_10_4.Update(std::numeric_limits<double>::max() / 5, 25 * SECOND);
    hotness10_10_4.Update(std::numeric_limits<double>::max() / 5, 26 * SECOND);
    ASSERT_EQ(hotness10_10_4.GetTotalHotness(),
              std::numeric_limits<double>::max());
    // check if the value still decreases afterwards
    hotness10_10_4.Update(0.0, 30 * SECOND);
    ASSERT_LT(hotness10_10_4.GetTotalHotness(),
              std::numeric_limits<double>::max());
    ASSERT_GT(hotness10_10_4.GetTotalHotness(), 0);
}

TEST(PageTest, Basic)
{
    const double ACCURACY = 1e-9;
    PageMetadata page(0XFFFF, 10.0, 0);
    ASSERT_EQ(page.GetHotness(), 10.0);
    page.Touch();
    page.Touch();
    page.Touch();
    page.Touch();
    page.Touch();
    page.Touch();
    page.Touch();
    ASSERT_EQ(page.GetHotness(), 10.0);
    page.UpdateHotness(0);
    double hotness_p7 = page.GetHotness() - 10.0;
    ASSERT_GT(hotness_p7, 0.0);
    ASSERT_EQ(page.GetHotness(), hotness_p7 + 10.0);
    page.UpdateHotness(0);
    ASSERT_EQ(page.GetHotness(), hotness_p7 + 10.0);
    page.UpdateHotness(10);
    ASSERT_LT(page.GetHotness(), hotness_p7 + 10.0);
    double temp = page.GetHotness();
    page.Touch();
    ASSERT_EQ(page.GetHotness(), temp);
    page.UpdateHotness(10);
    double hotness_p1 = page.GetHotness() - temp;
    ASSERT_LT(abs(7 * hotness_p1 - hotness_p7), ACCURACY);
}

TEST(RankingTest, Basic)
{
    Ranking ranking;
    Ranking ranking2;

    // check empty hotness

    double hotness0 = -1.0;
    double hotness1 = -1.0;
    bool success0 = ranking.GetHottest(hotness0);
    bool success1 = ranking.GetColdest(hotness1);

    ASSERT_FALSE(success0);
    ASSERT_FALSE(success1);

    // check non-empty hotness
    ranking.AddPages(0xFFFF0000, 2, 0);
    ranking.AddPages(2 * 0xFFFF0000, 3, 0);
    ranking.AddPages(3 * 0xFFFF0000, 3, 2);

    double hotness_hottest0 = -1.0;
    double hotness_coldest0 = -1.0;
    bool success2 = ranking.GetHottest(hotness_hottest0);
    bool success3 = ranking.GetColdest(hotness_coldest0);

    ASSERT_TRUE(success2);
    ASSERT_TRUE(success3);

    // check touch without update
    ASSERT_EQ(hotness_hottest0, hotness_coldest0);
    ASSERT_EQ(hotness_hottest0, 0.0);

    ranking.Touch(0xFFFF0000);
    ranking.Touch(0xFFFF0000);

    double hotness_hottest1 = -1.0;
    double hotness_coldest1 = -1.0;
    (void)ranking.GetHottest(hotness_hottest1);
    (void)ranking.GetColdest(hotness_coldest1);

    ASSERT_EQ(hotness_coldest1, hotness_hottest1);
    ASSERT_EQ(hotness_coldest1, 0.0);

    // check touch with update
    ranking.Update(1000000000);

    double hotness_hottest2 = -1.0;
    double hotness_coldest2 = -1.0;
    (void)ranking.GetHottest(hotness_hottest2);
    (void)ranking.GetColdest(hotness_coldest2);

    ASSERT_LT(hotness_coldest2, hotness_hottest2);
    ASSERT_EQ(hotness_coldest2, 0.0);

    // check moving pages between rankings
    // check new ranking is empty
    double hottest_r2_0_hotness = -1.0;
    double coldest_r2_0_hotness = -1.0;
    bool success4 = ranking2.GetHottest(hottest_r2_0_hotness);
    bool success5 = ranking2.GetColdest(coldest_r2_0_hotness);

    ASSERT_FALSE(success4);
    ASSERT_FALSE(success5);

    // move hottest page to the new ranking
    ranking2.AddPage(ranking.PopHottest());

    double hottest_r2_1_hotness = -1.0;
    double coldest_r2_1_hotness = -1.0;
    bool success6 = ranking2.GetHottest(hottest_r2_1_hotness);
    bool success7 = ranking2.GetColdest(coldest_r2_1_hotness);

    ASSERT_TRUE(success6);
    ASSERT_TRUE(success7);

    ASSERT_EQ(hottest_r2_1_hotness, coldest_r2_1_hotness);
    ASSERT_EQ(hottest_r2_1_hotness, hotness_hottest2);
    ASSERT_GT(hottest_r2_1_hotness, 0.0);

    // move second hottest page to the new ranking
    ranking2.AddPage(ranking.PopHottest());

    double hottest_r2_2_hotness = -1.0;
    double coldest_r2_2_hotness = -1.0;
    (void)ranking2.GetHottest(hottest_r2_2_hotness);
    (void)ranking2.GetColdest(coldest_r2_2_hotness);

    double hottest_3_hotness = -1.0;
    bool success8 = ranking.GetHottest(hottest_3_hotness);

    ASSERT_TRUE(success8);
    ASSERT_GT(hottest_r2_2_hotness, coldest_r2_2_hotness);
    ASSERT_GT(hottest_r2_2_hotness, hottest_3_hotness);
    ASSERT_GT(hottest_r2_2_hotness, 0.0);

    double hottest_4_hotness = -1.0;
    double coldest_4_hotness = -1.0;
    (void)ranking.GetHottest(hottest_4_hotness);
    (void)ranking.GetColdest(coldest_4_hotness);

    ASSERT_LT(hottest_4_hotness, hottest_r2_2_hotness);
    ASSERT_EQ(coldest_4_hotness, hottest_4_hotness);
    ASSERT_EQ(coldest_4_hotness, 0.0);

    // Add new pages to the new ranking
    ranking2.AddPage(ranking.PopColdest());
    ranking2.AddPages(5 * 0xFFFF0000, 1, 2000000000);

    double hottest_r2_3_hotness = -1.0;
    double coldest_r2_3_hotness = -1.0;
    (void)ranking2.GetHottest(hottest_r2_3_hotness);
    (void)ranking2.GetColdest(coldest_r2_3_hotness);

    ASSERT_GT(hottest_r2_3_hotness, coldest_r2_3_hotness);
    ASSERT_EQ(hottest_r2_2_hotness, hottest_r2_3_hotness);

    // remove two hottest page from the new ranking
    (void)ranking2.PopHottest();

    // check the newly added page is the hottest
    double hottest_r2_4_hotness = -1.0;
    bool success9 = ranking2.GetHottest(hottest_r2_4_hotness);

    ASSERT_TRUE(success9);
    ASSERT_EQ(hottest_r2_2_hotness, hottest_r2_4_hotness);

    // pop all pages except for two - hottest and coldest
    (void)ranking2.PopColdest();

    double hottest_r2_5_hotness = -1.0;
    double coldest_r2_5_hotness = -1.0;
    bool success10 = ranking2.GetHottest(hottest_r2_5_hotness);
    bool success11 = ranking2.GetColdest(coldest_r2_5_hotness);

    ASSERT_TRUE(success10);
    ASSERT_TRUE(success11);

    ASSERT_EQ(hottest_r2_5_hotness, hottest_r2_3_hotness);
    ASSERT_EQ(coldest_r2_5_hotness, coldest_r2_3_hotness);
    ASSERT_GT(hottest_r2_5_hotness, coldest_r2_5_hotness);

    // pop hottest, add new page and check if hottest value was updated
    (void)ranking2.PopHottest();
    ranking2.AddPages(6 * 0xFFFF0000, 1, 2500000000);

    double hottest_r2_6_hotness = -1.0;
    double coldest_r2_6_hotness = -1.0;
    bool success12 = ranking2.GetHottest(hottest_r2_6_hotness);
    bool success13 = ranking2.GetColdest(coldest_r2_6_hotness);

    ASSERT_TRUE(success12);
    ASSERT_TRUE(success13);

    ASSERT_EQ(hottest_r2_6_hotness, coldest_r2_6_hotness);
    ASSERT_EQ(coldest_r2_6_hotness, coldest_r2_3_hotness);

    // pop all pages
    (void)ranking2.PopColdest();
    double hottest_r2_7_hotness = -1.0;
    bool success14 = ranking2.GetHottest(hottest_r2_7_hotness);
    ASSERT_TRUE(success14);
    ASSERT_EQ(hottest_r2_7_hotness, 0.0);

    (void)ranking2.PopColdest();
    double hottest_r2_8_hotness = -1.0;
    bool success15 = ranking2.GetHottest(hottest_r2_8_hotness);
    ASSERT_FALSE(success15);
}

class RankingBindingsTest: public ::testing::Test
{
protected:
    ranking_handle ranking1;
    ranking_handle ranking2;
    metadata_handle temp;
    const uint64_t SECOND = 1000000000u;

private:
    void SetUp()
    {
        ranking_create(&ranking1);
        ranking_create(&ranking2);
        ranking_metadata_create(&temp);
    }

    void TearDown()
    {
        ranking_destroy(ranking1);
        ranking_destroy(ranking2);
        ranking_metadata_destroy(temp);
    }
};

TEST_F(RankingBindingsTest, Basic)
{
    // the same as RankingTest, but uses C bindings instead of c++ functions
    // check empty hotness

    double hotness0 = -1.0;
    double hotness1 = -1.0;

    bool success0 = ranking_get_hottest(ranking1, &hotness0);
    bool success1 = ranking_get_coldest(ranking1, &hotness1);

    ASSERT_FALSE(success0);
    ASSERT_FALSE(success1);

    size_t total_size = ranking_get_total_size(ranking1);
    ASSERT_EQ(total_size, 0u);

    // check non-empty hotness
    ranking_add_pages(ranking1, 0xFFFF0000, 2, 0);
    total_size = ranking_get_total_size(ranking1);
    ASSERT_EQ(total_size, 2u * TRACED_PAGESIZE);
    ranking_add_pages(ranking1, 2 * 0xFFFF0000, 3, 0);
    total_size = ranking_get_total_size(ranking1);
    ASSERT_EQ(total_size, 5 * TRACED_PAGESIZE);
    ranking_add_pages(ranking1, 3 * 0xFFFF0000, 3, 2);
    total_size = ranking_get_total_size(ranking1);
    ASSERT_EQ(total_size, 8 * TRACED_PAGESIZE);

    double hotness_hottest0 = -1.0;
    double hotness_coldest0 = -1.0;
    bool success2 = ranking_get_hottest(ranking1, &hotness_hottest0);
    bool success3 = ranking_get_coldest(ranking1, &hotness_coldest0);

    ASSERT_TRUE(success2);
    ASSERT_TRUE(success3);

    // check touch without update
    ASSERT_EQ(hotness_hottest0, hotness_coldest0);
    ASSERT_EQ(hotness_hottest0, 0.0);

    ranking_touch(ranking1, 0xFFFF0000);
    ranking_touch(ranking1, 0xFFFF0000);

    double hotness_hottest1 = -1.0;
    double hotness_coldest1 = -1.0;
    (void)ranking_get_hottest(ranking1, &hotness_hottest1);
    (void)ranking_get_coldest(ranking1, &hotness_coldest1);

    ASSERT_EQ(hotness_coldest1, hotness_hottest1);
    ASSERT_EQ(hotness_coldest1, 0.0);

    // check touch with update
    ranking_update(ranking1, 1000000000);

    double hotness_hottest2 = -1.0;
    double hotness_coldest2 = -1.0;
    (void)ranking_get_hottest(ranking1, &hotness_hottest2);
    (void)ranking_get_coldest(ranking1, &hotness_coldest2);

    ASSERT_LT(hotness_coldest2, hotness_hottest2);
    ASSERT_EQ(hotness_coldest2, 0.0);

    // check moving pages between rankings
    // check new ranking is empty
    double hottest_r2_0_hotness = -1.0;
    double coldest_r2_0_hotness = -1.0;
    bool success4 = ranking_get_hottest(ranking2, &hottest_r2_0_hotness);
    bool success5 = ranking_get_coldest(ranking2, &coldest_r2_0_hotness);

    ASSERT_FALSE(success4);
    ASSERT_FALSE(success5);

    // move hottest page to the new ranking
    ranking_pop_hottest(ranking1, temp);
    ranking_add_page(ranking2, temp);

    total_size = ranking_get_total_size(ranking1);
    ASSERT_EQ(total_size, 7 * TRACED_PAGESIZE);

    total_size = ranking_get_total_size(ranking2);
    ASSERT_EQ(total_size, 1 * TRACED_PAGESIZE);

    double hottest_r2_1_hotness = -1.0;
    double coldest_r2_1_hotness = -1.0;
    bool success6 = ranking_get_hottest(ranking2, &hottest_r2_1_hotness);
    bool success7 = ranking_get_coldest(ranking2, &coldest_r2_1_hotness);

    ASSERT_TRUE(success6);
    ASSERT_TRUE(success7);

    ASSERT_EQ(hottest_r2_1_hotness, coldest_r2_1_hotness);
    ASSERT_EQ(hottest_r2_1_hotness, hotness_hottest2);
    ASSERT_GT(hottest_r2_1_hotness, 0.0);

    // move second hottest page to the new ranking
    ranking_pop_hottest(ranking1, temp);
    ranking_add_page(ranking2, temp);

    total_size = ranking_get_total_size(ranking1);
    ASSERT_EQ(total_size, 6 * TRACED_PAGESIZE);

    total_size = ranking_get_total_size(ranking2);
    ASSERT_EQ(total_size, 2 * TRACED_PAGESIZE);

    double hottest_r2_2_hotness = -1.0;
    double coldest_r2_2_hotness = -1.0;
    (void)ranking_get_hottest(ranking2, &hottest_r2_2_hotness);
    (void)ranking_get_coldest(ranking2, &coldest_r2_2_hotness);

    double hottest_3_hotness = -1.0;
    bool success8 = ranking_get_hottest(ranking1, &hottest_3_hotness);

    ASSERT_TRUE(success8);
    ASSERT_GT(hottest_r2_2_hotness, coldest_r2_2_hotness);
    ASSERT_GT(hottest_r2_2_hotness, hottest_3_hotness);
    ASSERT_GT(hottest_r2_2_hotness, 0.0);

    double hottest_4_hotness = -1.0;
    double coldest_4_hotness = -1.0;
    (void)ranking_get_hottest(ranking1, &hottest_4_hotness);
    (void)ranking_get_coldest(ranking1, &coldest_4_hotness);

    ASSERT_LT(hottest_4_hotness, hottest_r2_2_hotness);
    ASSERT_EQ(coldest_4_hotness, hottest_4_hotness);
    ASSERT_EQ(coldest_4_hotness, 0.0);

    // Add new pages to the new ranking
    ranking_pop_coldest(ranking1, temp);
    ranking_add_page(ranking2, temp);
    ranking_add_pages(ranking2, 5 * 0xFFFF0000, 1, 2000000000);

    total_size = ranking_get_total_size(ranking1);
    ASSERT_EQ(total_size, 5 * TRACED_PAGESIZE);

    total_size = ranking_get_total_size(ranking2);
    ASSERT_EQ(total_size, 4 * TRACED_PAGESIZE);

    double hottest_r2_3_hotness = -1.0;
    double coldest_r2_3_hotness = -1.0;
    (void)ranking_get_hottest(ranking2, &hottest_r2_3_hotness);
    (void)ranking_get_coldest(ranking2, &coldest_r2_3_hotness);

    ASSERT_GT(hottest_r2_3_hotness, coldest_r2_3_hotness);
    ASSERT_EQ(hottest_r2_2_hotness, hottest_r2_3_hotness);

    // remove two hottest page from the new ranking
    (void)ranking_pop_hottest(ranking2, temp);

    total_size = ranking_get_total_size(ranking2);
    ASSERT_EQ(total_size, 3 * TRACED_PAGESIZE);

    // check the newly added page is the hottest
    double hottest_r2_4_hotness = -1.0;
    bool success9 = ranking_get_hottest(ranking2, &hottest_r2_4_hotness);

    ASSERT_TRUE(success9);
    ASSERT_EQ(hottest_r2_2_hotness, hottest_r2_4_hotness);

    // pop all pages except for two - hottest and coldest
    ranking_pop_coldest(ranking2, temp);

    total_size = ranking_get_total_size(ranking2);
    ASSERT_EQ(total_size, 2 * TRACED_PAGESIZE);

    double hottest_r2_5_hotness = -1.0;
    double coldest_r2_5_hotness = -1.0;
    bool success10 = ranking_get_hottest(ranking2, &hottest_r2_5_hotness);
    bool success11 = ranking_get_coldest(ranking2, &coldest_r2_5_hotness);

    ASSERT_TRUE(success10);
    ASSERT_TRUE(success11);

    ASSERT_EQ(hottest_r2_5_hotness, hottest_r2_3_hotness);
    ASSERT_EQ(coldest_r2_5_hotness, coldest_r2_3_hotness);
    ASSERT_GT(hottest_r2_5_hotness, coldest_r2_5_hotness);

    // pop hottest, add new page and check if hottest value was updated
    ranking_pop_hottest(ranking2, temp);
    ranking_add_pages(ranking2, 6 * 0xFFFF0000, 1, 2500000000);

    total_size = ranking_get_total_size(ranking2);
    ASSERT_EQ(total_size, 2 * TRACED_PAGESIZE);

    double hottest_r2_6_hotness = -1.0;
    double coldest_r2_6_hotness = -1.0;
    bool success12 = ranking_get_hottest(ranking2, &hottest_r2_6_hotness);
    bool success13 = ranking_get_coldest(ranking2, &coldest_r2_6_hotness);

    ASSERT_TRUE(success12);
    ASSERT_TRUE(success13);

    ASSERT_EQ(hottest_r2_6_hotness, coldest_r2_6_hotness);
    ASSERT_EQ(coldest_r2_6_hotness, coldest_r2_3_hotness);

    // pop all pages
    ranking_pop_coldest(ranking2, temp);
    double hottest_r2_7_hotness = -1.0;
    bool success14 = ranking_get_hottest(ranking2, &hottest_r2_7_hotness);
    ASSERT_TRUE(success14);
    ASSERT_EQ(hottest_r2_7_hotness, 0.0);

    total_size = ranking_get_total_size(ranking2);
    ASSERT_EQ(total_size, 1 * TRACED_PAGESIZE);

    ranking_pop_coldest(ranking2, temp);
    double hottest_r2_8_hotness = -1.0;
    bool success15 = ranking_get_hottest(ranking2, &hottest_r2_8_hotness);
    ASSERT_FALSE(success15);

    total_size = ranking_get_total_size(ranking2);
    ASSERT_EQ(total_size, 0u);
}
