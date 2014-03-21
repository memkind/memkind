#include <pthread.h>

#include "hbwmalloc.h"
#include "numakind.h"

static inline int hbw_policy(int mode);

int hbw_getpolicy(void)
{
    return hbw_policy(0);
}

void hbw_setpolicy(int mode)
{
    hbw_policy(mode);
}

int hbw_IsHBWAvailable(void)
{
    return numakind_isavail(NUMAKIND_MCDRAM);
}


void *hbw_malloc(size_t size)
{
    void *result;
    result = numakind_malloc(NUMAKIND_MCDRAM, size);
    if (!result && hbw_getpolicy() == HBW_POLICY_PREFERRED) {
        result = numakind_malloc(NUMAKIND_DEFAULT, size);
    }
    return result;
}

void *hbw_calloc(size_t num, size_t size)
{
    void *result;
    result = numakind_calloc(NUMAKIND_MCDRAM, num, size);
    if (!result && hbw_getpolicy() == HBW_POLICY_PREFERRED) {
        result = numakind_calloc(NUMAKIND_DEFAULT, num, size);
    }
    return result;
}

int hbw_allocate_memalign(void **memptr, size_t alignment, size_t size)
{
    int err;
    err = numakind_posix_memalign(NUMAKIND_MCDRAM, memptr, alignment, size);
    if (err && hbw_getpolicy() == HBW_POLICY_PREFERRED) {
        err = numakind_posix_memalign(NUMAKIND_DEFAULT, memptr, alignment,
                                      size);
    }
    return err;
}

int hbw_allocate_memalign_psize(void **memptr, size_t alignment, size_t size,
    int pagesize)
{
    int err;
    if (pagesize == HBW_PAGESIZE_2MB) {
        err = numakind_posix_memalign(NUMAKIND_MCDRAM_HUGETLB, memptr, 
                                      alignment, size);
    }
    else {
        err = numakind_posix_memalign(NUMAKIND_MCDRAM, memptr, alignment, size);
    }
    if (err && hbw_getpolicy() == HBW_POLICY_PREFERRED) {
        err = numakind_posix_memalign(NUMAKIND_DEFAULT, memptr, alignment,
                                      size);
    }
    return err;
}

void *hbw_realloc(void *ptr, size_t size)
{
    void *result;
    result = numakind_realloc(NUMAKIND_MCDRAM, ptr, size);
    if (!result && hbw_getpolicy() == HBW_POLICY_PREFERRED) {
        result = numakind_realloc(NUMAKIND_DEFAULT, ptr, size);
    }
    return result;
}

void hbw_free(void *ptr)
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
