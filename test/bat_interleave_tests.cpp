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

#include <fstream>
#include <numa.h>

#include "common.h"
#include "check.h"
#include "trial_generator.h"
#include <numa.h>
#include <memkind/internal/memkind_hbw.h>


#define MIN_NUMA_NODES_INTERLEAVE 4

/* Set of basic acceptance tests for INTERLEAVE policy, the goal of this set of tests
 * is to prove that you can do incremental allocations of memory with different
 * sizes and that pages are actually allocated alternately in HBW and DRAM nodes.
 */
class BATInterleaveTests: public :: testing::Test
{

protected:
    size_t num_bandwidth;
    int *bandwidth;
    TrialGenerator *tgen;
    bool can_interleave;

    void SetUp()
    {
        //Check system numa nodes, interleave tests need at least 4.
        int config_numa_nodes = numa_num_configured_nodes();
        if (config_numa_nodes >= MIN_NUMA_NODES_INTERLEAVE) {
            num_bandwidth = NUMA_NUM_NODES;
            bandwidth = new int[num_bandwidth];

            nodemask_t nodemask;
            struct bitmask nodemask_bm = {NUMA_NUM_NODES, nodemask.n};
            numa_bitmask_clearall(&nodemask_bm);

            memkind_hbw_all_get_mbind_nodemask(NULL, nodemask.n, NUMA_NUM_NODES);

            int i, nodes_num = numa_num_configured_nodes();
            for (i=0; i<NUMA_NUM_NODES; i++) {
                if (i >= nodes_num) {
                    bandwidth[i] = 0;
                }
                else if (numa_bitmask_isbitset(&nodemask_bm, i)) {
                    bandwidth[i] = 2;
                }
                else {
                    bandwidth[i] = 1;
                }
            }
        }
        else {
            //No node-bandwidth available to parse, can't interleave
            can_interleave = false;
        }
    }

    void TearDown()
    {
        if (can_interleave) {
            delete[] bandwidth;
            delete tgen;
        }
    }

};


TEST_F(BATInterleaveTests, TC_Memkind_HBW_Interleave_CheckAvailable)
{
    hbw_set_policy(HBW_POLICY_INTERLEAVE);
    ASSERT_EQ(0, hbw_check_available());
}

TEST_F(BATInterleaveTests,TC_Memkind_HBW_Interleave_Policy)
{
    hbw_set_policy(HBW_POLICY_INTERLEAVE);
    EXPECT_EQ(HBW_POLICY_INTERLEAVE, hbw_get_policy());
}

TEST_F(BATInterleaveTests, TC_Memkind_HBW_Interleave_MallocIncremental)
{
    if (can_interleave) {
        hbw_set_policy(HBW_POLICY_INTERLEAVE);
        tgen->generate_interleave(HBW_MALLOC);
        tgen->run(num_bandwidth, bandwidth);
    }
    else {
        printf("System needs PMMT table and at least 4 HBW nodes \
                to run interleave allocation tests\n");
    }
}

TEST_F(BATInterleaveTests, TC_Memkind_HBW_Interleave_CallocIncremental)
{
    if (can_interleave) {
        hbw_set_policy(HBW_POLICY_INTERLEAVE);
        tgen->generate_interleave(HBW_CALLOC);
        tgen->run(num_bandwidth, bandwidth);
    }
    else {
        printf("System needs PMMT table and at least 4 HBW nodes \
                to run interleave allocation tests\n");
    }
}


TEST_F(BATInterleaveTests, TC_Memkind_HBW_Interleave_ReallocInremental)
{
    if (can_interleave) {
        hbw_set_policy(HBW_POLICY_INTERLEAVE);
        tgen->generate_interleave(HBW_REALLOC);
        tgen->run(num_bandwidth, bandwidth);
    }
    else {
        printf("System needs PMMT table and at least 4 HBW nodes \
                to run interleave allocation tests\n");
    }
}
