#include "execute_trials.h"

void execute_trials(std::vector<trial_t> trial_vec, int num_bandwidth, int *bandwidth)
{
    int num_trial = trial_vec.size();
    int i;
    std::vector<void *> ptr_vec(num_trial);

    for (i = 0; i < num_trial; ++i) {
        switch(trial[i].api) {
            case FREE:
                if (i == num_trial - 1 || trial[i + 1].api != REALLOC) {
                    hbw_free(ptr_vec[trial[i].free_index]);
                    ptr_vec[trial[i].free_index] = NULL;
                }
                else {
                    ptr_vec[i] = hbw_realloc(ptr_vec[trial[i].free_index], trial[i + 1].size);
                    ptr_vec[trial[i].free_index] = NULL;
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
                ptr_vec[i] = hbw_realloc(NULL, trial[i].size);
                break;
            case MEMALIGN:
                ptr_vec[i] = hbw_allocate_memalign(ptr_vec + i, trial[i].alignment, trial[i].size);
                break;
            case MEMALIGN_PSIZE:
	        ptr_vec[i] = hbw_allocate_memalign(ptr_vec + i, trial[i].alignment, trial[i].size, trial[i].page_size);
                break;
        }
        if (trial[i].api != FREE) {
            ASSERT_TRUE(ptr_vec[i] != NULL);
            memset(ptr_vec[i], 0, trial[i].size);
            Check check(ptr_vec[i], trial[i].size);
            EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
            if (trial[i].api == CALLOC) {
                check.check_zero();
            }
            if (trail[i].api == MEMALIGN || trail[i].api == MEMALIGN_PSIZE) {
                check.check_align(trial[i].alignment);
            }
            if (trial[i].api == MEMALIGN_PSIZE) {
                check.check_page_size(trial[i].page_size);
            }
        }
    }
    for (i = 0; i < num_trial; ++i) {
        if (ptr_vec[i]) {
            hbw_free(ptr_vec[i]);
        }
    }
}
