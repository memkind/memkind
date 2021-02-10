// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#ifndef execute_trials_include_h
#define execute_trials_include_h

#include "hbwmalloc.h"
#include "memkind.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <vector>

#include "common.h"

typedef enum
{
    HBW_MALLOC,
    HBW_CALLOC,
    HBW_REALLOC,
    HBW_MEMALIGN,
    HBW_MEMALIGN_PSIZE,
    HBW_FREE,
    MEMKIND_MALLOC,
    MEMKIND_CALLOC,
    MEMKIND_REALLOC,
    MEMKIND_POSIX_MEMALIGN,
    MEMKIND_FREE
} alloc_api_t;

typedef struct {
    alloc_api_t api;
    size_t size;
    size_t alignment;
    size_t page_size;
    memkind_t memkind;
    int free_index;
} trial_t;

class TrialGenerator
{
public:
    TrialGenerator()
    {}
    void generate_gb(alloc_api_t api, int number_of_gb_pages, memkind_t memkind,
                     alloc_api_t api_free, bool psize_strict = false,
                     size_t align = GB);
    void run(int num_bandwidth, std::vector<int> &bandwidths);
    void generate_size_2bytes_2KB_2MB(alloc_api_t api);
    /*For debugging purposes*/
    void print();

private:
    std::vector<trial_t> trial_vec;
    trial_t create_trial_tuple(alloc_api_t api, size_t size, size_t alignment,
                               int page_size, memkind_t memkind,
                               int free_index);
};

class TGTest: public ::testing::Test
{
protected:
    size_t num_bandwidth;
    std::vector<int> bandwidth;
    std::unique_ptr<TrialGenerator> tgen;
    void SetUp();
    void TearDown();
};

#endif
