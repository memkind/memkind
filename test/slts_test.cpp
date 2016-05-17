/*
 * Copyright (C) 2014 - 2016 Intel Corporation.
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#include "common.h"
#include <memkind.h>
#include <hbwmalloc.h>
#include <memkind/internal/memkind_gbtlb.h>

static int v_lvl;
static int l_flag;

typedef enum test_operations {
    MALLOC,
    CALLOC,
    REALLOC,
    GBTLBMALLOC,
    GBTLBCALLOC,
    GBTLBREALLOC,
    MEMALIGN
} test_operations;

typedef struct test_summary {
    long unsigned int d_fail;
    long unsigned int d_pass;
    long unsigned int n_fail;
    long unsigned int n_pass;
    long unsigned int n_alloc;
    long unsigned int n_avail;
} test_summary;

typedef struct test_type {
    test_operations op;
    std :: string name;
    int (*fp)(size_t);
} test_type;

std :: vector<test_type> test_vec;

typedef struct thread_args {
    test_operations op;
    size_t time;
    test_summary res;
} thread_args;

memkind_t random_buffer_kind()
{
    switch(rand() % 10) {
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

const char* get_kind_name(memkind_t kind)
{
    if (kind == MEMKIND_DEFAULT)
        return "memkind_default";
    if (kind == MEMKIND_HUGETLB)
        return "memkind_hugetlb";
    if (kind == MEMKIND_HBW)
        return "memkind_hbw";
    if (kind == MEMKIND_HBW_PREFERRED)
        return "memkind_hbw_preferred";
    if (kind == MEMKIND_HBW_HUGETLB)
        return "memkind_hbw_hugetlb";
    if (kind == MEMKIND_HBW_PREFERRED_HUGETLB)
        return "memkind_hbw_preferred_hugetlb";
    if (kind == MEMKIND_HBW_GBTLB)
        return "memkind_hbw_gbtlb";
    if (kind == MEMKIND_HBW_PREFERRED_GBTLB)
        return "memkind_hbw_preferred_gbtlb";
    if (kind == MEMKIND_GBTLB)
        return "memkind_gbtlb";

    return "Not defined";
}

size_t random_buffer_size(memkind_t kind)
{
    size_t free;
    size_t total;

    if (memkind_check_available(kind) == 0) {
        memkind_get_size(kind, &total, &free);
        if (kind == MEMKIND_GBTLB ||
            kind == MEMKIND_HBW_GBTLB ||
            kind == MEMKIND_HBW_PREFERRED_GBTLB) {
            if (free < GB) return (size_t)-1;
            return (size_t)(((rand() % free) % ((free - GB) + 1)) + GB);
        }
        else {
            if (free > 0) return (size_t)(rand() % free);
        }
    }
    return -1; /* Not available memory for this kind */
}

int data_check(void *ptr, size_t size)
{
    size_t i = 0;
    char  *p = (char*)ptr;

    memset(p, 0x0A, size);
    for(i = 0; i < size; i++)
        if (p[i] != 0x0A) return -1;

    return 0;
}

