/*
 * Copyright (C) 2014 - 2016 Intel Corporation.
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

#include "memkind.h"

#include <fstream>
#include <algorithm>

#include "common.h"
#include "check.h"
#include "omp.h"
#include "trial_generator.h"

/* Set of basic acceptance tests for PREFERRED policy, the goal of this set of tests
 * is to prove that you can do incremental allocations of memory with different
 * sizes and that pages are actually allocated in HBW node.
 */
class EnvHbwMallocTest : public TGTest
{
};


TEST_F(EnvHbwMallocTest, TC_Memkind_Negative_Hbw_Malloc)
{
    /* Set usupported value of MEMKIND_HBW_NODES, then try to perform
      a successfull allocation from DRAM using hbw_malloc()
      thanks to default HBW_POLICY_PREFERRED policy*/

    static const char numa_warning[] = "libnuma: Warning: node argument -1 is out of range";
    char buf[KB];
    trial_t t;
    t.size = KB;
    t.page_size = 0;

    ASSERT_EQ(setenv("MEMKIND_HBW_NODES", "-1", 1), 0);

    void *ptr = NULL;

    buf[0] = '\0';
    setbuf(stderr, buf);
    ptr = hbw_malloc(KB);
    EXPECT_EQ(ptr != NULL, true);
    EXPECT_EQ(strncmp(numa_warning, buf, strlen(numa_warning)), 0);

    Check c(ptr, t);
    c.check_node_hbw();

    hbw_free(ptr);
    buf[0] = '\0';
    setbuf(stderr, NULL);
}
