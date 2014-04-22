/* FILE LICENSE TAG: intel */
/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "hbwmalloc.h"
#include "numakind.h"

static inline int HBW_policy(int mode);

int hbw_get_policy(void)
{
    return HBW_policy(0);
}

void hbw_set_policy(int mode)
{
    HBW_policy(mode);
}

int hbw_is_available(void)
{
    return numakind_is_available(NUMAKIND_HBW);
}


void *hbw_malloc(size_t size)
{
    int kind;
    if (hbw_get_policy() == HBW_POLICY_BIND) {
        kind = NUMAKIND_HBW;
    }
    else {
        kind = NUMAKIND_HBW_PREFERRED;
    }
    return numakind_malloc(kind, size);
}

void *hbw_calloc(size_t num, size_t size)
{
    int kind;
    if (hbw_get_policy() == HBW_POLICY_BIND) {
        kind = NUMAKIND_HBW;
    }
    else {
        kind = NUMAKIND_HBW_PREFERRED;
    }
    return numakind_calloc(kind, num, size);
}

int hbw_allocate_memalign(void **memptr, size_t alignment, size_t size)
{
    int kind;
    if (hbw_get_policy() == HBW_POLICY_BIND) {
        kind = NUMAKIND_HBW;
    }
    else {
        kind = NUMAKIND_HBW_PREFERRED;
    }
    return numakind_posix_memalign(kind, memptr, alignment, size);
}

int hbw_allocate_memalign_psize(void **memptr, size_t alignment, size_t size,
    int pagesize)
{
    int kind;
    if (hbw_get_policy() == HBW_POLICY_BIND) {
        if (pagesize == HBW_PAGESIZE_2MB) {
            kind = NUMAKIND_HBW_HUGETLB;
        }
        else {
            kind = NUMAKIND_HBW;
        }
    }
    else {
        if (pagesize == HBW_PAGESIZE_2MB) {
            kind = NUMAKIND_HBW_PREFERRED_HUGETLB;
        }
        else {
            kind = NUMAKIND_HBW_PREFERRED;
        }
    }
    return numakind_posix_memalign(kind, memptr, alignment, size);
}

void *hbw_realloc(void *ptr, size_t size)
{
    int kind;
    if (hbw_get_policy() == HBW_POLICY_BIND) {
        kind = NUMAKIND_HBW;
    }
    else {
        kind = NUMAKIND_HBW_PREFERRED;
    }
    return numakind_realloc(kind, ptr, size);
}

void hbw_free(void *ptr)
{
    numakind_free(NUMAKIND_DEFAULT, ptr);
}

static inline int HBW_policy(int mode)
{
    static pthread_mutex_t policy_mutex = PTHREAD_MUTEX_INITIALIZER;
    static int policy = HBW_POLICY_BIND;
    static int is_set = 0; /* Policy can be set only once */
    int err = 0;

    if (!mode) {
        return policy;
    }

    if (mode == HBW_POLICY_BIND || mode == HBW_POLICY_PREFERRED) {
        if (is_set == 0) {
            pthread_mutex_lock(&policy_mutex);
            if (is_set == 0) {
                policy = mode;
                is_set = 1;
                pthread_mutex_unlock(&policy_mutex);
            }
            else {
                err = 1;
            }
        }
        else {
            err = 1;
        }
        if (err) {
            fprintf(stderr, "WARNING: hbw_set_policy() called more than once, only first call heeded.\n");
        }
    }
    else {
        fprintf(stderr, "WARNING: hbw_set_policy() called with unknown mode %i, ignored.\n", mode);
    }
    return policy;
}
