/*
 * Copyright (C) 2014 - 2016 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#include <hbwmalloc.h>
#include <memkind.h>

#include <memkind/internal/memkind_private.h>

static hbw_policy_t hbw_policy_g = HBW_POLICY_PREFERRED;
static pthread_once_t hbw_policy_once_g = PTHREAD_ONCE_INIT;

static void hbw_policy_bind_init(void)
{
    hbw_policy_g = HBW_POLICY_BIND;
}

static void hbw_policy_preferred_init(void)
{
    hbw_policy_g = HBW_POLICY_PREFERRED;
}

static void hbw_policy_interleave_init(void)
{
    hbw_policy_g = HBW_POLICY_INTERLEAVE;
}

// This function is intended to be called once per pagesize
// Getting kind should be done using hbw_get_kind() defined below
static memkind_t hbw_choose_kind(hbw_pagesize_t pagesize)
{
    memkind_t result = NULL;

    if(hbw_policy_once_g == PTHREAD_ONCE_INIT) {
        //hbw_policy_g is statically initialized
        hbw_set_policy(hbw_policy_g);
    }

    int policy = hbw_get_policy();

    if (policy == HBW_POLICY_BIND || policy == HBW_POLICY_INTERLEAVE) {
        switch (pagesize) {
            case HBW_PAGESIZE_2MB:
                result = MEMKIND_HBW_HUGETLB;
                break;
            case HBW_PAGESIZE_1GB:
            case HBW_PAGESIZE_1GB_STRICT:
                result = MEMKIND_HBW_GBTLB;
                break;
            default:
                if (policy == HBW_POLICY_BIND) {
                    result = MEMKIND_HBW;
                }
                else {
                    result = MEMKIND_HBW_INTERLEAVE;
                }
                break;
        }
    }
    else if (memkind_check_available(MEMKIND_HBW) == 0) {
        switch (pagesize) {
            case HBW_PAGESIZE_2MB:
                result = MEMKIND_HBW_PREFERRED_HUGETLB;
                break;
            case HBW_PAGESIZE_1GB:
            case HBW_PAGESIZE_1GB_STRICT:
                result = MEMKIND_HBW_PREFERRED_GBTLB;
                break;
            default:
                result = MEMKIND_HBW_PREFERRED;
                break;
        }
    }
    else {
        switch (pagesize) {
            case HBW_PAGESIZE_2MB:
                result = MEMKIND_HUGETLB;
                break;
            case HBW_PAGESIZE_1GB:
            case HBW_PAGESIZE_1GB_STRICT:
                result = MEMKIND_GBTLB;
                break;
            default:
                result = MEMKIND_DEFAULT;
                break;
        }
    }
    return result;
}

static memkind_t pagesize_kind[HBW_PAGESIZE_MAX_VALUE];
static inline memkind_t hbw_get_kind(hbw_pagesize_t pagesize)
{
    if(pagesize_kind[pagesize] == NULL)
    {
        pagesize_kind[pagesize] = hbw_choose_kind(pagesize);
    }
    return pagesize_kind[pagesize];
}


hbw_policy_t hbw_get_policy(void)
{
    return hbw_policy_g;
}

int hbw_set_policy(hbw_policy_t mode)
{
    switch(mode) {
        case HBW_POLICY_PREFERRED:
            pthread_once(&hbw_policy_once_g, hbw_policy_preferred_init);
            break;
        case HBW_POLICY_BIND:
            pthread_once(&hbw_policy_once_g, hbw_policy_bind_init);
            break;
        case HBW_POLICY_INTERLEAVE:
            pthread_once(&hbw_policy_once_g, hbw_policy_interleave_init);
            break;
        default:
             return EINVAL;
    }

    if (mode != hbw_policy_g) {
        return EPERM;
    }

    return 0;
}

int hbw_check_available(void)
{
    return  (memkind_check_available(MEMKIND_HBW) == 0) ? 0 : ENODEV;
}

void *hbw_malloc(size_t size)
{
    return memkind_malloc(hbw_get_kind(HBW_PAGESIZE_4KB), size);
}

void *hbw_calloc(size_t num, size_t size)
{
    return memkind_calloc(hbw_get_kind(HBW_PAGESIZE_4KB), num, size);
}

int hbw_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    return memkind_posix_memalign(hbw_get_kind(HBW_PAGESIZE_4KB), memptr, alignment, size);
}

int hbw_posix_memalign_psize(void **memptr, size_t alignment, size_t size,
                             hbw_pagesize_t pagesize)
{
    memkind_t kind;

    if (pagesize == HBW_PAGESIZE_1GB_STRICT &&
        size % (1 << 30)) {
        return EINVAL;
    }

    kind = hbw_get_kind(pagesize);
    return memkind_posix_memalign(kind, memptr, alignment, size);
}

void *hbw_realloc(void *ptr, size_t size)
{
    int i;
    memkind_t kind;
    memkind_t gbtlb_kinds[] = {MEMKIND_HBW_GBTLB, MEMKIND_HBW_PREFERRED_GBTLB, MEMKIND_GBTLB};
    #define GBTLB_KINDS_LEN  sizeof(gbtlb_kinds)/sizeof(memkind_t)

    for (i = 0; i < GBTLB_KINDS_LEN; i++) {
        kind = gbtlb_kinds[i];
        if (kind->ops->check_addr(kind, ptr) == 0) {
            break;
        }
    }
    if (i == GBTLB_KINDS_LEN) {
        kind = hbw_get_kind(HBW_PAGESIZE_4KB);
    }
    return memkind_realloc(kind, ptr, size);
}

void hbw_free(void *ptr)
{
    memkind_free(0, ptr);
}
