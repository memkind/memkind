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

#include "common.h"
#include "omp.h"

#define NTHREADS 2

class APITESTS: public :: testing::Test
{

protected:

  APITESTS()
  {}

  void SetUp()
  {

  }

  void TearDown()
  {
  }

};

class THREADEDMALLOC: public :: testing::Test
{

protected:
  
  THREADEDMALLOC()
  {}
  
  void SetUp()
  {}
  
  void TearDown()
  {}

};



class MALLOCTESTS: public :: testing::Test
{
  
protected:
  
  MALLOCTESTS()
  {}
  
  void SetUp()
  {}
  
  void TearDown()
  {}

};


TEST_F(APITESTS,HbwExistsTest){

  int ret = HBW_SUCCESS;
  ret = HBW_IsHBWAvailable();
  ASSERT_EQ(1,ret);

}

TEST_F(APITESTS,HbwPolicy){
  ASSERT_EQ(1, HBW_getpolicy());
  HBW_setpolicy(2);
  ASSERT_EQ(2, HBW_getpolicy());
}


TEST_F(APITESTS, HbwMalloc2GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  size_t size = (size_t)(2048*MB);
  ptr = (char *) HBW_malloc(size);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  /*Lets touch a part of this memory*/
  ptr[1024] ='a';

  if (NULL != ptr){
    HBW_free(ptr);
  }

  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwCalloc2GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  size_t size = (size_t)(2*GB);
  ptr = (char *) HBW_calloc(size,1);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  /*Lets touch a part of this memory*/
  ptr[1024] ='a';

  if (NULL != ptr){
    HBW_free(ptr);
  }

  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwRealloc2GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  size_t size = (size_t)(2*GB);
  ptr = (char *) HBW_realloc(ptr, size);

  if (NULL == ptr){
    ret = HBW_ERROR;
  }

  /*Lets touch a part of this memory*/
  ptr[1024] ='a';

  if (NULL != ptr){
    HBW_free(ptr);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwAllocateMemAlign2GB){

  void *ptr = NULL;
  int ret = HBW_SUCCESS, fret=0;
  size_t size = (size_t)(2*GB);
  size_t align = 32;
  char *tmp = NULL;

  fret = HBW_allocate_memalign(&ptr,align,size);

  if (fret != HBW_SUCCESS
      || ((size_t)ptr%align != 0)
      || (NULL == ptr)){
    ret = HBW_ERROR;
    goto exit;
  }

  tmp = (char *)ptr;
  tmp[1024] ='a';

  if (NULL != ptr){
    HBW_free(ptr);
  }

 exit:
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwAllocateMemAlignPsize4K){

  void *ptr = NULL;
  int ret = HBW_SUCCESS, fret=0;
  size_t size = (size_t)(16*MB);
  char *tmp = NULL;
  size_t align = 4*KB;
  hbw_pagesize_t psize = HBW_PAGESIZE_4KB;

  fret = HBW_allocate_memalign_psize(&ptr,align,size,psize);

  if (fret != HBW_SUCCESS
      || (NULL == ptr)){
    ret = HBW_ERROR;
    goto exit;
  }

  tmp = (char *)ptr;
  tmp[1024] = 'a';

  if (NULL != ptr){
    HBW_free(ptr);
  }

 exit:
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwAllocateMemAlignPsize2M){

  void *ptr = NULL;
  int ret = HBW_SUCCESS, fret=0;
  size_t size = (size_t)(16*MB);
  char *tmp = NULL;
  size_t align = 2*MB;
  hbw_pagesize_t psize = HBW_PAGESIZE_2MB;

  fret = HBW_allocate_memalign_psize(&ptr,align,size,psize);

  if (fret != HBW_SUCCESS
      || (NULL == ptr)){
    ret = HBW_ERROR;
    goto exit;
  }

  tmp = (char *)ptr;
  tmp[1024] = 'a';

  if (NULL != ptr){
    HBW_free(ptr);
  }

 exit:
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc1KB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }

  ptr[1024] ='a';

  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc2KB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(2*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }

  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}



TEST_F(MALLOCTESTS, HbwMalloc4KB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(4 * KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }

  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc16KB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(16 * KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }

  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc256KB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(256 * KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc512KB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(512 * KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc1MB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}


TEST_F(MALLOCTESTS, HbwMalloc2MB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(2 * MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc4MB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(4 * MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc16MB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(16 * MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc256MB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(256 * MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc512MB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(512 * MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc1GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc2GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(2 * GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc4GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(4 * GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(MALLOCTESTS, HbwMalloc8GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  ptr = (char *) HBW_malloc(8 * GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  ptr[1024] ='a';
  HBW_free(ptr);
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(THREADEDMALLOC, HbwMalloc1KB){

  char *ptr[NTHREADS];
  int ret = HBW_SUCCESS;
  omp_set_num_threads(NTHREADS);

#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(KB);
    if (NULL == ptr[tid]){
      ret = HBW_ERROR;
    }
  
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(THREADEDMALLOC, HbwMalloc4KB){

  char *ptr[NTHREADS];
  int ret = HBW_SUCCESS;
  omp_set_num_threads(NTHREADS);

#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(4 * KB);
    if (NULL == ptr[tid]){
      ret = HBW_ERROR;
    }
  
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}


TEST_F(THREADEDMALLOC, HbwMalloc2MB){

  char *ptr[NTHREADS];
  int ret = HBW_SUCCESS;
  omp_set_num_threads(NTHREADS);

#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(2 * MB);
    if (NULL == ptr[tid]){
      ret = HBW_ERROR;
    }
  
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}


TEST_F(THREADEDMALLOC, HbwMalloc16MB){

  char *ptr[NTHREADS];
  int ret = HBW_SUCCESS;
  omp_set_num_threads(NTHREADS);

#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(16 * MB);
    if (NULL == ptr[tid]){
      ret = HBW_ERROR;
    }
  
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(THREADEDMALLOC, HbwMalloc2GB){

  char *ptr[NTHREADS];
  int ret = HBW_SUCCESS;
  omp_set_num_threads(NTHREADS);

#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(2 * GB);
    if (NULL == ptr[tid]){
      ret = HBW_ERROR;
    }
  
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}


