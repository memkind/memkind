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

void execute_trials(std::vector<trial_t> trial_vec);

#endif
