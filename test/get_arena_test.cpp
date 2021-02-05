// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include <memkind/internal/memkind_arena.h>

#include <algorithm>
#include <gtest/gtest.h>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <pthread.h>

class GetArenaTest: public ::testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

bool uint_comp(unsigned int a, unsigned int b)
{
    return (a < b);
}

TEST_F(GetArenaTest, test_TC_MEMKIND_ThreadHash)
{
#ifdef _OPENMP
    int num_threads = omp_get_max_threads();
    std::vector<unsigned int> arena_idx(num_threads);
    unsigned int thread_idx, idx;
    int err = 0;
    size_t size = 0;
    int i;
    unsigned max_collisions, collisions;
    const unsigned collisions_limit = 5;

    // Initialize kind
    memkind_malloc(MEMKIND_HBW, 0);

#pragma omp parallel shared(arena_idx) private(thread_idx)
    {
        thread_idx = omp_get_thread_num();
        err = memkind_thread_get_arena(MEMKIND_HBW, &(arena_idx[thread_idx]),
                                       size);
    }
    ASSERT_TRUE(err == 0);
    std::sort(arena_idx.begin(), arena_idx.end(), uint_comp);
    idx = arena_idx[0];
    collisions = 0;
    max_collisions = 0;
    for (i = 1; i < num_threads; ++i) {
        if (arena_idx[i] == idx) {
            collisions++;
        } else {
            if (collisions > max_collisions) {
                max_collisions = collisions;
            }
            idx = arena_idx[i];
            collisions = 0;
        }
    }
    EXPECT_LE(max_collisions, collisions_limit);
    RecordProperty("max_collisions", max_collisions);
#else
    std::cout << "[ SKIPPED ] Feature OPENMP not supported" << std::endl;
#endif
}
