#ifndef execute_trials_include_h
#define execute_trials_include_h

typedef enum {
    MALLOC,
    CALLOC,
    REALLOC,
    MEMALIGN,
    MEMALIGN_PSIZE,
    FREE
} alloc_api_t;

typedef struct {
    alloc_api_t api;
    size_t size;
    size_t alignment;
    size_t page_size;
    size_t free_index;
} trial_t;

class TrialGenerator
{
   public:
       TrialGenerator();
       void generate_trials_incremental(alloc_api_t api);
       void generate_trials_multi_api_stress(void);
       void execute_trials(int num_bandwith, int *bandwiths);
   private:
       std::vector<trial_t> trial_vec;
}


#endif
