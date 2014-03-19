#ifndef hbw_malloc_include_h
#define hbw_malloc_include_h

#include "numakind.h"
#include "pthread.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline int hbw_policy(int mode)
{
    static pthread_mutex_t policy_mutex = PTHREAD_MUTEX_INITIALIZER;
    static int policy = 1;
    if (mode) {
        pthread_mutex_lock(&policy_mutex);
        policy = mode;
        pthread_mutex_unlock(&policy_mutex);
    }
    return policy;
}

static inline int hbw_getpolicy(void)
{
    return hbw_policy(0);
}

static inline void hbw_setpolicy(int mode)
{
    if (mode == 1 || mode == 2) {
        hbw_policy(mode);
    }
}

static inline int hbw_IsHBWAvailable(void)
{
    return numakind_isavail(NUMAKIND_MCDRAM);
}


static inline void *hbw_malloc(size_t size)
{
    void *result;
    result = numakind_malloc(NUMAKIND_MCDRAM, size);
    if (!result && hbw_getpolicy() == 2) {
        result = numakind_malloc(NUMAKIND_DEFAULT, size);
    }
    return result;
}

static inline void *hbw_calloc(size_t num, size_t size)
{
    void *result;
    result = numakind_calloc(NUMAKIND_MCDRAM, num, size);
    if (!result && hbw_getpolicy() == 2) {
        result = numakind_calloc(NUMAKIND_DEFAULT, num, size);
    }
    return result;
}

static inline int hbw_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    int err;
    err = numakind_posix_memalign(NUMAKIND_MCDRAM, memptr, alignment, size);
    if (err && hbw_getpolicy() == 2) {
        err = numakind_posix_memalign(NUMAKIND_DEFAULT, memptr, alignment,
                                      size);
    }
    return err;
}

static inline void *hbw_realloc(void *ptr, size_t size)
{
    void *result;
    result = numakind_realloc(NUMAKIND_MCDRAM, ptr, size);
    if (!result && hbw_getpolicy() == 2) {
        result = numakind_realloc(NUMAKIND_DEFAULT, ptr, size);
    }
    return result;
}

static inline void hbw_free(void *ptr)
{
    numakind_free(NUMAKIND_DEFAULT, ptr);
}

  // all of your legacy C code here

#ifdef __cplusplus
}
#endif
#endif
