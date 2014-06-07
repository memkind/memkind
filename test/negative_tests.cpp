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
#include <numa.h>
#include <errno.h>
#include "common.h"
#include "check.h"
#include "omp.h"
#include "numakind.h"
#include "trial_generator.h"


class NegativeTest: public :: testing::Test
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


TEST_F(NegativeTest, ErrorUnavailable)
{
    int ret = 0;
    int numakind_flags;
    unsigned long maxnodes = NUMA_NUM_NODES;
    nodemask_t nodemask;
    numakind_error_t err = NUMAKIND_ERROR_UNAVAILABLE;
    ret = numakind_get_mmap_flags(-1, 
				  &numakind_flags);
    EXPECT_EQ(NUMAKIND_ERROR_UNAVAILABLE, ret);
    ret = numakind_get_nodemask(-1, nodemask.n, maxnodes);
    
    EXPECT_EQ(err, ret);
    ret = numakind_mbind(-1, NULL, 1024);
    
    EXPECT_EQ(err, ret);
}


TEST_F(NegativeTest, ErrorMbind)
{
    int ret = 0;
    numakind_error_t err = NUMAKIND_ERROR_MBIND;
    
    ret = numakind_mbind(NUMAKIND_HBW,
			 NULL,
			 1024);
    EXPECT_EQ(err, ret);
}

TEST_F(NegativeTest, ErrorMemalign)
{
    int ret = 0;
    void *ptr = NULL;
    numakind_error_t err = NUMAKIND_ERROR_MEMALIGN;
    
    ret = numakind_posix_memalign(NUMAKIND_DEFAULT,
				  &ptr, 5,
				  100);
    EXPECT_EQ(err, ret);
}

TEST_F(NegativeTest, ErrorAlignment)
{
    int ret = 0;
    void *ptr = NULL;
    numakind_error_t err = NUMAKIND_ERROR_ALIGNMENT;
    
    ret = numakind_posix_memalign(NUMAKIND_HBW,
				  &ptr, 5,
				  100);
    EXPECT_EQ(err, ret);
}

#if 0
TEST_F(NegativeTest, ErrorAllocM)
{
    int ret = 0;
    void *ptr = NULL;
    numakind_error_t err = NUMAKIND_ERROR_ALLOCM;
    
    ptr = numakind_malloc(NUMAKIND_DEFAULT,
			  100);
     
    ret = numakind_posix_memalign(NUMAKIND_HBW,
				  &ptr,
				  16,
				  100);
    EXPECT_EQ(err, ret);
				  
    
}
#endif

TEST_F(NegativeTest, InvalidSizeMalloc)
{
    void *ptr = NULL;
    ptr = hbw_malloc(-1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);

    ptr = numakind_malloc(NUMAKIND_HBW, -1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);
}

TEST_F(NegativeTest, InvalidSizeCalloc)
{
    void *ptr = NULL;
    ptr = hbw_calloc(1, -1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);

    ptr = numakind_calloc(NUMAKIND_HBW, 1,
			  -1);
    ASSERT_TRUE(ptr == NULL);
    EXPECT_EQ(errno, ENOMEM);
}

TEST_F(NegativeTest, InvalidSizeRealloc){

    void *ptr = NULL;
    ptr = hbw_realloc(ptr, -1);
    ASSERT_TRUE(ptr==NULL);
    EXPECT_EQ(errno, ENOMEM);

    ptr = numakind_realloc(NUMAKIND_HBW,
			   ptr,
			   -1);
    ASSERT_TRUE(ptr==NULL);
    EXPECT_EQ(errno, ENOMEM);
}

