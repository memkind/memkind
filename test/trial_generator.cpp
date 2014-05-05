#include "trial_generator.h"
#include "check.h"

void TrialGenerator :: generate_trials_incremental(alloc_api_t api){

    size_t size[] = {2, 2*KB, 2*MB, 2*GB};
    int    psize[] = {4096, 2097152, 2097152,
		      2097152};

    trial_vec.clear();
    for (int i = 0; i< 4; i++){
	trial_t ltrial;
	ltrial.api = api;
	ltrial.size = size[i];
	ltrial.alignment = 32;
	ltrial.page_size = psize[i];
	ltrial.numakind = NUMAKIND_HBW;
	ltrial.free_index = -1;
	trial_vec.push_back(ltrial);
	ltrial.api = FREE;
	ltrial.size = 0;
	ltrial.alignment = 0;
	ltrial.page_size = 0;
	ltrial.numakind = NUMAKIND_HBW;
	ltrial.free_index = i;    
	trial_vec.push_back(ltrial);
    }
}


void TrialGenerator :: generate_trials_recycle_incremental(alloc_api_t api){

    size_t size[] = {2*MB, 2*GB};
    int k = 0;
    trial_vec.clear();
    for (int i = 0; i < 2; i++){
	trial_t ltrial;
	ltrial.api = api;
	ltrial.size = size[i];
	ltrial.alignment = 0;
	ltrial.page_size = 0;
	ltrial.numakind = NUMAKIND_DEFAULT;
	ltrial.free_index = -1;
	trial_vec.push_back(ltrial);
	ltrial.api = NUMAKIND_FREE;
	ltrial.size = 0;
	ltrial.alignment = 0;
	ltrial.page_size = 0;
	ltrial.numakind = NUMAKIND_DEFAULT;
	ltrial.free_index = k++;
	trial_vec.push_back(ltrial);
	ltrial.api = api;
	ltrial.size = size[i];
	ltrial.alignment = 0;
	ltrial.page_size = 0;
	ltrial.numakind = NUMAKIND_HBW;
	ltrial.free_index = -1;
	trial_vec.push_back(ltrial);
	ltrial.api = NUMAKIND_FREE;
	ltrial.size = 0;
	ltrial.alignment = 0;
	ltrial.page_size = 0;
	ltrial.numakind = NUMAKIND_HBW;
	ltrial.free_index = k++;
	trial_vec.push_back(ltrial);

    }

}

int n_random(int i) { return random() % i;}
void TrialGenerator :: generate_trials_multi_app_stress(int num_types){
    
    int i;
    int num_trials = 1000;
    int index, k = 0;
    
    srandom(0);
    trial_vec.clear();
    for (i = 0; i < num_trials; i++){

	if (n_random(3)){
	    trial_t ltrial;
	    ltrial.api = NUMAKIND_MALLOC;
	    ltrial.size = n_random(8*MB - 1) + 1;
	    ltrial.numakind = 
		(numakind_t)n_random(num_types);
	    ltrial.page_size = 2097152;
	    ltrial.alignment = 0;
	    ltrial.free_index = k++;
	    trial_vec.push_back(ltrial);
	}
	else{
	    index = n_random(trial_vec.size());
	    while (trial_vec[index].api == NUMAKIND_FREE ||
		   trial_vec[index].free_index == -1){
		index = n_random(trial_vec.size());
	    }
	    
	    trial_t ltrial;
	    ltrial.api = NUMAKIND_FREE;
	    ltrial.size = 0;
	    ltrial.numakind = trial_vec[index].numakind;
	    ltrial.page_size = 2097152;
	    ltrial.alignment = 0;
	    ltrial.free_index = trial_vec[index].free_index;
	    trial_vec[index].free_index = -1;
	    trial_vec.push_back(ltrial);
	}
    }

    /* Adding free's for remaining malloc's*/
    for (i = 0; i <(int) trial_vec.size(); i++){
	if (trial_vec[i].api != NUMAKIND_FREE &&
	    trial_vec[i].free_index > 0){
	    trial_t ltrial;
	    ltrial.api = NUMAKIND_FREE;
	    ltrial.size = 0;
	    ltrial.numakind = trial_vec[i].numakind;
	    ltrial.page_size = 0;
	    ltrial.alignment = 0;
	    ltrial.free_index = trial_vec[i].free_index;
	    trial_vec[i].free_index = -1;
	    trial_vec.push_back(ltrial);
	}
    }
}


void TrialGenerator :: generate_trials_recycle_psize_incremental(alloc_api_t api){

    size_t size[] = {2*MB, 2*GB};

    int k = 0;
    trial_vec.clear();
    for (int i = 0; i < 2; i++){
	trial_t ltrial;
	ltrial.api = api;
	ltrial.size = size[i];
	ltrial.alignment = 32;
	ltrial.page_size = 4096;
	ltrial.numakind = NUMAKIND_HBW;
	ltrial.free_index = -1;
	trial_vec.push_back(ltrial);
	ltrial.api = NUMAKIND_FREE;
	ltrial.size = 0;
	ltrial.alignment = 0;
	ltrial.page_size = 0;
	ltrial.numakind = NUMAKIND_HBW;
	ltrial.free_index = k++;
	trial_vec.push_back(ltrial);
	ltrial.api = api;
	ltrial.size = size[i];
	ltrial.alignment = 32;
	ltrial.page_size = 2097152;
	ltrial.numakind = NUMAKIND_HBW_HUGETLB;
	ltrial.free_index = -1;
	trial_vec.push_back(ltrial);
	ltrial.api = NUMAKIND_FREE;
	ltrial.size = 0;
	ltrial.alignment = 0;
	ltrial.page_size = 0;
	ltrial.numakind = NUMAKIND_HBW_HUGETLB;
	ltrial.free_index = k++;
	trial_vec.push_back(ltrial);
    }
    
}

