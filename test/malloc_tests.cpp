/* FILE LICENSE TAG: intel */
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

class MALLOCTESTS: public :: testing::Test
{
  
protected:
  char *ptr;
  int ret;
   
  MALLOCTESTS()
  {}
  
  void SetUp()
  {
    ptr = NULL;
    ret = HBW_SUCCESS;
  }
  
  void TearDown()
  {
    
    ptr[1024] ='a';
    hbw_free(ptr);
    ASSERT_EQ(HBW_SUCCESS, ret);
    
  }

};


TEST_F(MALLOCTESTS, HbwMalloc1KB){
  ptr = (char *) hbw_malloc(KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc2KB){
  ptr = (char *) hbw_malloc(2*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc4KB){
  ptr = (char *) hbw_malloc(4*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc16KB){
  ptr = (char *) hbw_malloc(16*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc256KB){
  ptr = (char *) hbw_malloc(256*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc512KB){
  ptr = (char *) hbw_malloc(512*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc1MB){
  ptr = (char *) hbw_malloc(1*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc2MB){
  ptr = (char *) hbw_malloc(2*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc4MB){
  ptr = (char *) hbw_malloc(4*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc16MB){
  ptr = (char *) hbw_malloc(16*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc256MB){
  ptr = (char *) hbw_malloc(256*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc512MB){
  ptr = (char *) hbw_malloc(512*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc1GB){
  ptr = (char *) hbw_malloc(1*GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc2GB){
  ptr = (char *) hbw_malloc(2*GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc4GB){
  ptr = (char *) hbw_malloc(4*GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc8GB){
  ptr = (char *) hbw_malloc(8*GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

