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

#include "trial_generator.h"
#include "check.h"

void TrialGenerator :: generate_incremental(alloc_api_t api)
{

    size_t size[] = {2, 2*KB, 2*MB};
    size_t psize[] = {4096, 4096, 2097152};
    size_t align[] = {8, 128, 4*KB};
    int k = 0;
    trial_vec.clear();
    for (int i = 0; i< (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i],
                                               align[i], psize[i],
                                               MEMKIND_HBW,-1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(HBW_FREE,0,0,0,
                                               MEMKIND_HBW, k++));

        trial_vec.push_back(create_trial_tuple(api, size[i],
                                               align[i], psize[i],
                                               MEMKIND_HBW_PREFERRED,-1));
        k++;
        trial_vec.push_back(create_trial_tuple(HBW_FREE,0,0,0,
                                               MEMKIND_HBW_PREFERRED, k++));
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
                                               MEMKIND_DEFAULT,-1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(MEMKIND_FREE,0,0,0,
                                               MEMKIND_DEFAULT, k++));

        trial_vec.push_back(create_trial_tuple(api, size[i], 0, 0,
                                               MEMKIND_HBW,-1));
        k++;
        trial_vec.push_back(create_trial_tuple(MEMKIND_FREE,0,0,0,
                                               MEMKIND_HBW, k++));

        trial_vec.push_back(create_trial_tuple(api, size[i], 0, 0,
                                               MEMKIND_HBW_PREFERRED,-1));
        k++;
        trial_vec.push_back(create_trial_tuple(MEMKIND_FREE,0,0,0,
                                               MEMKIND_HBW_PREFERRED, k++));

    }

}

trial_t TrialGenerator :: create_trial_tuple(alloc_api_t api,
        size_t size,
        size_t alignment,
        int page_size,
        memkind_t memkind,
        int free_index)
{
    trial_t ltrial;
    ltrial.api = api;
    ltrial.size = size;
    ltrial.alignment = alignment;
    ltrial.page_size = page_size;
    ltrial.memkind = memkind;
    ltrial.free_index = free_index;
    ltrial.test = MEMALLOC;
    return ltrial;
}

void TrialGenerator :: generate_gb_misalign (alloc_api_t api,
        size_t talign)
{

    trial_vec.clear();
    trial_vec.push_back(create_trial_tuple(api,
                                           GB, talign, GB,
                                           MEMKIND_HBW_PREFERRED_GBTLB,
                                           -1));
    trial_vec.push_back(create_trial_tuple(HBW_FREE,0,0,0,
                                           MEMKIND_HBW_PREFERRED_GBTLB,
                                           0));

}


void TrialGenerator :: generate_hbw_gb_strict (alloc_api_t api)
{

    size_t size[] = {GB,(2*GB), 3*GB};
    size_t psize[] = {GB, GB, GB};
    size_t align[] = {GB, GB, GB};
    int k = 0;
    trial_vec.clear();

    for (int i = 0; i< (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i],
                                               align[i], psize[i],
                                               MEMKIND_HBW_PREFERRED_GBTLB,
                                               -1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(HBW_FREE,0,0,0,
                                               MEMKIND_HBW_PREFERRED_GBTLB,
                                               k++));
    }
}


void TrialGenerator :: generate_hbw_gb_incremental(alloc_api_t api)
{

    size_t size[] = {GB+1,(2*GB)+1};
    size_t psize[] = {GB, GB};
    size_t align[] = {GB, GB};
    int k = 0;
    trial_vec.clear();
    for (int i = 0; i< (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i],
                                               align[i], psize[i],
                                               MEMKIND_HBW_PREFERRED_GBTLB,
                                               -1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(HBW_FREE,0,0,0,
                                               MEMKIND_HBW_PREFERRED_GBTLB,
                                               k++));
    }
}

void TrialGenerator :: generate_gb_incremental(alloc_api_t api)
{

    size_t size[] = {GB+1,(2*GB)+1};
    size_t psize[] = {GB, GB};
    size_t align[] = {GB, GB};
    int k = 0;
    trial_vec.clear();
    for (int i = 0; i< (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i],
                                               align[i], psize[i],
                                               MEMKIND_HBW_GBTLB,-1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(MEMKIND_FREE,0,0,0,
                                               MEMKIND_HBW_GBTLB,
                                               k++));

    }
}

void TrialGenerator :: generate_gb_regular(alloc_api_t api)
{

    size_t size[] = {GB+1,(2*GB)+1};
    size_t psize[] = {GB, GB};
    size_t align[] = {GB, GB};
    int k = 0;
    trial_vec.clear();
    for (int i = 0; i< (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i],
                                               align[i], psize[i],
                                               MEMKIND_GBTLB,-1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(MEMKIND_FREE,0,0,0,
                                               MEMKIND_GBTLB,
                                               k++));

    }
}

int n_random(int i)
{
    return random() % i;
}

