#include <pthread.h>

#include "hbwmalloc.h"
#include "numakind.h"

static inline int hbw_policy(int mode);

inline int hbw_getpolicy(void)
{
    return hbw_policy(0);
}

inline void hbw_setpolicy(int mode)
{
    hbw_policy(mode);
}

inline int hbw_IsHBWAvailable(void)
{
    return numakind_isavail(NUMAKIND_MCDRAM);
}


inline void *hbw_malloc(size_t size)
{
    void *result;
    result = numakind_malloc(NUMAKIND_MCDRAM, size);
    if (!result && hbw_getpolicy() == 2) {
        result = numakind_malloc(NUMAKIND_DEFAULT, size);
    }
    return result;
}

inline void *hbw_calloc(size_t num, size_t size)
{
    void *result;
    result = numakind_calloc(NUMAKIND_MCDRAM, num, size);
    if (!result && hbw_getpolicy() == 2) {
        result = numakind_calloc(NUMAKIND_DEFAULT, num, size);
    }
    return result;
}

inline int hbw_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    int err;
    err = numakind_posix_memalign(NUMAKIND_MCDRAM, memptr, alignment, size);
    if (err && hbw_getpolicy() == 2) {
        err = numakind_posix_memalign(NUMAKIND_DEFAULT, memptr, alignment,
                                      size);
    }
    return err;
}

inline void *hbw_realloc(void *ptr, size_t size)
{
    void *result;
    result = numakind_realloc(NUMAKIND_MCDRAM, ptr, size);
    if (!result && hbw_getpolicy() == 2) {
        result = numakind_realloc(NUMAKIND_DEFAULT, ptr, size);
    }
    return result;
}

inline void hbw_free(void *ptr)
{
    numakind_free(NUMAKIND_DEFAULT, ptr);
}

static inline int hbw_policy(int mode)
{
    static pthread_mutex_t policy_mutex = PTHREAD_MUTEX_INITIALIZER;
    static int policy = HBW_POLICY_BIND;
    if (mode == HBW_POLICY_BIND || mode == HBW_POLICY_PREFERRED) {
        pthread_mutex_lock(&policy_mutex);
        policy = mode;
        pthread_mutex_unlock(&policy_mutex);
    }
    return policy;
}
