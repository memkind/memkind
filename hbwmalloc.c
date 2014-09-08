/*
 * Copyright (C) 2014 Intel Corperation.
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

#include "hbwmalloc.h"
#include "memkind.h"

static int hbw_policy_g = HBW_POLICY_PREFERRED;
static pthread_once_t hbw_policy_once_g = PTHREAD_ONCE_INIT;
static inline memkind_t hbw_get_kind(int pagesize);
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

int hbw_check_available(void)
{
    return memkind_check_available(MEMKIND_HBW);
}

void *hbw_malloc(size_t size)
{
    memkind_t kind;

    kind = hbw_get_kind(HBW_PAGESIZE_4KB);
    return memkind_malloc(kind, size);
}

void *hbw_calloc(size_t num, size_t size)
{
    memkind_t kind;

    kind = hbw_get_kind(HBW_PAGESIZE_4KB);
    return memkind_calloc(kind, num, size);
}

int hbw_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    memkind_t kind;

    kind = hbw_get_kind(HBW_PAGESIZE_4KB);
    return memkind_posix_memalign(kind, memptr, alignment, size);
}

int hbw_posix_memalign_psize(void **memptr, size_t alignment, size_t size,
                                int pagesize)
{
    memkind_t kind;

    kind = hbw_get_kind(pagesize);
    return memkind_posix_memalign(kind, memptr, alignment, size);
}

void *hbw_realloc(void *ptr, size_t size)
{
    memkind_t kind;

    kind = hbw_get_kind(HBW_PAGESIZE_4KB);
    return memkind_realloc(kind, ptr, size);
}

void hbw_free(void *ptr)
{
    memkind_t kind;
    memkind_get_kind_for_free(ptr, &kind);
    memkind_free(kind, ptr);
}

static inline memkind_t hbw_get_kind(int pagesize)
{
    memkind_t result = NULL;

    if (hbw_get_policy() == HBW_POLICY_BIND) {
        switch (pagesize) {
            case HBW_PAGESIZE_2MB:
                result = MEMKIND_HBW_HUGETLB;
                break;
            case HBW_PAGESIZE_1GB:
                result = MEMKIND_HBW_GBTLB;
                break;
            case HBW_PAGESIZE_1GB_STRICT:
                result = MEMKIND_HBW_GBTLB_STRICT;
                break;
            default:
                result = MEMKIND_HBW;
                break;
        }
    }
    else if (memkind_check_available(MEMKIND_HBW) == 0) {
        switch (pagesize) {
            case HBW_PAGESIZE_2MB:
                result = MEMKIND_HBW_PREFERRED_HUGETLB;
                break;
            case HBW_PAGESIZE_1GB:
                result = MEMKIND_HBW_PREFERRED_GBTLB;
                break;
            case HBW_PAGESIZE_1GB_STRICT:
                result = MEMKIND_HBW_PREFERRED_GBTLB_STRICT;
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
                result = MEMKIND_GBTLB;
                break;
            case HBW_PAGESIZE_1GB_STRICT:
                result = MEMKIND_GBTLB_STRICT;
                break;
            default:
                result = MEMKIND_DEFAULT;
                break;
        }
    }
    return result;
}


static inline void hbw_policy_bind_init(void)
{
    hbw_policy_g = HBW_POLICY_BIND;
}

static inline void hbw_policy_preferred_init(void)
{
    hbw_policy_g = HBW_POLICY_PREFERRED;
}
