#include "trial_generator.h"
#include "check.h"

void TrialGenerator :: generate_incremental(alloc_api_t api)
{

    size_t size[] = {2, 2*KB, 2*MB, 2*GB};
    size_t psize[] = {4096, 4096, 2097152,
                      2097152
                     };
    size_t align[] = {8, 128, 4*KB, 2*MB};
    int k = 0;
    trial_vec.clear();
    for (int i = 0; i< (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i],
                                               align[i], psize[i],
                                               NUMAKIND_HBW,-1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(HBW_FREE,0,0,0,
                                               NUMAKIND_HBW, k++));

    }
}


void TrialGenerator :: generate_recycle_incremental(alloc_api_t api)
{

    size_t size[] = {2*MB, 2*GB};
    int k = 0;
    trial_vec.clear();
    for (int i = 0; i < (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i], 0, 0,
                                               NUMAKIND_DEFAULT,-1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(NUMAKIND_FREE,0,0,0,
                                               NUMAKIND_DEFAULT, k++));
        trial_vec.push_back(create_trial_tuple(api, size[i], 0, 0,
                                               NUMAKIND_HBW,-1));
        k++;
        trial_vec.push_back(create_trial_tuple(NUMAKIND_FREE,0,0,0,
                                               NUMAKIND_HBW, k++));
    }

}

trial_t TrialGenerator :: create_trial_tuple(alloc_api_t api,
        size_t size,
        size_t alignment,
        int page_size,
        numakind_t numakind,
        int free_index)
{
    trial_t ltrial;
    ltrial.api = api;
    ltrial.size = size;
    ltrial.alignment = alignment;
    ltrial.page_size = page_size;
    ltrial.numakind = numakind;
    ltrial.free_index = free_index;

    return ltrial;
}

int n_random(int i)
{
    return random() % i;
}

void TrialGenerator :: generate_multi_app_stress(int num_types)
{

    int i;
    int num_trials = 1000;
    int index, k = 0;
    int num_alloc = 0;

    srandom(0);
    trial_vec.clear();
    for (i = 0; i < num_trials; i++) {

        if (n_random(3) || num_alloc == 0) {
            trial_vec.push_back(create_trial_tuple(NUMAKIND_MALLOC,
                                                   n_random(8*MB - 1) + 1,
                                                   0, 2097152,
                                                   (numakind_t)n_random(num_types),
                                                   k++));
            num_alloc++;
        }
        else {
            index = n_random(trial_vec.size());
            while (trial_vec[index].api == NUMAKIND_FREE ||
                   trial_vec[index].free_index == -1) {
                index = n_random(trial_vec.size());
            }

            trial_vec.push_back(create_trial_tuple(NUMAKIND_FREE,
                                                   0,
                                                   0, 2097152,
                                                   trial_vec[index].numakind,
                                                   trial_vec[index].free_index));
            trial_vec[index].free_index = -1;
            k++;
            num_alloc--;
        }
    }

    /* Adding free's for remaining malloc's*/
    for (i = 0; i <(int) trial_vec.size(); i++) {
        if (trial_vec[i].api != NUMAKIND_FREE &&
            trial_vec[i].free_index > 0) {

            trial_t ltrial = create_trial_tuple(NUMAKIND_FREE,
                                                0, 0, 2097152,
                                                trial_vec[i].numakind,
                                                trial_vec[i].free_index);

            trial_vec[i].free_index = -1;
            trial_vec.push_back(ltrial);
        }
    }
}

void TrialGenerator :: generate_recycle_psize_2GB(alloc_api_t api)
{

    trial_vec.clear();
    trial_vec.push_back(create_trial_tuple(api, 2*GB, 32, 4096,
                                           NUMAKIND_HBW,-1));
    trial_vec.push_back(create_trial_tuple(NUMAKIND_FREE, 0, 0, 0,
                                           NUMAKIND_HBW, 0));
    trial_vec.push_back(create_trial_tuple(api, 2*GB, 32, 2097152,
                                           NUMAKIND_HBW_HUGETLB,-1));
    trial_vec.push_back(create_trial_tuple(NUMAKIND_FREE, 0, 0, 2097152,
                                           NUMAKIND_HBW_HUGETLB, 2));

}

void TrialGenerator :: generate_recycle_psize_incremental(alloc_api_t api)
{

    size_t size[] = {2*MB, 2*GB};

    int k = 0;
    trial_vec.clear();
    for (int i = 0; i < (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i], 32, 4096,
                                               NUMAKIND_HBW,-1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(NUMAKIND_FREE, 0, 0, 0,
                                               NUMAKIND_HBW, k++));

        trial_vec.push_back(create_trial_tuple(api, size[i], 32, 2097152,
                                               NUMAKIND_HBW_HUGETLB,-1));
        k++;
        trial_vec.push_back(create_trial_tuple(NUMAKIND_FREE, 0, 0, 0,
                                               NUMAKIND_HBW_HUGETLB, k++));
    }
}



void TrialGenerator :: generate_size_1KB_2GB(alloc_api_t api)
{

    size_t size[] = {KB, 2*KB, 4*KB, 16*KB, 256*KB,
                     512*KB, MB, 2*MB, 4*MB, 16*MB,
                     256*MB, 512*MB, GB, 2*GB, 4*GB};

    int k = 0;
    trial_vec.clear();
    for (unsigned int i = 0; i < (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api,size[i],32,
                                               4096, NUMAKIND_HBW,
                                               -1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(HBW_FREE, 0, 0, 0,
                                               NUMAKIND_HBW, k));
        k++;
    }
}

