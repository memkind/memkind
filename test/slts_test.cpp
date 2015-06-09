/*
 * Copyright (C) 2014, 2015 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "common.h"
#include "memkind.h"
#include "hbwmalloc.h"
#include "memkind_gbtlb.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

static int v_lvl;
static int l_flag;

bool              t_done;
size_t            duration;
long unsigned int d_fail;
long unsigned int d_pass;
long unsigned int n_fail;
long unsigned int n_pass;
long unsigned int n_alloc;
long unsigned int n_avail;

typedef enum{
  MALLOC,
  CALLOC,
  REALLOC,
  GBTLBMALLOC,
  GBTLBCALLOC,
  GBTLBREALLOC,
  MEMALIGN
}testop;

typedef struct{
  testop op;
  std :: string name;
  int (*fp)();
}test_t;

std :: vector<test_t> test_vec;

memkind_t randomBufferKind()
{
    switch(rand() % 10)
    {
        case 1:
            return MEMKIND_DEFAULT;
        case 2:
	    return MEMKIND_HUGETLB;
        case 3:
	    return MEMKIND_HBW;
        case 4:
	    return MEMKIND_HBW_PREFERRED;
        case 5:
	    return MEMKIND_HBW_HUGETLB;
        case 6:
	    return MEMKIND_HBW_PREFERRED_HUGETLB;
        case 7:
	    return MEMKIND_HBW_GBTLB;
        case 8:
	    return MEMKIND_HBW_PREFERRED_GBTLB;
        case 9:
	    return MEMKIND_GBTLB;
    }
    return MEMKIND_DEFAULT;
}

void waitForDuration()
{
    struct timeval final;
    struct timeval current;

    sleep(1);
    gettimeofday(&current, NULL);
    final.tv_sec = current.tv_sec + duration;
    do{
        gettimeofday(&current, NULL);
    }while(current.tv_sec < final.tv_sec);
    t_done = true;
}

const char* getKindName(memkind_t kind)
{
    if(kind == MEMKIND_DEFAULT)
        return "memkind_default";
    if(kind == MEMKIND_HUGETLB)
        return "memkind_hugetlb";
    if(kind == MEMKIND_HBW)
        return "memkind_hbw";
    if(kind == MEMKIND_HBW_PREFERRED)
        return "memkind_hbw_preferred";
    if(kind == MEMKIND_HBW_HUGETLB)
        return "memkind_hbw_hugetlb";
    if(kind == MEMKIND_HBW_PREFERRED_HUGETLB)
        return "memkind_hbw_preferred_hugetlb";
    if(kind == MEMKIND_HBW_GBTLB)
        return "memkind_hbw_gbtlb";
    if(kind == MEMKIND_HBW_PREFERRED_GBTLB)
        return "memkind_hbw_preferred_gbtlb";
    if(kind == MEMKIND_GBTLB)
        return "memkind_gbtlb";

  return "Not defined";
}

size_t randomBufferSize(memkind_t kind)
{
    size_t free;
    size_t total;

    if(memkind_check_available(kind) == 0)
    {
        memkind_get_size(kind, &total, &free);
        if (kind == MEMKIND_GBTLB ||
            kind == MEMKIND_HBW_GBTLB ||
	    kind == MEMKIND_HBW_PREFERRED_GBTLB)
        {
            if (free < GB) return (size_t)-1;
	    return (size_t)(((rand() % free) % ((free - GB) + 1)) + GB);
        }else{
	  if (free > 0) return (size_t)(rand() % free);
        }
    }
    return -1; /* Not available memory for this kind */
}

int dataCheck(void *ptr, size_t size)
{
    size_t i = 0;
    char  *p = (char*)ptr;

    memset(p, 1, size);
    for(i = 0; i < size; i++)
    {
        if (p[i] != 1) return -1;
    }

    return 0;
}