void TrialGenerator :: generate_trials_size_1KB_2GB(alloc_api_t api){

    size_t size[] = {KB, 2*KB, 4*KB, 16*KB, 256*KB,
		     512*KB, MB, 2*MB, 4*MB, 16*MB,
		     256*MB, 512*MB, GB, 2*GB};
    size_t psize[] = {4096, 4096, 4096, 4096, 4096, 4096,
		      4096, 2097152, 2097152, 2097152, 
		      2097152, 2097152, 2097152, 2097152};

    trial_vec.clear();
    for (int i = 0; i < 14; i++){
	trial_t ltrial;
	ltrial.api = api;
	ltrial.size = size[i];
	ltrial.alignment = 32;
	ltrial.page_size = psize[i];
	ltrial.numakind = NUMAKIND_HBW;
	ltrial.free_index = -1;
	trial_vec.push_back(ltrial);
	ltrial.api = FREE;
	ltrial.size = 0;
	ltrial.alignment = 0;
	ltrial.page_size = 0;
	ltrial.numakind = NUMAKIND_HBW;
	ltrial.free_index = i;
	trial_vec.push_back(ltrial);
    }

}
void TrialGenerator :: print_trial_list(){

    std::vector<trial_t>:: iterator it;
    
    std::cout <<"*********** Size: "<< trial_vec.size()
	      <<"********\n";
    std::cout << "SIZE PSIZE ALIGN FREE KIND"<<std::endl;

    for (it = trial_vec.begin();
         it != trial_vec.end();
         it++){
	std::cout << it->size <<" "
                  << it->page_size <<" "
                  << it->alignment <<" "
                  << it->free_index <<" "
		  << it->numakind <<" " 
		  <<std::endl;
    }

}


void TrialGenerator :: execute_trials(int num_bandwidth, int *bandwidth){

    int num_trial = trial_vec.size();
    int i, ret = 0;
    void **ptr_vec = NULL;

    ptr_vec = (void **) malloc (num_trial *
				sizeof (void *));
    if (NULL == ptr_vec){
	fprintf (stderr, "Error in allocating ptr array\n");
	exit(-1);
    }
    

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
  	   case NUMAKIND_FREE:
	       numakind_free(trial_vec[i].numakind,  
			     ptr_vec[trial_vec[i].free_index]);
	       ptr_vec[trial_vec[i].free_index] = NULL;
	       ptr_vec[i] = NULL;
	       break;	    
    	   case MALLOC:
	       fprintf (stdout,"Allocating %zd bytes using hbw_malloc\n",
			trial_vec[i].size);
	       ptr_vec[i] = hbw_malloc(trial_vec[i].size);
	       break;
    	   case CALLOC:
	       fprintf (stdout,"Allocating %zd bytes using hbw_calloc\n",
			trial_vec[i].size);
	       ptr_vec[i] = hbw_calloc(trial_vec[i].size, 1);
	       break;
	   case REALLOC:
	       fprintf (stdout,"Allocating %zd bytes using hbw_realloc\n",
			trial_vec[i].size);
	       fflush(stdout);
	       ptr_vec[i] = hbw_realloc(NULL, trial_vec[i].size);
	       break;
	   case MEMALIGN:
	       fprintf (stdout,"Allocating %zd bytes using hbw_memalign\n",
			trial_vec[i].size); 
	       ret =  hbw_allocate_memalign(&ptr_vec[i], 
					    trial_vec[i].alignment,
					    trial_vec[i].size);
	       break;
           case MEMALIGN_PSIZE:
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
	      /*  	       fprintf (stdout, "Allocating %zd bytes using numakind_malloc \n",
			       trial_vec[i].size); */
	       
	       ptr_vec[i] = numakind_malloc(trial_vec[i].numakind,
					    trial_vec[i].size);
	       break;
        }
        if (trial_vec[i].api != FREE && 
	    trial_vec[i].api != NUMAKIND_FREE &&
	    trial_vec[i].numakind != NUMAKIND_DEFAULT) {

	    
            ASSERT_TRUE(ptr_vec[i] != NULL);
	    memset(ptr_vec[i], 0, trial_vec[i].size);
	    Check check(ptr_vec[i], trial_vec[i].size);
            EXPECT_EQ(0, check.check_node_hbw(num_bandwidth, bandwidth));
            if (trial_vec[i].api == CALLOC) {
                check.check_zero();
            }
            if (trial_vec[i].api == MEMALIGN || trial_vec[i].api == MEMALIGN_PSIZE) {
                check.check_align(trial_vec[i].alignment);
            }
            if (trial_vec[i].api == MEMALIGN_PSIZE ||
		(trial_vec[i].api == NUMAKIND_MALLOC &&
		 (trial_vec[i].numakind == NUMAKIND_HBW_HUGETLB ||
		  trial_vec[i].numakind == NUMAKIND_HBW_PREFERRED_HUGETLB))) {
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

