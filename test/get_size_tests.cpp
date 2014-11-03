#include <memkind.h>
#include <gtest/gtest.h>

class GetSize: public :: testing::Test { };

TEST_F(GetSize, get_size_test)
{
    size_t total = 0;
    size_t free = 0;
    memkind_get_size(MEMKIND_DEFAULT, &total, &free);
    EXPECT_TRUE(total > 0);
    EXPECT_TRUE(free > 0);
    EXPECT_TRUE(total >= free);
}
