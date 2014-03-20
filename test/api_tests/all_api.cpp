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

TEST_F(APITESTS,HbwExistsTest){
  
  int ret = HBW_SUCCESS;
  ret = hbw_IsHBWAvailable();
  ASSERT_EQ(1,ret);
 
}

TEST_F(APITESTS,HbwSetGetPolicyPreferred){
  
  hbw_setpolicy(2);
  ASSERT_EQ(2, hbw_getpolicy());
}

TEST_F(APITESTS,HbwSetGetPolicyBind){

  hbw_setpolicy(1);
  ASSERT_EQ(1, hbw_getpolicy());
}



TEST_F(APITESTS, HbwMalloc2GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  size_t size = (size_t)(2048*MB);
  ptr = (char *) hbw_malloc(size);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  /*Lets touch a part of this memory*/
  ptr[1024] ='a';

  if (NULL != ptr){
    hbw_free(ptr);
  }

  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwCalloc2GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  size_t size = (size_t)(2*GB);
  ptr = (char *) hbw_calloc(size,1);
  if (NULL == ptr){
    ret = HBW_ERROR;
  }
  /*Lets touch a part of this memory*/
  ptr[1024] ='a';
  
  if (NULL != ptr){
    hbw_free(ptr);
  }

  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwRealloc2GB){
  char *ptr = NULL;
  int ret = HBW_SUCCESS;
  size_t size = (size_t)(2*GB);
  ptr = (char *) hbw_realloc(ptr, size);
  
  if (NULL == ptr){
    ret = HBW_ERROR;
  }

  /*Lets touch a part of this memory*/
  ptr[1024] ='a';
  
  if (NULL != ptr){
    hbw_free(ptr);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwAllocateMemAlign2GB){
  
  void *ptr = NULL;
  int ret = HBW_SUCCESS, fret=0;
  size_t size = (size_t)(2*GB);
  size_t align = 32;
  char *tmp = NULL;

  fret = hbw_allocate_memalign(&ptr,align,size);

  if (fret != HBW_SUCCESS
      || ((size_t)ptr%align != 0) 
      || (NULL == ptr)){
    ret = HBW_ERROR;
  }
  tmp = (char *)ptr;
  tmp[1024] ='a';
  if (NULL != ptr){
    hbw_free(ptr);
  }
  
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwAllocateMemAlignPsize4K){
  
  void *ptr = NULL;
  int ret = HBW_SUCCESS, fret=0;
  size_t size = (size_t)(16*MB);
  char *tmp = NULL;
  size_t align = 4*KB;
  hbw_pagesize_t psize = hbw_4k_page;

  fret = hbw_allocate_memalign_psize(&ptr,align,size,psize);
  
  tmp = (char *)ptr;
  tmp[1024] = 'a';

  if (fret != HBW_SUCCESS
      || (NULL == ptr)){
    ret = HBW_ERROR;
  }
  

  if (NULL != ptr){
    hbw_free(ptr);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}

TEST_F(APITESTS, HbwAllocateMemAlignPsize2M){
  
  void *ptr = NULL;
  int ret = HBW_SUCCESS, fret=0;
  size_t size = (size_t)(16*MB);
  char *tmp = NULL;
  /*These two values need to be udpdate
   based on the page sizes set in /proc/cmdline
  */
  size_t align = 2*MB;
  hbw_pagesize_t psize = hbw_2m_page;

  fret = hbw_allocate_memalign_psize(&ptr,align,size,psize);
  
  tmp = (char *)ptr;
  tmp[1024] = 'a';

  if (fret != HBW_SUCCESS
      || (NULL == ptr)){
    ret = HBW_ERROR;
  }
  

  if (NULL != ptr){
    hbw_free(ptr);
  }
  ASSERT_EQ(HBW_SUCCESS, ret);
}