void *mem_allocations(void* arg)
{
    void       *p   = NULL;
    size_t     size = 0;
    static int op   = (intptr_t)(arg);
    memkind_t  kind = MEMKIND_DEFAULT;

    while(!t_done)
    {
        kind = randomBufferKind();
	size = randomBufferSize(kind);
	if(size == (size_t)-1)
	{
	    n_avail++;
	    if (v_lvl == 2)
	        fprintf(stderr, "No memmory available %zd for %s\n",
		        size, getKindName(kind));
	}else{
            n_alloc++;
            switch(op)
	    {
                case MALLOC:
	            p = memkind_malloc(kind, size);
		    break;
	        case CALLOC:
		    p = memkind_calloc(kind, 1, size);
		    break;
                case REALLOC:
	            p = memkind_malloc(kind, size);
		    p = memkind_realloc(kind, p, randomBufferSize(kind));
		    break;
	        case GBTLBMALLOC:
		    p = memkind_gbtlb_malloc(kind,size);
		    break;
	        case GBTLBCALLOC:
		    p = memkind_gbtlb_calloc(kind, 1, size);
		    break;
	        case GBTLBREALLOC:
		    p = memkind_gbtlb_malloc(kind,size);
		    p = memkind_gbtlb_realloc(kind, p, randomBufferSize(kind));
		    break;
	        case MEMALIGN:
		    memkind_posix_memalign(kind, &p, 1024, size);
		    break;
	    }
	    if (p == NULL){
                n_fail++;
	        if (v_lvl == 1) fprintf(stdout,"Alloc: %zd of %s: Failed\n",
					size, getKindName(kind));
	    }else{
	        n_pass++;
	        if (v_lvl == 1) fprintf(stdout,"Alloc: %zd of %s: Passed - DataCheck: ",
					size, getKindName(kind));

		if (dataCheck(p, size) == 0){
                    d_pass++;
	            if (v_lvl == 1) fprintf(stdout,"Passed\n");
		}else{
		    d_fail++;
		    if (v_lvl == 1) fprintf(stdout,"Failed\n");
		}
		switch(op)
	        {
	            case GBTLBMALLOC:
	            case GBTLBCALLOC:
                    case GBTLBREALLOC:
	                memkind_gbtlb_free(kind, p);
			break;
                    default:
		        memkind_free(kind, p);
			break;
		}
	    }
	}
    }

    return 0;
}

void init_t()
{
   t_done  = false;
   n_fail  = 0;
   n_pass  = 0;
   d_fail  = 0;
   d_pass  = 0;
   n_alloc = 0;
   n_avail = 0;
}

void printResults(std::string name)
{
  printf("************** Execution Summary *********************\n");
  printf("API exercised             : %s\n", name.c_str());
  printf("Allocation failures       : %ld\n", n_fail);
  printf("Allocation passes         : %ld\n", n_pass);
  printf("Data check failures       : %ld\n", d_fail);
  printf("Data check passes         : %ld\n", d_pass);
  printf("Memory not available      : %ld\n", n_avail);
  printf("Total of alloc trial      : %ld\n", n_alloc);
  printf("*****************************************************\n");
}

int TestMalloc()
{
    pthread_t t;
    init_t();
    pthread_create(&t, NULL, mem_allocations, (void*)MALLOC);
    waitForDuration();
    printResults("memkind_malloc()");
    return 0;
}
int TestCalloc()
{
    pthread_t t;
    init_t();
    pthread_create(&t, NULL, mem_allocations, (void*)CALLOC);
    waitForDuration();
    printResults("memkind_calloc()");
    return 0;
}
int TestRealloc()
{
    pthread_t t;
    init_t();
    pthread_create(&t,NULL,mem_allocations,(void*)REALLOC);
    waitForDuration();
    printResults("memkind_realloc()");
    return 0;
}

int TestGBTLBMalloc()
{
    pthread_t t;
    init_t();
    pthread_create(&t,NULL,mem_allocations,(void*)GBTLBMALLOC);
    waitForDuration();
    printResults("memkind_gbtlb_malloc()");
    return 0;
}

int TestGBTLBCalloc()
{
    pthread_t t;
    init_t();
    pthread_create(&t,NULL,mem_allocations,(void*)GBTLBCALLOC);
    waitForDuration();
    printResults("memkind_gbtlb_calloc()");
    return 0;
}

int TestGBTLBRealloc()
{
    pthread_t t;
    init_t();
    pthread_create(&t,NULL,mem_allocations,(void*)GBTLBREALLOC);
    waitForDuration();
    printResults("memkind_gbtlb_realloc()");
    return 0;
}

