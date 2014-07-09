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

static int hbw_policy_g = HBW_POLICY_BIND;
static pthread_once_t hbw_policy_once_g = PTHREAD_ONCE_INIT;
static inline void hbw_policy_preferred_init(void);
static inline void hbw_policy_bind_init(void);

int hbw_get_policy(void)
{
    return hbw_policy_g;
}

void hbw_set_policy(int mode)
{
    if (mode == HBW_POLICY_PREFERRED) {
        pthread_once(&hbw_policy_once_g, hbw_policy_preferred_init);
    }
    else if (mode == HBW_POLICY_BIND) {
        pthread_once(&hbw_policy_once_g, hbw_policy_bind_init);
    }
    if (mode != hbw_policy_g) {
        fprintf(stderr, "WARNING: hbw_set_policy() called more than once with different values, only first call heeded.\n");
    }
}

int hbw_is_available(void)
{
    numakind_init();
    return numakind_is_available(NUMAKIND_HBW);
}

void *hbw_malloc(size_t size)
{
    numakind_t kind;
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
    numakind_t kind;
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
    numakind_t kind;
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
    numakind_t kind;
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
    numakind_t kind;
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


static inline void hbw_policy_bind_init(void)
{
    hbw_policy_g = HBW_POLICY_BIND;
}

static inline void hbw_policy_preferred_init(void)
{
    hbw_policy_g = HBW_POLICY_PREFERRED;
}
