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
    HBW_free(ptr);
    ASSERT_EQ(HBW_SUCCESS, ret);
    
  }

};


TEST_F(MALLOCTESTS, HbwMalloc1KB){
  ptr = (char *) HBW_malloc(KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc2KB){
  ptr = (char *) HBW_malloc(2*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc4KB){
  ptr = (char *) HBW_malloc(4*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc16KB){
  ptr = (char *) HBW_malloc(16*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc256KB){
  ptr = (char *) HBW_malloc(256*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc512KB){
  ptr = (char *) HBW_malloc(512*KB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc1MB){
  ptr = (char *) HBW_malloc(1*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc2MB){
  ptr = (char *) HBW_malloc(2*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc4MB){
  ptr = (char *) HBW_malloc(4*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc16MB){
  ptr = (char *) HBW_malloc(16*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc256MB){
  ptr = (char *) HBW_malloc(256*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc512MB){
  ptr = (char *) HBW_malloc(512*MB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc1GB){
  ptr = (char *) HBW_malloc(1*GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc2GB){
  ptr = (char *) HBW_malloc(2*GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc4GB){
  ptr = (char *) HBW_malloc(4*GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

TEST_F(MALLOCTESTS, HbwMalloc8GB){
  ptr = (char *) HBW_malloc(8*GB);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
}

