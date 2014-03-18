#ifndef hbw_malloc_include_h
#define hbw_malloc_include_h

#include "numakind.h"

inline void *hbw_malloc(size_t size)
{
    void *result;
    result = numakind_malloc(NUMAKIND_MCDRAM, size);
    if (!result) {
        result = numakind_malloc(NUMAKIND_DEFAULT, size);
    }
    return result;
}

inline void *hbw_calloc(size_t num, size_t size)
{
    void *result;
    result = numakind_calloc(NUMAKIND_MCDRAM, num, size);
    if (!result) {
        result = numakind_calloc(NUMAKIND_DEFAULT, num, size);
    }
    return result;
}

inline int hbw_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    int err;
    err = numakind_posix_memalign(NUMAKIND_MCDRAM, memptr, alignment, size);
    if (err) {
        err = numakind_posix_memalign(NUMAKIND_DEFAULT, memptr, alignment,
                                      size);
    }
    return err;
}

inline void *hbw_realloc(void *ptr, size_t size)
{
    void *result;
    result = numakind_realloc(NUMAKIND_MCDRAM, ptr, size);
    if (!result) {
        result = numakind_realloc(NUMAKIND_DEFAULT, ptr, size);
    }
    return result;
}

inline void hbw_free(void *ptr)
{
    numakind_free(NUMAKIND_DEFAULT, ptr);
}

#endif
