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

int hbw_getpolicy(void);
void hbw_setpolicy(int mode);
int hbw_IsHBWAvailable(void);
void *hbw_malloc(size_t size);
void *hbw_calloc(size_t num, size_t size);
int hbw_allocate_memalign(void **memptr, size_t alignment, size_t size);
int hbw_allocate_memalign_psize(void **memptr, size_t alignment, size_t size,
    int pagesize);
void *hbw_realloc(void *ptr, size_t size);
void hbw_free(void *ptr);

#ifdef __cplusplus
}
#endif
#endif
