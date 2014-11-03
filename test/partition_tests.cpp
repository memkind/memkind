#include <memkind.h>
#include <gtest.h>


class Partition: public :: testing::Test { };

TEST_F(Partition, check_available_test)
{
    int i;
    for (i = 0; i < MEMKIND_NUM_BASE_KIND; ++i) {
        EXPECT_EQ(0, memkind_partition_check_available(i));
    }
    EXPECT_EQ(MEMKIND_ERROR_UNAVAILABLE, 
              memkind_patition_check_available(0xdeadbeaf);
}