void TrialGenerator :: generate_multi_app_stress(int num_types, test_t test)
{
    int i;
    int num_trials = 1000;
    int index = 0;
    int k = 0;
    int num_alloc = 0;
    memkind_t kind;

    srandom(0);
    trial_vec.clear();
    for (i = 0; i < num_trials; i++) {
        if (n_random(3) || num_alloc == 0) {
            memkind_get_kind_by_partition(n_random(num_types), &kind);
        trial_t ltrial = create_trial_tuple(MEMKIND_MALLOC,
                                                   n_random(8*MB - 1) + 1,
                                                   0, 2097152,
                                                   kind,
                                                   k++);
        if (test == DATACHECK) ltrial.test = DATACHECK;
            trial_vec.push_back(ltrial);
            num_alloc++;
        }
        else {
            index = n_random(trial_vec.size());
            while (trial_vec[index].api == MEMKIND_FREE ||
                   trial_vec[index].free_index == -1) {
                index = n_random(trial_vec.size());
            }
            trial_vec.push_back(create_trial_tuple(MEMKIND_FREE,
                                                   0,
                                                   0, 2097152,
                                                   trial_vec[index].memkind,
                                                   trial_vec[index].free_index));
            trial_vec[index].free_index = -1;
            k++;
            num_alloc--;
        }
    }

    /* Adding free's for remaining malloc's*/
    for (i = 0; i <(int) trial_vec.size(); i++) {
        if (trial_vec[i].api != MEMKIND_FREE &&
            trial_vec[i].free_index > 0) {
            trial_t ltrial = create_trial_tuple(MEMKIND_FREE,
                                                0, 0, 2097152,
                                                trial_vec[i].memkind,
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
                                           MEMKIND_HBW,-1));
    trial_vec.push_back(create_trial_tuple(MEMKIND_FREE, 0, 0, 0,
                                           MEMKIND_HBW, 0));
    trial_vec.push_back(create_trial_tuple(api, 2*GB, 32, 2097152,
                                           MEMKIND_HBW_HUGETLB,-1));
    trial_vec.push_back(create_trial_tuple(MEMKIND_FREE, 0, 0, 2097152,
                                           MEMKIND_HBW_HUGETLB, 2));

}

void TrialGenerator :: generate_recycle_psize_incremental(alloc_api_t api)
{

    size_t size[] = {2*KB, 2*MB};

    int k = 0;
    trial_vec.clear();
    for (int i = 0; i < (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i], 32, 4096,
                                               MEMKIND_HBW,-1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(MEMKIND_FREE, 0, 0, 0,
                                               MEMKIND_HBW, k++));

        trial_vec.push_back(create_trial_tuple(api, size[i], 32, 2097152,
                                               MEMKIND_HBW_PREFERRED_HUGETLB,-1));
        k++;
        trial_vec.push_back(create_trial_tuple(MEMKIND_FREE, 0, 0, 0,
                                               MEMKIND_HBW_PREFERRED_HUGETLB, k++));
    }
}



void TrialGenerator :: generate_size_1KB_2GB(alloc_api_t api)
{

    size_t size[] = {KB, 2*KB, 4*KB, 16*KB, 256*KB,
                     512*KB, MB, 2*MB, 4*MB, 16*MB,
                     256*MB, 512*MB, GB, 2*GB
                    };

    int k = 0;
    trial_vec.clear();
    for (unsigned int i = 0; i < (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(create_trial_tuple(api,size[i],32,
                                               4096, MEMKIND_HBW,
                                               -1));
        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(HBW_FREE, 0, 0, 0,
                                               MEMKIND_HBW, k));
        k++;
    }
}

void TrialGenerator :: generate_interleave(alloc_api_t api)
{
    size_t size[] = {4*KB, 2*MB, 2*GB};
    size_t psize = 4096;
    int k = 0;
    trial_vec.clear();
    for (size_t i = 0; i < (sizeof(size)/sizeof(size[0])); i++) {
        trial_vec.push_back(create_trial_tuple(api, size[i],0, psize,
                                               MEMKIND_HBW_INTERLEAVE,-1));

        if (i > 0)
            k++;
        trial_vec.push_back(create_trial_tuple(HBW_FREE,0,0,0,
                                               MEMKIND_HBW_INTERLEAVE, k++));
    }
}

