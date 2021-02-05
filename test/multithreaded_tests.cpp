// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include "memkind.h"

#include <algorithm>
#include <fstream>

#include "check.h"
#include "common.h"
#include "trial_generator.h"
#ifdef _OPENMP
#include <omp.h>

#define NTHREADS 2

/* Multithread test content which will also use the trial_benerator to call
 * malloc, calloc, memalign, realloc, with different memory sizes but this time
 * it will use Open MP threads with NTHREADS set to 2
 */
class MultithreadedTest: public TGTest
{
};

TEST_F(MultithreadedTest,
       test_TC_MEMKIND_Multithread_HBW_Malloc_2bytes_2KB_2MB_sizes)
{
    tgen->generate_size_2bytes_2KB_2MB(HBW_MALLOC);
#pragma omp parallel num_threads(NTHREADS)
    {
        tgen->run(num_bandwidth, bandwidth);
    }
}

TEST_F(MultithreadedTest,
       test_TC_MEMKIND_Multithread_HBW_Calloc_2bytes_2KB_2MB_sizes)
{
    tgen->generate_size_2bytes_2KB_2MB(HBW_CALLOC);
#pragma omp parallel num_threads(NTHREADS)
    {
        tgen->run(num_bandwidth, bandwidth);
    }
}

TEST_F(MultithreadedTest,
       test_TC_MEMKIND_Multithread_HBW_Memalign_2bytes_2KB_2MB_sizes)
{
    tgen->generate_size_2bytes_2KB_2MB(HBW_MEMALIGN);
#pragma omp parallel num_threads(NTHREADS)
    {
        tgen->run(num_bandwidth, bandwidth);
    }
}

TEST_F(MultithreadedTest,
       test_TC_MEMKIND_Multithread_HBW_MemalignPsize_2bytes_2KB_2MB_sizes)
{
    tgen->generate_size_2bytes_2KB_2MB(HBW_MEMALIGN_PSIZE);
#pragma omp parallel num_threads(NTHREADS)
    {
        tgen->run(num_bandwidth, bandwidth);
    }
}
#endif
