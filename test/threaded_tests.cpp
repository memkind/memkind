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

class THREADEDMALLOC: public :: testing::Test
{
protected:
  char *ptr[NTHREADS];
  int ret;
  int tid;

  THREADEDMALLOC()
  {}

  void SetUp()
  {
    ret = HBW_SUCCESS;
    omp_set_num_threads(NTHREADS);
  }

  void TearDown()
  {
    ASSERT_EQ(HBW_SUCCESS, ret);
  }

};


TEST_F(THREADEDMALLOC, HbwMalloc1KB){
  #pragma omp parallel num_threads(NTHREADS)
  {
    tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(KB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
}


TEST_F(THREADEDMALLOC, HbwMalloc4KB){
  #pragma omp parallel num_threads(NTHREADS)
  {
    tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(4 * KB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
}


TEST_F(THREADEDMALLOC, HbwMalloc2MB){
  #pragma omp parallel num_threads(NTHREADS)
  {
    int tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(2 * MB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
}


TEST_F(THREADEDMALLOC, HbwMalloc16MB){
  #pragma omp parallel num_threads(NTHREADS)
  {
    tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(16 * MB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
}

TEST_F(THREADEDMALLOC, HbwMalloc2GB){
  #pragma omp parallel num_threads(NTHREADS)
  {
    tid = omp_get_thread_num();
    ptr[tid] = (char *) HBW_malloc(2 * GB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    HBW_free(ptr[tid]);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}