void TrialGenerator :: generate_size_2bytes_2KB_2MB(alloc_api_t api)
{
    size_t size[] = {2, 2*KB, 2*MB};

    int k = 0;
    trial_vec.clear();
    for (unsigned int i = 0; i < (int)(sizeof(size)/sizeof(size[0]));
         i++) {
        trial_vec.push_back(
          create_trial_tuple(
            api,size[i],
            32,
            4096,
            MEMKIND_HBW,
            -1
          )
        );

        if (i > 0) k++;
        trial_vec.push_back(create_trial_tuple(HBW_FREE, 0, 0, 0,
                                               MEMKIND_HBW, k));
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
                  << it->memkind <<" "
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
                    ptr_vec[i + 1] = hbw_realloc(ptr_vec[trial_vec[i].free_index], trial_vec[i + 1].size);
                    ptr_vec[trial_vec[i].free_index] = NULL;
                }
                break;
            case MEMKIND_FREE:
                if (i == num_trial - 1 || trial_vec[i + 1].api != MEMKIND_REALLOC) {
                    memkind_free(trial_vec[i].memkind,
                                 ptr_vec[trial_vec[i].free_index]);
                    ptr_vec[trial_vec[i].free_index] = NULL;
                    ptr_vec[i] = NULL;
                }
                else {
                    ptr_vec[i + 1] = memkind_realloc(trial_vec[i].memkind,
                                                     ptr_vec[trial_vec[i].free_index],
                                                     trial_vec[i + 1].size);
                    ptr_vec[trial_vec[i].free_index] = NULL;
                }
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
                ret =  hbw_posix_memalign(&ptr_vec[i],
                                          trial_vec[i].alignment,
                                          trial_vec[i].size);
                break;
            case HBW_MEMALIGN_PSIZE:
                fprintf (stdout,"Allocating %zd bytes using hbw_memalign_psize\n",
                         trial_vec[i].size);
                hbw_pagesize_t psize;
                if (trial_vec[i].page_size == 4096)
                    psize = HBW_PAGESIZE_4KB;
                else if (trial_vec[i].page_size == 2097152)
                    psize = HBW_PAGESIZE_2MB;
                else if (trial_vec[i].size %
                         trial_vec[i].page_size > 0)
                    psize = HBW_PAGESIZE_1GB;
                else
                    psize = HBW_PAGESIZE_1GB_STRICT;

                ret = hbw_posix_memalign_psize(&ptr_vec[i],
                                               trial_vec[i].alignment,
                                               trial_vec[i].size,
                                               psize);

                break;
            case MEMKIND_MALLOC:
                fprintf (stdout,"Allocating %zd bytes using memkind_malloc\n",
                             trial_vec[i].size);
                ptr_vec[i] = memkind_malloc(trial_vec[i].memkind,
                                            trial_vec[i].size);
                break;
            case MEMKIND_CALLOC:
                fprintf (stdout,"Allocating %zd bytes using memkind_calloc\n",
                         trial_vec[i].size);
                ptr_vec[i] = memkind_calloc(trial_vec[i].memkind,
                                            trial_vec[i].size, 1);
                break;
            case MEMKIND_REALLOC:
                fprintf (stdout,"Allocating %zd bytes using memkind_realloc\n",
                         trial_vec[i].size);
                if (NULL == ptr_vec[i]) {
                    ptr_vec[i] = memkind_realloc(trial_vec[i].memkind,
                                                 ptr_vec[i],
                                                 trial_vec[i].size);
                }
                break;
            case MEMKIND_POSIX_MEMALIGN:
                fprintf (stdout,
                         "Allocating %zd bytes using memkind_posix_memalign\n",
                         trial_vec[i].size);

                ret = memkind_posix_memalign(trial_vec[i].memkind,
                                             &ptr_vec[i],
                                             trial_vec[i].alignment,
                                             trial_vec[i].size);
                break;
        }
        if (trial_vec[i].api != HBW_FREE &&
            trial_vec[i].api != MEMKIND_FREE &&
            trial_vec[i].memkind != MEMKIND_DEFAULT) {
            ASSERT_TRUE(ptr_vec[i] != NULL);
            memset(ptr_vec[i], 0, trial_vec[i].size);
            Check check(ptr_vec[i], trial_vec[i]);
        if (trial_vec[i].test == DATACHECK){
            EXPECT_EQ(0, check.check_data(0x0A));
        }
            if (trial_vec[i].memkind != MEMKIND_DEFAULT &&
                trial_vec[i].memkind != MEMKIND_HUGETLB &&
                trial_vec[i].memkind != MEMKIND_GBTLB) {
                if (trial_vec[i].memkind == MEMKIND_HBW_INTERLEAVE) {
                    EXPECT_EQ(0, check.check_node_hbw_interleave(num_bandwidth, bandwidth));
                    EXPECT_EQ(0, check.check_page_size(trial_vec[i].page_size));
                } else {
                    EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
                }
            }
            if (trial_vec[i].api == HBW_CALLOC) {
                EXPECT_EQ(0, check.check_zero());
            }
            if (trial_vec[i].api == HBW_MEMALIGN ||
                trial_vec[i].api == HBW_MEMALIGN_PSIZE ||
                trial_vec[i].api == MEMKIND_POSIX_MEMALIGN) {
                EXPECT_EQ(0, check.check_align(trial_vec[i].alignment));
                EXPECT_EQ(0, ret);
            }
            if (trial_vec[i].api == HBW_MEMALIGN_PSIZE ||
                (trial_vec[i].api == MEMKIND_MALLOC &&
                 (trial_vec[i].memkind == MEMKIND_HBW_HUGETLB ||
                  trial_vec[i].memkind == MEMKIND_HBW_PREFERRED_HUGETLB))) {
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

