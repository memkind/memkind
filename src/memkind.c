/*
 * Copyright (C) 2014, 2015 Intel Corporation.
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
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>
#include <jemalloc/jemalloc.h>

#include "memkind.h"
#include "memkind_default.h"
#include "memkind_hugetlb.h"
#include "memkind_arena.h"
#include "memkind_hbw.h"
#include "memkind_gbtlb.h"


static struct memkind MEMKIND_DEFAULT_STATIC = {
    &MEMKIND_DEFAULT_OPS,
    MEMKIND_PARTITION_DEFAULT,
    "memkind_default",
    PTHREAD_ONCE_INIT,
    0, NULL
};

static struct memkind MEMKIND_HUGETLB_STATIC = {
    &MEMKIND_HUGETLB_OPS,
    MEMKIND_PARTITION_HUGETLB,
    "memkind_hugetlb",
    PTHREAD_ONCE_INIT,
    0, NULL
};

static struct memkind MEMKIND_HBW_STATIC = {
    &MEMKIND_HBW_OPS,
    MEMKIND_PARTITION_HBW,
    "memkind_hbw",
    PTHREAD_ONCE_INIT,
    0, NULL
};

static struct memkind MEMKIND_HBW_PREFERRED_STATIC = {
    &MEMKIND_HBW_PREFERRED_OPS,
    MEMKIND_PARTITION_HBW_PREFERRED,
    "memkind_hbw_preferred",
    PTHREAD_ONCE_INIT,
    0, NULL
};

static struct memkind MEMKIND_HBW_HUGETLB_STATIC = {
    &MEMKIND_HBW_HUGETLB_OPS,
    MEMKIND_PARTITION_HBW_HUGETLB,
    "memkind_hbw_hugetlb",
    PTHREAD_ONCE_INIT,
    0, NULL
};

static struct memkind MEMKIND_HBW_PREFERRED_HUGETLB_STATIC = {
    &MEMKIND_HBW_PREFERRED_HUGETLB_OPS,
    MEMKIND_PARTITION_HBW_PREFERRED_HUGETLB,
    "memkind_hbw_preferred_hugetlb",
    PTHREAD_ONCE_INIT,
    0, NULL
};

static struct memkind MEMKIND_HBW_GBTLB_STATIC = {
    &MEMKIND_HBW_GBTLB_OPS,
    MEMKIND_PARTITION_HBW_GBTLB,
    "memkind_hbw_gbtlb",
    PTHREAD_ONCE_INIT,
    0, NULL
};

static struct memkind MEMKIND_HBW_PREFERRED_GBTLB_STATIC = {
    &MEMKIND_HBW_PREFERRED_GBTLB_OPS,
    MEMKIND_PARTITION_HBW_PREFERRED_GBTLB,
    "memkind_hbw_preferred_gbtlb",
    PTHREAD_ONCE_INIT,
    0, NULL
};

static struct memkind MEMKIND_GBTLB_STATIC = {
    &MEMKIND_GBTLB_OPS,
    MEMKIND_PARTITION_GBTLB,
    "memkind_gbtlb",
    PTHREAD_ONCE_INIT,
    0, NULL
};

struct memkind *MEMKIND_DEFAULT = &MEMKIND_DEFAULT_STATIC;
struct memkind *MEMKIND_HUGETLB = &MEMKIND_HUGETLB_STATIC;
struct memkind *MEMKIND_HBW = &MEMKIND_HBW_STATIC;
struct memkind *MEMKIND_HBW_PREFERRED = &MEMKIND_HBW_PREFERRED_STATIC;
struct memkind *MEMKIND_HBW_HUGETLB = &MEMKIND_HBW_HUGETLB_STATIC;
struct memkind *MEMKIND_HBW_PREFERRED_HUGETLB = &MEMKIND_HBW_PREFERRED_HUGETLB_STATIC;
struct memkind *MEMKIND_HBW_GBTLB = &MEMKIND_HBW_GBTLB_STATIC;
struct memkind *MEMKIND_HBW_PREFERRED_GBTLB = &MEMKIND_HBW_PREFERRED_GBTLB_STATIC;
struct memkind *MEMKIND_GBTLB = &MEMKIND_GBTLB_STATIC;

struct memkind_registry {
    struct memkind *partition_map[MEMKIND_MAX_KIND];
    int num_kind;
    pthread_mutex_t lock;
};

static struct memkind_registry memkind_registry_g = {
    {
        [MEMKIND_PARTITION_DEFAULT] = &MEMKIND_DEFAULT_STATIC,
        [MEMKIND_PARTITION_HBW] = &MEMKIND_HBW_STATIC,
        [MEMKIND_PARTITION_HBW_PREFERRED] = &MEMKIND_HBW_PREFERRED_STATIC,
        [MEMKIND_PARTITION_HBW_HUGETLB] = &MEMKIND_HBW_HUGETLB_STATIC,
        [MEMKIND_PARTITION_HBW_PREFERRED_HUGETLB] = &MEMKIND_HBW_PREFERRED_HUGETLB_STATIC,
        [MEMKIND_PARTITION_HUGETLB] = &MEMKIND_HUGETLB_STATIC,
        [MEMKIND_PARTITION_HBW_GBTLB] = &MEMKIND_HBW_GBTLB_STATIC,
        [MEMKIND_PARTITION_HBW_PREFERRED_GBTLB] = &MEMKIND_HBW_PREFERRED_GBTLB_STATIC,
        [MEMKIND_PARTITION_GBTLB] = &MEMKIND_GBTLB_STATIC,
    },
    MEMKIND_NUM_BASE_KIND,
    PTHREAD_MUTEX_INITIALIZER
};

static int memkind_get_kind_for_free(void *ptr, struct memkind **kind);

void memkind_error_message(int err, char *msg, size_t size)
{
    switch (err) {
        case MEMKIND_ERROR_UNAVAILABLE:
            strncpy(msg, "<memkind> Requested memory kind is not available", size);
            break;
        case MEMKIND_ERROR_MBIND:
            strncpy(msg, "<memkind> Call to mbind() failed", size);
            break;
        case MEMKIND_ERROR_MMAP:
            strncpy(msg, "<memkind> Call to mmap() failed", size);
            break;
        case MEMKIND_ERROR_MEMALIGN:
            strncpy(msg, "<memkind> Call to posix_memalign() failed", size);
            break;
        case MEMKIND_ERROR_MALLCTL:
            strncpy(msg, "<memkind> Call to je_mallctl() failed", size);
            break;
        case MEMKIND_ERROR_MALLOC:
            strncpy(msg, "<memkind> Call to je_malloc() failed", size);
            break;
        case MEMKIND_ERROR_GETCPU:
            strncpy(msg, "<memkind> Call to sched_getcpu() returned out of range", size);
            break;
        case MEMKIND_ERROR_PMTT:
            snprintf(msg, size, "<memkind> Unable to find parsed PMTT table or Invalid PMTT table entries in: %s", MEMKIND_BANDWIDTH_PATH);
            break;
        case MEMKIND_ERROR_TIEDISTANCE:
            strncpy(msg, "<memkind> Two NUMA memory nodes are equidistant from target cpu node", size);
            break;
        case MEMKIND_ERROR_ENVIRON:
            strncpy(msg, "<memkind> Error parsing environment variable (MEMKIND_*)", size);
            break;
        case MEMKIND_ERROR_INVALID:
            strncpy(msg, "<memkind> Invalid input arguments to memkind routine", size);
            break;
        case MEMKIND_ERROR_TOOMANY:
            snprintf(msg, size, "<memkind> Attempted to inizailze more than maximum (%i) number of kinds", MEMKIND_MAX_KIND);
            break;
        case MEMKIND_ERROR_RUNTIME:
            strncpy(msg, "<memkind> Unspecified run-time error", size);
            break;
        case MEMKIND_ERROR_ALIGNMENT:
        case EINVAL:
            strncpy(msg, "<memkind> Alignment must be a power of two and larger than sizeof(void *)", size);
            break;
        case MEMKIND_ERROR_MALLOCX:
        case ENOMEM:
            strncpy(msg, "<memkind> Call to je_mallocx() failed", size);
            break;
        case MEMKIND_ERROR_PTHREAD:
            strncpy(msg, "<memkind> Call to pthread library failed", size);
            break;
        case MEMKIND_ERROR_BADOPS:
            strncpy(msg, "<memkind> memkind_ops structure is poorly formed (missing or incorrect functions)", size);
            break;
        default:
            snprintf(msg, size, "<memkind> Undefined error number: %i", err);
            break;
    }
    if (size > 0) {
        msg[size-1] = '\0';
    }
}

int memkind_create(const struct memkind_ops *ops, const char *name, struct memkind **kind)
{
    int err = 0;
    int tmp = 0;
    int i;

    *kind = NULL;
    err = pthread_mutex_lock(&(memkind_registry_g.lock));
    if (err) {
        err = MEMKIND_ERROR_PTHREAD;
        goto exit;
    }
    if (memkind_registry_g.num_kind == MEMKIND_MAX_KIND) {
        err = MEMKIND_ERROR_TOOMANY;
        goto exit;
    }
    if (ops->create == NULL ||
        ops->destroy == NULL ||
        ops->malloc == NULL ||
        ops->calloc == NULL ||
        ops->realloc == NULL ||
        ops->posix_memalign == NULL ||
        ops->free == NULL ||
        ops->get_size == NULL ||
        ops->init_once != NULL) {
        err = MEMKIND_ERROR_BADOPS;
        goto exit;
    }
    for (i = 0; i < memkind_registry_g.num_kind; ++i) {
        if (strcmp(name, memkind_registry_g.partition_map[i]->name) == 0) {
            err = MEMKIND_ERROR_REPNAME;
            goto exit;
        }
    }
    *kind = (struct memkind *)je_calloc(1, sizeof(struct memkind));
    if (!*kind) {
        err = MEMKIND_ERROR_MALLOC;
        goto exit;
    }

    err = ops->create(*kind, ops, name);
    if (err) {
        goto exit;
    }
    (*kind)->partition = memkind_registry_g.num_kind;
    memkind_registry_g.partition_map[memkind_registry_g.num_kind] = *kind;
    ++memkind_registry_g.num_kind;

exit:
    if (err != MEMKIND_ERROR_PTHREAD) {
        tmp = pthread_mutex_unlock(&(memkind_registry_g.lock));
        err = err ? err : tmp;
    }
    return err;
}

#ifdef __GNUC__
__attribute__((destructor))
#endif
int memkind_finalize(void)
{
    struct memkind *kind;
    int i;
    int err;

    err = pthread_mutex_lock(&(memkind_registry_g.lock));
    if (err) {
        err = MEMKIND_ERROR_PTHREAD;
        goto exit;
    }

    for (i = 0; i < memkind_registry_g.num_kind; ++i) {
        kind = memkind_registry_g.partition_map[i];
        if (kind) {
            err = kind->ops->destroy(kind);
            if (err) {
                goto exit;
            }
            memkind_registry_g.partition_map[i] = NULL;
            if (i >= MEMKIND_NUM_BASE_KIND) {
                je_free(kind);
            }
        }
    }

exit:
    if (err != MEMKIND_ERROR_PTHREAD) {
        err = pthread_mutex_unlock(&(memkind_registry_g.lock)) ?
              MEMKIND_ERROR_PTHREAD : 0;
    }
    return err;
}

int memkind_get_num_kind(int *num_kind)
{
    *num_kind = memkind_registry_g.num_kind;
    return 0;
}

int memkind_get_kind_by_partition(int partition, struct memkind **kind)
{
    int err = 0;

    if (partition >= 0 &&
        partition < MEMKIND_MAX_KIND &&
        memkind_registry_g.partition_map[partition] != NULL) {
        *kind = memkind_registry_g.partition_map[partition];
    }
    else {
        *kind = NULL;
        err = MEMKIND_ERROR_UNAVAILABLE;
    }
    return err;
}

int memkind_get_kind_by_name(const char *name, struct memkind **kind)
{
    int err = 0;
    int i;

    *kind = NULL;
    for (i = 0; i < memkind_registry_g.num_kind; ++i) {
        if (strcmp(name, memkind_registry_g.partition_map[i]->name) == 0) {
            *kind = memkind_registry_g.partition_map[i];
            break;
        }
    }
    if (*kind == NULL) {
        err = MEMKIND_ERROR_UNAVAILABLE;
    }
    return err;
}

void *memkind_partition_mmap(int partition, void *addr, size_t size)
{
    int err;
    void *result = MAP_FAILED;
    struct memkind *kind;

    err = memkind_get_kind_by_partition(partition, &kind);
    if (!err) {
        err = memkind_check_available(kind);
    }
    if (!err) {
        if (kind->ops->mmap) {
            result = kind->ops->mmap(kind, addr, size);
        }
        else {
            result = memkind_default_mmap(kind, addr, size);
        }
    }
    return result;
}

int memkind_check_available(struct memkind *kind)
{
    int err = 0;
    if (kind->ops->check_available) {
        err = kind->ops->check_available(kind);
    }
    return err;
}

void *memkind_malloc(struct memkind *kind, size_t size)
{
    if (kind->ops->init_once) {
        pthread_once(&(kind->init_once), kind->ops->init_once);
    }
    return kind->ops->malloc(kind, size);
}

void *memkind_calloc(struct memkind *kind, size_t num, size_t size)
{
    if (kind->ops->init_once) {
        pthread_once(&(kind->init_once), kind->ops->init_once);
    }
    return kind->ops->calloc(kind, num, size);
}

int memkind_posix_memalign(struct memkind *kind, void **memptr, size_t alignment,
                           size_t size)
{
    if (kind->ops->init_once) {
        pthread_once(&(kind->init_once), kind->ops->init_once);
    }
    return kind->ops->posix_memalign(kind, memptr, alignment, size);
}

void *memkind_realloc(struct memkind *kind, void *ptr, size_t size)
{
    if (kind->ops->init_once) {
        pthread_once(&(kind->init_once), kind->ops->init_once);
    }
    return kind->ops->realloc(kind, ptr, size);
}

void memkind_free(struct memkind *kind, void *ptr)
{
    if (!kind) {
        memkind_get_kind_for_free(ptr, &kind);
    }
    kind->ops->free(kind, ptr);
}

int memkind_get_size(memkind_t kind, size_t *total, size_t *free)
{
    return kind->ops->get_size(kind, total, free);
}

static int memkind_get_kind_for_free(void *ptr, struct memkind **kind)
{
    int i, num_kind;
    struct memkind *test_kind;

    *kind = MEMKIND_DEFAULT;
    memkind_get_num_kind(&num_kind);
    for (i = 0; i < num_kind; ++i) {
        memkind_get_kind_by_partition(i, &test_kind);
        if (test_kind &&
            test_kind->ops->check_addr &&
            test_kind->ops->check_addr(test_kind, ptr) == 0) {
            *kind = test_kind;
            break;
        }
    }
    return 0;
}
