#include <stdlib.h>
#include <vector>
#include "hbwmalloc.h"
#include "trial_generator.h"


void TrialGenerator::execute_trials(int num_bandwidth, int *bandwidth)
{
    int num_trial = trial_vec.size();
    int i;
    std::vector<void *> ptr_vec(num_trial);

    for (i = 0; i < num_trial; ++i) {
        switch(trial_vec[i].api) {
            case FREE:
                if (i == num_trial - 1 || trial_vec[i + 1].api != REALLOC) {
                    hbw_free(ptr_vec[trial_vec[i].free_index]);
                    ptr_vec[trial_vec[i].free_index] = NULL;
                    ptr_vec[i] = NULL;
                }
                else {
                    ptr_vec[i] = hbw_realloc(ptr_vec[trial_vec[i].free_index], trial_vec[i + 1].size);
                    ptr_vec[trial_vec[i].free_index] = NULL;
                    ++i;
                }
                break;
            case MALLOC:
                ptr_vec[i] = hbw_malloc(trial_vec[i].size);
                break;
            case CALLOC:
                ptr_vec[i] = hbw_calloc(trial_vec[i].size, 1);
                break;
            case REALLOC:
                ptr_vec[i] = hbw_realloc(NULL, trial_vec[i].size);
                break;
            case MEMALIGN:
	      ptr_vec[i] = hbw_allocate_memalign(std::addressof(ptr_vec[i]), trial_vec[i].alignment, trial_vec[i].size);
                break;
            case MEMALIGN_PSIZE:
	        ptr_vec[i] = hbw_allocate_memalign_psize(std::addressof(ptr_vec[i]), trial_vec[i].alignment, trial_vec[i].size, trial_vec[i].page_size);
                break;
        }
        if (trial_vec[i].api != FREE) {
            ASSERT_TRUE(ptr_vec[i] != NULL);
            memset(ptr_vec[i], 0, trial_vec[i].size);
            Check check(ptr_vec[i], trial_vec[i].size);
            EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
            if (trial_vec[i].api == CALLOC) {
                check.check_zero();
            }
            if (trail[i].api == MEMALIGN || trail[i].api == MEMALIGN_PSIZE) {
                check.check_align(trial_vec[i].alignment);
            }
            if (trial_vec[i].api == MEMALIGN_PSIZE) {
                check.check_page_size(trial_vec[i].page_size);
            }
        }
    }
    for (i = 0; i < num_trial; ++i) {
        if (ptr_vec[i]) {
            hbw_free(ptr_vec[i]);
        }
    }
}
