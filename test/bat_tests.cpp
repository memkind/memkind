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


class BATest: public :: testing::Test
{
protected:
  size_t num_bandwidth;
  int *bandwidth;
  void SetUp()
  {
        int node;
        char *hbw_nodes_env, *endptr;

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

TEST_F(BATest, hbw_malloc_2B) {
  size_t size = (size_t)(2);
  char *ptr;
  ASSERT_TRUE((ptr = (char *)hbw_malloc(size)) != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_malloc_2KB) {
  size_t size = (size_t)(2*KB);
  char *ptr;
  ASSERT_TRUE((ptr = (char *)hbw_malloc(size)) != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_malloc_2MB) {
  size_t size = (size_t)(2*MB);
  char *ptr;
  ASSERT_TRUE((ptr = (char *)hbw_malloc(size)) != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_malloc_2GB) {
  size_t size = (size_t)(2*GB);
  char *ptr;
  ASSERT_TRUE((ptr = (char *)hbw_malloc(size)) != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_calloc_2B) {
  size_t size = (size_t)(2);
  char *ptr;
  ASSERT_TRUE((ptr = (char *)hbw_calloc(size, 1)) != NULL);
  Check check(ptr, size);
  EXPECT_EQ(0, check.check_zero());
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_calloc_2KB) {
  size_t size = (size_t)(2*KB);
  char *ptr;
  ASSERT_TRUE((ptr = (char *)hbw_calloc(size, 1)) != NULL);
  Check check(ptr, size);
  EXPECT_EQ(0, check.check_zero());
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_calloc_2MB) {
  size_t size = (size_t)(2*MB);
  char *ptr;
  ASSERT_TRUE((ptr = (char *)hbw_calloc(size, 1)) != NULL);
  Check check(ptr, size);
  EXPECT_EQ(0, check.check_zero());
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_calloc_2GB) {
  size_t size = (size_t)(2*GB);
  char *ptr;
  ASSERT_TRUE((ptr = (char *)hbw_calloc(size, 1)) != NULL);
  Check check(ptr, size);
  EXPECT_EQ(0, check.check_zero());
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  hbw_free(ptr);
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

TEST_F(BATest, hbw_allocate_memalign_2B) {
  void *ptr = NULL;
  size_t size = (size_t)(2);
  size_t align = 32;
  ASSERT_EQ(0, hbw_allocate_memalign(&ptr, align, size));
  ASSERT_TRUE(ptr != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  EXPECT_EQ(0, check.check_align(align));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_allocate_memalign_2KB) {
  void *ptr = NULL;
  size_t size = (size_t)(2*KB);
  size_t align = 32;
  ASSERT_EQ(0, hbw_allocate_memalign(&ptr, align, size));
  ASSERT_TRUE(ptr != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  EXPECT_EQ(0, check.check_align(align));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_allocate_memalign_2MB) {
  void *ptr = NULL;
  size_t size = (size_t)(2*MB);
  size_t align = 32;
  ASSERT_EQ(0, hbw_allocate_memalign(&ptr, align, size));
  ASSERT_TRUE(ptr != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  EXPECT_EQ(0, check.check_align(align));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_allocate_memalign_2GB) {
  void *ptr = NULL;
  size_t size = (size_t)(2*GB);
  size_t align = 32;

  ASSERT_EQ(0, hbw_allocate_memalign(&ptr, align, size));
  ASSERT_TRUE(ptr != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  EXPECT_EQ(0, check.check_align(align));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_allocate_memalign_psize_2B) {
  void *ptr = NULL;
  size_t size = (size_t)(2);
  hbw_pagesize_t hbw_psize = HBW_PAGESIZE_4KB;
  size_t align = 8;
  ASSERT_EQ(0, hbw_allocate_memalign_psize(&ptr, align, size, hbw_psize));
  ASSERT_TRUE(ptr != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  EXPECT_EQ(0, check.check_align(align));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_allocate_memalign_psize_2KB) {
  void *ptr = NULL;
  size_t size = (size_t)(2*KB);
  hbw_pagesize_t hbw_psize = HBW_PAGESIZE_4KB;
  size_t align = 128;
  ASSERT_EQ(0, hbw_allocate_memalign_psize(&ptr, align, size, hbw_psize));
  ASSERT_TRUE(ptr != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  EXPECT_EQ(0, check.check_align(align));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_allocate_memalign_psize_2MB) {
  void *ptr = NULL;
  size_t size = (size_t)(2*MB);
  hbw_pagesize_t hbw_psize = HBW_PAGESIZE_2MB;
  size_t psize = 2*MB;
  size_t align = 4*KB;
  ASSERT_EQ(0, hbw_allocate_memalign_psize(&ptr, align, size, hbw_psize));
  ASSERT_TRUE(ptr != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  EXPECT_EQ(0, check.check_page_size(psize));
  EXPECT_EQ(0, check.check_align(align));
  hbw_free(ptr);
}

TEST_F(BATest, hbw_allocate_memalign_psize_2GB) {
  void *ptr = NULL;
  size_t size = (size_t)(2*GB);
  hbw_pagesize_t hbw_psize = HBW_PAGESIZE_2MB;
  size_t psize = 2*MB;
  size_t align = 2*MB;
  ASSERT_EQ(0, hbw_allocate_memalign_psize(&ptr, align, size, hbw_psize));
  ASSERT_TRUE(ptr != NULL);
  Check check(ptr, size);
  memset(ptr, 0, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  EXPECT_EQ(0, check.check_page_size(psize));
  EXPECT_EQ(0, check.check_align(align));
  hbw_free(ptr);
}

TEST_F(BATest, numakind_malloc_recycle_hbw_2MB)
{
    void *ptr = NULL;
    size_t size = (size_t)(2*MB);
    ptr = numakind_malloc(NUMAKIND_DEFAULT, size);
    ASSERT_TRUE(ptr != NULL);
    memset(ptr, 0, size);
    numakind_free(NUMAKIND_DEFAULT, ptr);
    ptr = numakind_malloc(NUMAKIND_HBW, size);
    ASSERT_TRUE(ptr != NULL);
    Check check(ptr, size);
    memset(ptr, 0, size);
    EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
    numakind_free(NUMAKIND_HBW, ptr);
}

TEST_F(BATest, numakind_malloc_recycle_hbw_2GB)
{
    void *ptr = NULL;
    size_t size = (size_t)(2*GB);
    ptr = numakind_malloc(NUMAKIND_DEFAULT, size);
    ASSERT_TRUE(ptr != NULL);
    memset(ptr, 0, size);
    numakind_free(NUMAKIND_DEFAULT, ptr);
    ptr = numakind_malloc(NUMAKIND_HBW, size);
    ASSERT_TRUE(ptr != NULL);
    Check check(ptr, size);
    memset(ptr, 0, size);
    EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
    numakind_free(NUMAKIND_HBW, ptr);
}

TEST_F(BATest, numakind_malloc_recycle_psize_2MB)
{
    void *ptr = NULL;
    size_t size = (size_t)(2*MB);
    ptr = numakind_malloc(NUMAKIND_HBW, size);
    ASSERT_TRUE(ptr != NULL);
    Check check0(ptr, size);
    memset(ptr, 0, size);
    EXPECT_EQ(0, check0.check_node_hbw(num_bandwidth, bandwidth));
    numakind_free(NUMAKIND_HBW, ptr);
    ptr = numakind_malloc(NUMAKIND_HBW_HUGETLB, size);
    ASSERT_TRUE(ptr != NULL);
    Check check1(ptr, size);
    memset(ptr, 0, size);
    EXPECT_EQ(0, check1.check_node_hbw(num_bandwidth, bandwidth));
    EXPECT_EQ(0, check1.check_page_size(2*MB));
    numakind_free(NUMAKIND_HBW_HUGETLB, ptr);
}

TEST_F(BATest, numakind_malloc_recycle_psize_2GB)
{
    void *ptr = NULL;
    size_t size = (size_t)(2*GB);
    ptr = numakind_malloc(NUMAKIND_HBW, size);
    ASSERT_TRUE(ptr != NULL);
    Check check0(ptr, size);
    memset(ptr, 0, size);
    EXPECT_EQ(0, check0.check_node_hbw(num_bandwidth, bandwidth));
    numakind_free(NUMAKIND_HBW, ptr);
    ptr = numakind_malloc(NUMAKIND_HBW_HUGETLB, size);
    ASSERT_TRUE(ptr != NULL);
    Check check1(ptr, size);
    memset(ptr, 0, size);
    EXPECT_EQ(0, check1.check_node_hbw(num_bandwidth, bandwidth));
    EXPECT_EQ(0, check1.check_page_size(2*MB));
    numakind_free(NUMAKIND_HBW_HUGETLB, ptr);
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

