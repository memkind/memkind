#ifndef hbw_malloc_include_h
#define hbw_malloc_include_h
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HBW_POLICY_BIND = 1,
    HBW_POLICY_PREFERRED = 2
} hbw_policy_t;

typedef enum {
    HBW_PAGESIZE_4KB = 1,
    HBW_PAGESIZE_2MB = 2
} hbw_pagesize_t;

int HBW_getpolicy(void);
void HBW_setpolicy(int mode);
int HBW_IsHBWAvailable(void);
void *HBW_malloc(size_t size);
void *HBW_calloc(size_t num, size_t size);
int HBW_allocate_memalign(void **memptr, size_t alignment, size_t size);
int HBW_allocate_memalign_psize(void **memptr, size_t alignment, size_t size,
    int pagesize);
void *HBW_realloc(void *ptr, size_t size);
void HBW_free(void *ptr);

#ifdef __cplusplus
}
#endif
#endif