void *mem_allocations(void* t_args)
{
    void *p = NULL;
    size_t size = 0;
    thread_args *args = (thread_args*)t_args;
    memkind_t kind = MEMKIND_DEFAULT;
    struct timeval current;
    struct timeval final;

    gettimeofday(&current, NULL);
    final.tv_sec = current.tv_sec + args->time;
    do {
        kind = random_buffer_kind();
        size = random_buffer_size(kind);
        if (size == (size_t)(-1)) {
            args->res.n_avail++;
            if (v_lvl == 2)
                fprintf(stderr, "No memmory available %zd for %s\n",
                        size, get_kind_name(kind));
        }
        else {
            args->res.n_alloc++;
            switch (args->op) {
                case MALLOC:
                    p = memkind_malloc(kind, size);
                    break;
                case CALLOC:
                    p = memkind_calloc(kind, 1, size);
                    break;
                case REALLOC:
                    p = memkind_malloc(kind, size);
                    p = memkind_realloc(kind, p, random_buffer_size(kind));
                    break;
                case GBTLBMALLOC:
                    p = memkind_gbtlb_malloc(kind,size);
                    break;
                case GBTLBCALLOC:
                    p = memkind_gbtlb_calloc(kind, 1, size);
                    break;
                case GBTLBREALLOC:
                    p = memkind_gbtlb_malloc(kind,size);
                    p = memkind_gbtlb_realloc(kind, p, random_buffer_size(kind));
                    break;
                case MEMALIGN:
                    memkind_posix_memalign(kind, &p, 1024, size);
                    break;
            }
            if (p == NULL) {
                args->res.n_fail++;
                if (v_lvl == 1) fprintf(stdout,"Alloc: %zd of %s: Failed\n",
                                            size, get_kind_name(kind));
            }
            else {
                args->res.n_pass++;
                if (v_lvl == 1) fprintf(stdout,"Alloc: %zd of %s: Passed - DataCheck: ",
                                            size, get_kind_name(kind));

                if (data_check(p, size) == 0) {
                    args->res.d_pass++;
                    if (v_lvl == 1) fprintf(stdout,"Passed\n");
                }
                else {
                    args->res.d_fail++;
                    if (v_lvl == 1) fprintf(stdout,"Failed\n");
                }
                switch (args->op) {
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
        gettimeofday(&current, NULL);
    }
    while(current.tv_sec < final.tv_sec);
    return 0;
}

void print_results(std::string name, test_summary test_results)
{
    test_summary res = test_results;

    printf("************** Execution Summary *********************\n");
    printf("API exercised             : %s\n", name.c_str());
    printf("Allocation failures       : %ld\n", res.n_fail);
    printf("Allocation passes         : %ld\n", res.n_pass);
    printf("Data check failures       : %ld\n", res.d_fail);
    printf("Data check passes         : %ld\n", res.d_pass);
    printf("Memory not available      : %ld\n", res.n_avail);
    printf("Total of alloc trial      : %ld\n", res.n_alloc);
    printf("*****************************************************\n");
}

void test(test_operations op, size_t duration, std::string name)
{
    pthread_t t;
    thread_args args;
    test_summary results = {};

    args.op = op;
    args.time = duration;
    args.res = results;

    pthread_create(&t, NULL, mem_allocations, (void*)&args);
    pthread_join(t, NULL);
    print_results(name, args.res);
}

int test_malloc(size_t duration)
{
    test(MALLOC, duration, "memkind_malloc()");
    return 0;
}
int test_calloc(size_t duration)
{
    test(CALLOC, duration, "memkind_calloc()");
    return 0;
}
int test_realloc(size_t duration)
{
    test(REALLOC, duration, "memkind_realloc()");
    return 0;
}

int test_gbtlb_malloc(size_t duration)
{
    test(GBTLBMALLOC, duration, "memkind_gbtlb_malloc()");
    return 0;
}

int test_gbtlb_calloc(size_t duration)
{
    test(GBTLBCALLOC, duration, "memkind_gbtlb_calloc()");
    return 0;
}

int test_gbtlb_realloc(size_t duration)
{
    test(GBTLBREALLOC, duration, "memkind_gbtlb_realloc()");
    return 0;
}

int test_posix_memalign(size_t duration)
{
    test(MEMALIGN, duration, "memkind_posix_memalign()");
    return 0;
}

test_type create_test(test_operations op, std::string name,  int (*fp)(size_t))
{
    test_type t;

    t.op = op;
    t.name = name;
    t.fp = fp;

    return t;
}

void init()
{
    srand(time(NULL));
    test_vec.clear();
    test_vec.push_back(create_test(MALLOC,
                                   "TC_Memkind_Malloc_slts",
                                   &test_malloc));
    test_vec.push_back(create_test(CALLOC,
                                   "TC_Memkind_Calloc_slts",
                                   &test_calloc));
    test_vec.push_back(create_test(REALLOC,
                                   "TC_Memkind_Realloc_slts",
                                   &test_realloc));
    test_vec.push_back(create_test(GBTLBMALLOC,
                                   "TC_Memkind_GBTLB_Malloc_slts",
                                   &test_gbtlb_malloc));
    test_vec.push_back(create_test(GBTLBCALLOC,
                                   "TC_Memkind_GBTLB_Calloc_slts",
                                   &test_gbtlb_calloc));
    test_vec.push_back(create_test(GBTLBREALLOC,
                                   "TC_Memkind_GBTLB_Realloc_slts",
                                   &test_gbtlb_realloc));
    test_vec.push_back(create_test(MEMALIGN,
                                   "TC_Memkind_Posix_MemAlign_slts",
                                   &test_posix_memalign));
}

void print_list()
{
    size_t i;

    for (i = 0; i < test_vec.size(); i++) {
        fprintf(stdout,"%s\n",test_vec[i].name.c_str());
    }

    exit(0);
}

int run_all(size_t duration)
{
    size_t i;

    for (i = 0; i < test_vec.size(); i++) {
        fprintf(stdout, "Test in progress...\n");
        test_vec[i].fp(duration);
    }

    return 0;
}

int run_single(std::string test, size_t duration)
{
    size_t i;

    for (i = 0; i < test_vec.size(); i++) {
        if (test_vec[i].name == test) {
            fprintf(stdout,"Test in progress...\n");
            test_vec[i].fp(duration);
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
    size_t duration = 60 * 1; // Default to 1 minute

    init();
    while (1) {
        static struct option long_options[] = {
            /* These options set a flag. */
            {"list",       no_argument,       &l_flag,  1 },
            {"verbose",    required_argument, 0,       'v'},
            {"filter",     required_argument, 0,       'f'},
            {"duration",   required_argument, 0,       'd'},
            {"help",       no_argument,       0,       'h'},
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
        if (l_flag) print_list();

        switch (c) {
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
        run_single(test_name, duration);
    else
        run_all(duration);

    return 0;
}
