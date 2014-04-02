/**
 INTEL CONFIDENTIAL

 Copyright (C) 2007 by Intel Corporation.
 All Rights Reserved.

 The source code contained or described herein and all documents
 related to the source code ("Material") are owned by Intel Corporation
 or its suppliers or licensors. Title to the Material remains with Intel
 Corporation or its suppliers and licensors. The Material contains trade
 secrets and proprietary and confidential information of Intel or its
 suppliers and licensors. The Material is protected by worldwide copyright
 and trade secret laws and treaty provisions. No part of the Material may
 be used, copied, reproduced, modified, published, uploaded, posted,
 transmitted, distributed, or disclosed in any way without Intel's prior
 express written permission.

 No License under any patent, copyright, trade secret or other intellectual
 property right is granted to or conferred upon you by disclosure or
 delivery of the Materials, either expressly, by implication, inducement,
 estoppel or otherwise. Any license under such intellectual property rights
 must be express and approved by Intel in writing.

 Description:
 Helloworld for MYO shared programming model
 In this example, one global shared data(*buffer) is declared, and inited
 at myoiUserInit(). After runtime init(myoiLibInit), both host and LRB can
 have the buffer pointed to the same address.
*/

#include <fstream>
#include "common.h"
#include "check.h"
#include "omp.h"


class BATest: public :: testing::Test
{
protected:
  size_t num_bandwidth;
  int *bandwidth;

  void SetUp()
  {
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

  void TearDown()
  {
    delete[] bandwidth;
  }

};


TEST_F(BATest, HBW_IsHBWAvailable) {
  ASSERT_EQ(1, HBW_IsHBWAvailable());
}

TEST_F(BATest, HBW_policy) {
  EXPECT_EQ(1, HBW_getpolicy());
  HBW_setpolicy(2);
  EXPECT_EQ(2, HBW_getpolicy());
}

TEST_F(BATest, HBW_malloc) {
  int i;
  size_t size;
  size_t size_array[2] = {(size_t)(2*KB), (size_t)(2048*MB)};
  char *ptr;
  Check check;
  for (i = 0; i < sizeof(size_array); ++i) {
      size = size_array[i];
      ASSERT_TRUE(ptr = (char *)HBW_malloc(size));
      memset(ptr, 0, size);
      check = Check(ptr, size);
      EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
      HBW_free(ptr);
  }
}

TEST_F(BATest, HBW_calloc) {
  size_t size = (size_t)(2048*MB);
  char *ptr;
  ASSERT_TRUE(ptr = (char *)HBW_calloc(size, 1));
  for (int i = 0; i < size; ++i) {
      EXPECT_EQ('\0', ptr[i]);
  }
  Check check(ptr, size);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  HBW_free(ptr);
}

TEST_F(BATest, HBW_realloc) {
  size_t size0 = (size_t)(2*KB);
  size_t size1 = (size_t)(2048*MB);
  char *ptr = (char *)HBW_realloc(NULL, size0);
  ASSERT_TRUE(ptr != NULL);
  memset(ptr, 0, size0);
  Check check(ptr, size0);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  ptr = (char *)HBW_realloc(ptr, size1);
  ASSERT_TRUE(ptr != NULL);
  memset(ptr, 0, size1);
  Check check(ptr, size1);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  ptr = (char *)HBW_realloc(ptr, size0);
  ASSERT_TRUE(ptr != NULL);
  memset(ptr, 0, size0);
  Check check(ptr, size0);
  EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
  HBW_free(ptr);
}

// FIX ME have not refactored below

TEST_F(BATest, HBW_allocate_memalign) {
  void *ptr = NULL;
  int ret = 0, fret=0;
  size_t size = (size_t)(2*GB);
  size_t align = 32;

  fret = HBW_allocate_memalign(&ptr,align,size);

  if (fret != 0
      || ((size_t)ptr%align != 0)
      || (NULL == ptr)) {
    ret = HBW_ERROR;
    goto exit;
  }

  /*Check that we got high bandwidth nodes*/
  ASSERT_EQ(0, check_page_hbw(ptr, size));

  if (NULL != ptr) {
    HBW_free(ptr);
  }

 exit:
  ASSERT_EQ(0, ret);
}

TEST_F(BATest, HbwAllocateMemAlignPsize4K) {

  void *ptr = NULL;
  int ret = 0, fret=0;
  size_t size = (size_t)(16*MB);
  size_t align = 4*KB;
  hbw_pagesize_t psize = HBW_PAGESIZE_4KB;

  fret = HBW_allocate_memalign_psize(&ptr,align,size,psize);

  if (fret != 0
      || (NULL == ptr)) {
    ret = HBW_ERROR;
    goto exit;
  }

  /*Check that we got high bandwidth nodes*/
  ASSERT_EQ(0, check_page_hbw(ptr, size));

  if (NULL != ptr) {
    HBW_free(ptr);
  }

 exit:
  ASSERT_EQ(0, ret);
}

TEST_F(BATest, HbwAllocateMemAlignPsize2M) {

  void *ptr = NULL;
  int ret = 0, fret=0;
  size_t size = (size_t)(16*MB);
  size_t align = 2*MB;
  hbw_pagesize_t psize = HBW_PAGESIZE_2MB;

  fret = HBW_allocate_memalign_psize(&ptr,align,size,psize);

  if (fret != 0
      || (NULL == ptr)) {
    ret = HBW_ERROR;
    goto exit;
  }

  /*Check that we got high bandwidth nodes*/
  ASSERT_EQ(0, check_page_hbw(ptr, size));

  if (NULL != ptr) {
    HBW_free(ptr);
  }

 exit:
  ASSERT_EQ(0, ret);
}


