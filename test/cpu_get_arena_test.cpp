#include <algorithm>
#include <vector>
#include <gtest/gtest.h>
#include <omp.h>
#include "memkind_arena.h"

class GetArenaTest: public :: testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}

};

bool uint_comp(unsigned int a, unsigned int b) { return (a < b); }

TEST_F(GetArenaTest, test_thread_hash)
{
    int num_threads = omp_get_max_threads();
    std::vector<unsigned int> arena_idx(num_threads);
    int err = 0;
    int max_collisions;

#pragma omp parallel shared(arena_idx) private(thread_idx)
{
    thread_idx = omp_get_thread_num();
    err = memkind_thread_get_arena(MEMKIND_HBW, &(arena_idx[thread_idx]));
}
    ASSERT_TRUE(err == 0);
    std::sort(arena_idx.begin(), arena_idx.end(), uint_comp);
    idx = arena_idx[0];
    collisions = 0;
    max_collisions = 0;
    for (i = 1; i < num_threads; ++i) {
        if (arena_idx[i] == idx) {
            collisions++;
        }
        else {
            if (collisions > max_collisions) {
                max_collisions = collisions;
            }
            idx = arena_idx[i];
            collisions = 0;
        }
    }
    EXPECT_TRUE(max_collisions < 4);
}
