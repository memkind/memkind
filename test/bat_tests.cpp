/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
 */

#include <fstream>
#include <algorithm>
#include "common.h"
#include "check.h"
#include "omp.h"
#include "numakind.h"
#include "trial_generator.h"


class BATest: public :: testing::Test
{

protected:
    size_t num_bandwidth;
    int *bandwidth;
    TrialGenerator *tgen;
    void SetUp()
    {
        size_t node;
        char *hbw_nodes_env, *endptr;
        tgen = new TrialGenerator();

        hbw_nodes_env = getenv("NUMAKIND_HBW_NODES");
        if (hbw_nodes_env) {
            num_bandwidth = 128;
            bandwidth = new int[num_bandwidth];
            for (node = 0; node < num_bandwidth; node++) {
                bandwidth[node] = 1;
            }
            node = strtol(hbw_nodes_env, &endptr, 10);
            bandwidth[node] = 2;
            while (*endptr == ':') {
                hbw_nodes_env = endptr + 1;
                node = strtol(hbw_nodes_env, &endptr, 10);
                if (endptr != hbw_nodes_env && node >= 0 && node < num_bandwidth) {
                    bandwidth[node] = 2;
                }
            }
        } else {
            const char *node_bandwidth_path = "/etc/numakind/node-bandwidth";
            std::ifstream nbw_file;

            nbw_file.open(node_bandwidth_path, std::ifstream::binary);
            nbw_file.seekg(0, nbw_file.end);
            num_bandwidth = nbw_file.tellg()/sizeof(int);
            nbw_file.seekg(0, nbw_file.beg);
            bandwidth = new int[num_bandwidth];
            nbw_file.read((char *)bandwidth, num_bandwidth*sizeof(int));
            nbw_file.close();
        }
    }

    void TearDown()
    {
        delete[] bandwidth;
        delete tgen;
    }

};


TEST_F(BATest, hbw_is_available)
{
    ASSERT_EQ(1, hbw_is_available());
}

TEST_F(BATest, hbw_policy)
{
    EXPECT_EQ(1, hbw_get_policy());

}

TEST_F(BATest, hbw_malloc_incremental)
{
    tgen->generate_trials_incremental(MALLOC);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_calloc_incremental)
{
    tgen->generate_trials_incremental(CALLOC);
    tgen->execute_trials(num_bandwidth, bandwidth);
}


TEST_F(BATest, hbw_realloc_incremental)
{
    tgen->generate_trials_incremental(REALLOC);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

#if 0
TEST_F(BATest, hbw_realloc_2KB_2MB_2KB)
{
    size_t size0 = (size_t)(2*KB);
    size_t size1 = (size_t)(2*MB);
    char *ptr;
    ASSERT_TRUE((ptr = (char *)hbw_realloc(NULL, size0)) != NULL);
    Check check1(ptr, size0);
    memset(ptr, 0, size0);
    EXPECT_EQ(0, check1.check_node_hbw(num_bandwidth, bandwidth));
    ASSERT_TRUE((ptr = (char *)hbw_realloc(ptr, size1)) != NULL);
    Check check2(ptr, size1);
    memset(ptr, 0, size1);
    EXPECT_EQ(0, check2.check_node_hbw(num_bandwidth, bandwidth));
    ASSERT_TRUE((ptr = (char *)hbw_realloc(ptr, size0)) != NULL);
    Check check3(ptr, size0);
    memset(ptr, 0, size0);
    EXPECT_EQ(0, check3.check_node_hbw(num_bandwidth, bandwidth));
    hbw_free(ptr);
}
#endif

TEST_F(BATest, hbw_memalign_incremental)
{
    tgen->generate_trials_incremental(MEMALIGN);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_memalign_psize_incremental)
{
    tgen->generate_trials_incremental(MEMALIGN_PSIZE);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_numakind_malloc_recycle)
{
    tgen->generate_trials_recycle_incremental(NUMAKIND_MALLOC);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_numakind_malloc_recycle_psize)
{
    tgen->generate_trials_recycle_psize_incremental(NUMAKIND_MALLOC);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_numakind_trials_two_kind_stress)
{
    tgen->generate_trials_multi_app_stress(2);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_numakind_trials_all_kind_stress)
{
    tgen->generate_trials_multi_app_stress(NUMAKIND_NUM_KIND);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

