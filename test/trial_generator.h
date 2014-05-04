#ifndef execute_trials_include_h
#define execute_trials_include_h

#include <vector>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>

#include "hbwmalloc.h"
#include "numakind.h"
#include "common.h"


typedef enum {
    MALLOC,
    CALLOC,
    REALLOC,
    MEMALIGN,
    MEMALIGN_PSIZE,
    FREE,
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
    TrialGenerator(){}
    void generate_trials_incremental(alloc_api_t api);
    void generate_trials_recycle_incremental(alloc_api_t api);
    void generate_trials_recycle_psize_incremental(alloc_api_t api);
    void generate_trials_multi_api_stress(void);

    void execute_trials(int num_bandwith, int *bandwiths);
    void print_trial_list();
   private:
       std::vector<trial_t> trial_vec;
       int check_order_of_correctness();

};


#endif