void TrialGenerator :: print()
{

    std::vector<trial_t>:: iterator it;

    std::cout <<"*********** Size: "<< trial_vec.size()
              <<"********\n";
    std::cout << "SIZE PSIZE ALIGN FREE KIND"<<std::endl;

    for (it = trial_vec.begin();
         it != trial_vec.end();
         it++) {
        std::cout << it->size <<" "
                  << it->page_size <<" "
                  << it->alignment <<" "
                  << it->free_index <<" "
                  << it->numakind <<" "
                  <<std::endl;
    }

}


void TrialGenerator :: run(int num_bandwidth, int *bandwidth)
{

    int num_trial = trial_vec.size();
    int i, ret = 0;
    void **ptr_vec = NULL;

    ptr_vec = (void **) malloc (num_trial *
                                sizeof (void *));
    if (NULL == ptr_vec) {
        fprintf (stderr, "Error in allocating ptr array\n");
        exit(-1);
    }

    for (i = 0; i < num_trial; ++i) {
        ptr_vec[i] = NULL;
    }
    for (i = 0; i < num_trial; ++i) {
        switch(trial_vec[i].api) {
            case HBW_FREE:
                if (i == num_trial - 1 || trial_vec[i + 1].api != HBW_REALLOC) {
                    hbw_free(ptr_vec[trial_vec[i].free_index]);
                    ptr_vec[trial_vec[i].free_index] = NULL;
                    ptr_vec[i] = NULL;
                }
                else {
                    ptr_vec[i] = hbw_realloc(ptr_vec[trial_vec[i].free_index], trial_vec[i + 1].size);
                    ptr_vec[trial_vec[i].free_index] = NULL;
                }
                break;
            case NUMAKIND_FREE:
                numakind_free(trial_vec[i].numakind,
                              ptr_vec[trial_vec[i].free_index]);
                ptr_vec[trial_vec[i].free_index] = NULL;
                ptr_vec[i] = NULL;
                break;
            case HBW_MALLOC:
                fprintf (stdout,"Allocating %zd bytes using hbw_malloc\n",
                         trial_vec[i].size);
                ptr_vec[i] = hbw_malloc(trial_vec[i].size);
                break;
            case HBW_CALLOC:
                fprintf (stdout,"Allocating %zd bytes using hbw_calloc\n",
                         trial_vec[i].size);
                ptr_vec[i] = hbw_calloc(trial_vec[i].size, 1);
                break;
            case HBW_REALLOC:
                fprintf (stdout,"Allocating %zd bytes using hbw_realloc\n",
                         trial_vec[i].size);
                fflush(stdout);
                if (NULL == ptr_vec[i]) {
                    ptr_vec[i] = hbw_realloc(NULL, trial_vec[i].size);
                }
                break;
            case HBW_MEMALIGN:
                fprintf (stdout,"Allocating %zd bytes using hbw_memalign\n",
                         trial_vec[i].size);
                ret =  hbw_allocate_memalign(&ptr_vec[i],
                                             trial_vec[i].alignment,
                                             trial_vec[i].size);
                break;
            case HBW_MEMALIGN_PSIZE:
                fprintf (stdout,"Allocating %zd bytes using hbw_memalign_psize\n",
                         trial_vec[i].size);
                hbw_pagesize_t psize;
                if (trial_vec[i].page_size == 4096)
                    psize = HBW_PAGESIZE_4KB;
                else
                    psize = HBW_PAGESIZE_2MB;

                ret = hbw_allocate_memalign_psize(&ptr_vec[i],
                                                  trial_vec[i].alignment,
                                                  trial_vec[i].size,
                                                  psize);

                break;

            case NUMAKIND_MALLOC:
                ptr_vec[i] = numakind_malloc(trial_vec[i].numakind,
                                             trial_vec[i].size);
                break;
        }
        if (trial_vec[i].api != HBW_FREE &&
            trial_vec[i].api != NUMAKIND_FREE &&
            trial_vec[i].numakind != NUMAKIND_DEFAULT) {


            ASSERT_TRUE(ptr_vec[i] != NULL);
            memset(ptr_vec[i], 0, trial_vec[i].size);
            Check check(ptr_vec[i], trial_vec[i].size);
            EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
            if (trial_vec[i].api == HBW_CALLOC) {
                check.check_zero();
            }
            if (trial_vec[i].api == HBW_MEMALIGN || trial_vec[i].api == HBW_MEMALIGN_PSIZE) {
                EXPECT_EQ(0,check.check_align(trial_vec[i].alignment));
            }
            if (trial_vec[i].api == HBW_MEMALIGN_PSIZE ||
                (trial_vec[i].api == NUMAKIND_MALLOC &&
                 (trial_vec[i].numakind == NUMAKIND_HBW_HUGETLB ||
                  trial_vec[i].numakind == NUMAKIND_HBW_PREFERRED_HUGETLB))) {
                EXPECT_EQ(0, check.check_page_size(trial_vec[i].page_size));
            }
        }
    }
    for (i = 0; i < num_trial; ++i) {
        if (ptr_vec[i]) {
            hbw_free(ptr_vec[i]);
        }
    }
}

