/*
 * Copyright (C) 2014, 2015 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memkind.h>
#include <gtest/gtest.h>

/* Test to check each memkind parititon through memkind_get_kind_by_partition API */
class Partition: public :: testing::Test { };

TEST_F(Partition, TC_Memkind_CheckBasePartitions)
{
    int i;
    memkind_t kind;
    memkind_t other_kind;
    void *ptr;

    for (i = 0; i < MEMKIND_NUM_BASE_KIND; ++i) {
        EXPECT_EQ(0, memkind_get_kind_by_partition(i, &kind));
        EXPECT_EQ(kind->partition, i);
        memkind_get_kind_by_name(kind->name, &other_kind);
        EXPECT_EQ(other_kind->partition, i);
        if (memkind_check_available(kind) == 0) {
            ptr = memkind_malloc(kind, 16);
            EXPECT_TRUE(ptr != NULL);
            memkind_free(kind, ptr);
        }
    }
    EXPECT_EQ(MEMKIND_ERROR_UNAVAILABLE,
              memkind_get_kind_by_partition(0xdeadbeef, &kind));
}
