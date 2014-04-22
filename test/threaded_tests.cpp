/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
 */

#include "common.h"
#include "omp.h"

#define NTHREADS 2

class THREADEDMALLOC: public :: testing::Test
{
protected:
  char *ptr[NTHREADS];
  int ret;

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
  int tid;
  #pragma omp parallel num_threads(NTHREADS) private(tid)
  {
    tid = omp_get_thread_num();
    ptr[tid] = (char *) hbw_malloc(KB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    hbw_free(ptr[tid]);
  }
}


TEST_F(THREADEDMALLOC, HbwMalloc4KB){
  int tid;
  #pragma omp parallel num_threads(NTHREADS) private(tid)
  {
    tid = omp_get_thread_num();
    ptr[tid] = (char *) hbw_malloc(4 * KB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    hbw_free(ptr[tid]);
  }
}


TEST_F(THREADEDMALLOC, HbwMalloc2MB){
  int tid;
  #pragma omp parallel num_threads(NTHREADS) private(tid)
  {
    int tid = omp_get_thread_num();
    ptr[tid] = (char *) hbw_malloc(2 * MB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    hbw_free(ptr[tid]);
  }
}


TEST_F(THREADEDMALLOC, HbwMalloc16MB){
  int tid;
  #pragma omp parallel num_threads(NTHREADS) private(tid)
  {
    tid = omp_get_thread_num();
    ptr[tid] = (char *) hbw_malloc(16 * MB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    hbw_free(ptr[tid]);
  }
}

TEST_F(THREADEDMALLOC, HbwMalloc2GB){
  int tid;
  #pragma omp parallel num_threads(NTHREADS) private(tid)
  {
    tid = omp_get_thread_num();
    ptr[tid] = (char *) hbw_malloc(2 * GB);
    if (NULL == ptr[tid]){
      #pragma omp critical
      {
        ret = HBW_ERROR;
      }
    }
    ptr[tid][1024] ='a';
    hbw_free(ptr[tid]);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}