int TestPosixMemAlign()
{
    pthread_t t;
    init_t();
    pthread_create(&t,NULL,mem_allocations,(void*)MEMALIGN);
    waitForDuration();
    printResults("memkind_posix_memalign()");
    return 0;
}

test_t create_test(testop op, std::string name,  int (*fp)())
{
  test_t t;
  t.op = op;
  t.name = name;
  t.fp = fp;

  return t;
}

void init()
{
    /* Default test duration = 1 minute */
    duration = 60 * 1;

    test_vec.clear();
    test_vec.push_back(create_test(MALLOC,
	                           "TC_Memkind_Malloc_slts",
		                   &TestMalloc));
    test_vec.push_back(create_test(CALLOC,
                                   "TC_Memkind_Calloc_slts",
			           &TestCalloc));
    test_vec.push_back(create_test(REALLOC,
                                   "TC_Memkind_Realloc_slts",
			           &TestRealloc));
    test_vec.push_back(create_test(GBTLBMALLOC,
                                   "TC_Memkind_GBTLB_Malloc_slts",
			           &TestGBTLBMalloc));
    test_vec.push_back(create_test(GBTLBCALLOC,
                                   "TC_Memkind_GBTLB_Calloc_slts",
			           &TestGBTLBCalloc));
    test_vec.push_back(create_test(GBTLBREALLOC,
				   "TC_Memkind_GBTLB_Realloc_slts",
				   &TestGBTLBRealloc));
    test_vec.push_back(create_test(MEMALIGN,
				   "TC_Memkind_Posix_MemAlign_slts",
				   &TestPosixMemAlign));
}

int runAll()
{
   int i;

   for (i = 0; i < (int)test_vec.size(); i++)
   {
     fprintf(stdout, "Test in progress...\n");
     test_vec[i].fp();
   }

   return 0;
}

void PrintList()
{
   int i;
   int n_test = test_vec.size();

   for (i = 0; i < n_test; i++)
   {
       fprintf(stdout,"%s\n",test_vec[i].name.c_str());
   }

    exit(0);
}

int runSingle(std::string test)
{
   int i;
   int n_test = test_vec.size();

   for (i = 0; i < n_test; i++)
   {
       if (test_vec[i].name == test)
       {
           fprintf(stdout,"Test in progress...\n");
           test_vec[i].fp();
       }
   }

   exit(0);
}

void usage()
{
   printf("Usage:                                              \n");
   printf("slts_tests <options> <args>                         \n");
   printf("Options & args:                                     \n");
   printf("    --filter       Run the selected test            \n");
   printf("    --list         List all the test availables     \n");
   printf("    --duration     Sets the time duration per test  \n");
   printf("    --verbose      Enables stdout=1 and stderr=2    \n");
   printf("    --help         Prints this help message         \n");
   exit(0);
}

int main(int argc, char **argv)
{
    int c;
    std::string test_name;

    init();
    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
	    {"list",       no_argument,       &l_flag,  1 },
	    {"verbose",    required_argument, 0,       'v'},
	    {"filter",     required_argument, 0,       'f'},
	    {"duration",   required_argument, 0,       'd'},
	    {"help",       required_argument, 0,       'h'},
	    {0,            0,                 0,        0 }
	};

	/* getopt_long stores the option index here. */
	int option_index = 0;
	c = getopt_long (argc, argv, "f:d:h:v:",
			 long_options, &option_index);

	/* Detect the end of the options. */
	if (c == -1)
	    break;

	/* Print list of test cases */
	if (l_flag) PrintList();

	switch (c)
	{
            case 'f':
		test_name = optarg;
		break;
            case 'd':
	        duration = 60 * strtol(optarg, NULL, 10);
		break;
            case 'v':
	        v_lvl = strtol(optarg, NULL, 10);
		break;
            case 'h':
               usage();
	       break;
            case '?':
	       fprintf(stderr,"missing argument\n");
               usage();
               break;
	}
    }

    if (!test_name.empty())
        runSingle(test_name);
    else
        runAll();

    return 0;
}
