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
        }
        else {
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
  }

};


TEST_F(BATest, hbw_is_available) {
  ASSERT_EQ(1, hbw_is_available());
}

TEST_F(BATest, hbw_policy) {
  EXPECT_EQ(1, hbw_get_policy());
  hbw_set_policy(2);
  EXPECT_EQ(2, hbw_get_policy());
}

TEST_F(BATest, hbw_malloc_incremental) {
  tgen->generate_trials_incremental(MALLOC);
  tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_calloc_incremental){
    tgen->generate_trials_incremental(CALLOC);
    tgen->execute_trials(num_bandwidth, bandwidth);
}



TEST_F(BATest, hbw_realloc_2KB_2MB_2KB) {
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

TEST_F(BATest, hbw_memalign_incremental){
    tgen->generate_trials_incremental(MEMALIGN);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_memalign_psize_incremental){
    tgen->generate_trials_incremental(MEMALIGN_PSIZE);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_numakind_malloc_recycle){
    tgen->generate_trials_recycle_incremental(NUMAKIND_MALLOC);
    tgen->execute_trials(num_bandwidth, bandwidth);
}

TEST_F(BATest, hbw_numakind_malloc_recycle_psize){
    tgen->generate_trials_recycle_psize_incremental(NUMAKIND_MALLOC);
    tgen->execute_trials(num_bandwidth, bandwidth);       
}

int myrandom(int i) { return random() % i;}

TEST_F(BATest, numakind_malloc_stress_hbw)
{
    int i;
    numakind_t kind;
    int num_trials = 1000;
    void *ptr;
    size_t size;
    std::vector<void *> ptrs;

    srandom(0);

    for (i = 0; i < num_trials; i++) {
        // choose at random to allocate or free (unless there is nothing to free)
        if (ptrs.size() == 0 || myrandom(3)) {
            size = myrandom(8*MB - 1) + 1;
            // size = (random() & (~0ULL >> (63 - myrandom(28)))) + 1;
            // choose at random between NUMAKIND_DEFAULT and NUMAKIND_HBW
            kind = (numakind_t)myrandom(2);
            ptr = numakind_malloc(kind, size);
            ASSERT_TRUE(ptr != NULL);
            memset(ptr, 0, size);
            if (kind == NUMAKIND_HBW) {
                Check check(ptr, size);
                EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
            }
            ptrs.push_back(ptr);
        }
        else {
            // free a random pointer from the list
            std::random_shuffle(ptrs.begin(), ptrs.end(), myrandom);
            numakind_free(NUMAKIND_DEFAULT, ptrs.back());
            ptrs.pop_back();
        }
    }
    while (ptrs.size()) {
        // free any left overs
        numakind_free(NUMAKIND_DEFAULT, ptrs.back());
        ptrs.pop_back();
    }
}

TEST_F(BATest, numakind_malloc_stress_all)
{
    int i;
    numakind_t kind;
    int num_trials = 1000;
    void *ptr;
    size_t size;
    std::vector<void *> ptrs;

    srandom(0);

    for (i = 0; i < num_trials; i++) {
        // choose at random to allocate or free (unless there is nothing to free)
        if (ptrs.size() == 0 || myrandom(3)) {
            size = myrandom(8*MB - 1) + 1;
            // size = (random() & (~0ULL >> (63 - myrandom(28)))) + 1;
            // choose kind at random
            kind = (numakind_t)myrandom(NUMAKIND_NUM_KIND);
            ptr = numakind_malloc(kind, size);
            ASSERT_TRUE(ptr != NULL);
            memset(ptr, 0, size);
            Check check(ptr, size);
            if (kind == NUMAKIND_HBW ||
                kind == NUMAKIND_HBW_HUGETLB ||
                kind == NUMAKIND_HBW_PREFERRED ||
                kind == NUMAKIND_HBW_PREFERRED_HUGETLB) {
                EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
            }
            if (kind == NUMAKIND_HBW_HUGETLB ||
                kind == NUMAKIND_HBW_PREFERRED_HUGETLB) {
                EXPECT_EQ(0, check.check_page_size(2*MB));
            }
            ptrs.push_back(ptr);
        }
        else {
            // free a random pointer from the list
            std::random_shuffle(ptrs.begin(), ptrs.end(), myrandom);
            numakind_free(NUMAKIND_DEFAULT, ptrs.back());
            ptrs.pop_back();
        }
    }
    while (ptrs.size()) {
        // free any left overs
        numakind_free(NUMAKIND_DEFAULT, ptrs.back());
        ptrs.pop_back();
    }
}

