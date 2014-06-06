#ifndef execute_trials_include_h
#define execute_trials_include_h

#include <vector>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>
#include <iostream>

#include "hbwmalloc.h"
#include "numakind.h"
#include "common.h"


typedef enum {
    HBW_MALLOC,
    HBW_CALLOC,
    HBW_REALLOC,
    HBW_MEMALIGN,
    HBW_MEMALIGN_PSIZE,
    HBW_FREE,
    NUMAKIND_MALLOC,
    NUMAKIND_FREE
} alloc_api_t;

typedef struct {
    alloc_api_t api;
    size_t size;
    size_t alignment;
    int page_size;
    numakind_t numakind;
    int free_index;
} trial_t;

class TrialGenerator
{
public:
    TrialGenerator() {}
    void generate_incremental(alloc_api_t api);
    void generate_recycle_incremental(alloc_api_t api);
    void generate_recycle_psize_incremental(alloc_api_t api);
    void generate_recycle_psize_2GB(alloc_api_t api);
    void generate_multi_app_stress(int num_types);
    void generate_size_1KB_2GB(alloc_api_t api);
    void generate_size_4GB_8GB(alloc_api_t api);
    void run(int num_bandwidth, int *bandwidths);
    /*For debugging purposes*/
    void print();
private:
    std::vector<trial_t> trial_vec;
    trial_t create_trial_tuple(alloc_api_t api,
                               size_t size,
                               size_t alignment,
                               int page_size,
                               numakind_t numakind,
                               int free_index);


};


#endif
