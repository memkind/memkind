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
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <jemalloc/jemalloc.h>

#include "numakind.h"
#include "numakind_default.h"
#include "numakind_arena.h"
#include "numakind_hbw.h"


struct numakind NUMAKIND_DEFAULT_STATIC = {&NUMAKIND_DEFAULT_OPS, NUMAKIND_PARTITION_DEFAULT, "numakind_default", 0, NULL};
struct numakind NUMAKIND_HBW_STATIC = {&NUMAKIND_HBW_OPS, NUMAKIND_PARTITION_HBW, "numakind_hbw", 0, NULL};
struct numakind NUMAKIND_HBW_PREFERRED_STATIC = {&NUMAKIND_HBW_PREFERRED_OPS, NUMAKIND_PARTITION_HBW_PREFERRED, "numakind_hbw_preferred", 0, NULL};
struct numakind NUMAKIND_HBW_HUGETLB_STATIC = {&NUMAKIND_HBW_HUGETLB_OPS, NUMAKIND_PARTITION_HBW_HUGETLB, "numakind_hbw_hugetlb", 0, NULL};
struct numakind NUMAKIND_HBW_PREFERRED_HUGETLB_STATIC = {&NUMAKIND_HBW_PREFERRED_HUGETLB_OPS, NUMAKIND_PARTITION_HBW_PREFERRED_HUGETLB, "numakind_hbw_preferred_hugetlb", 0, NULL};

struct numakind *NUMAKIND_DEFAULT = &NUMAKIND_DEFAULT_STATIC;
struct numakind *NUMAKIND_HBW = &NUMAKIND_HBW_STATIC;
struct numakind *NUMAKIND_HBW_PREFERRED = &NUMAKIND_HBW_PREFERRED_STATIC;
struct numakind *NUMAKIND_HBW_HUGETLB = &NUMAKIND_HBW_HUGETLB_STATIC;
struct numakind *NUMAKIND_HBW_PREFERRED_HUGETLB = &NUMAKIND_HBW_PREFERRED_HUGETLB_STATIC;


struct numakind_registry {
    struct numakind *partition_map[NUMAKIND_MAX_KIND];
    int num_kind;
    pthread_mutex_t lock;
};

static struct numakind_registry numakind_registry_g = {{0}, 0, PTHREAD_MUTEX_INITIALIZER};
static pthread_once_t numakind_init_once_g = PTHREAD_ONCE_INIT;
static void numakind_init_once(void);

void numakind_init(void)
{
    pthread_once(&numakind_init_once_g, numakind_init_once);
}

void numakind_error_message(int err, char *msg, size_t size)
{
    switch (err) {
        case NUMAKIND_ERROR_UNAVAILABLE:
            strncpy(msg, "<numakind> Requested numa kind is not available", size);
            break;
        case NUMAKIND_ERROR_MBIND:
            strncpy(msg, "<numakind> Call to mbind() failed", size);
            break;
        case NUMAKIND_ERROR_MEMALIGN:
            strncpy(msg, "<numakind> Call to posix_memalign() failed", size);
            break;
        case NUMAKIND_ERROR_MALLCTL:
            strncpy(msg, "<numakind> Call to je_mallctl() failed", size);
            break;
        case NUMAKIND_ERROR_MALLOC:
            strncpy(msg, "<numakind> Call to je_malloc() failed", size);
            break;
        case NUMAKIND_ERROR_GETCPU:
            strncpy(msg, "<numakind> Call to sched_getcpu() returned out of range", size);
            break;
        case NUMAKIND_ERROR_PMTT:
            snprintf(msg, size, "<numakind> Unable to find parsed PMTT table or Invalid PMTT table entries in: %s", NUMAKIND_BANDWIDTH_PATH);
            break;
        case NUMAKIND_ERROR_TIEDISTANCE:
            strncpy(msg, "<numakind> Two NUMA memory nodes are equidistant from target cpu node", size);
            break;
        case NUMAKIND_ERROR_ENVIRON:
            strncpy(msg, "<numakind> Error parsing environment variable (NUMAKIND_*)", size);
            break;
        case NUMAKIND_ERROR_INVALID:
            strncpy(msg, "<numakind> Invalid input arguments to numakind routine", size);
            break;
        case NUMAKIND_ERROR_TOOMANY:
            strncpy(msg, "<numakind> Attempted to inizailze more than maximum (%i) number of kinds", NUMAKIND_MAX_KIND);
            break;
        case NUMAKIND_ERROR_RUNTIME:
            strncpy(msg, "<numakind> Unspecified run-time error", size);
            break;
        case NUMAKIND_ERROR_ALIGNMENT:
        case EINVAL:
            strncpy(msg, "<numakind> Alignment must be a power of two and larger than sizeof(void *)", size);
            break;
        case NUMAKIND_ERROR_MALLOCX:
        case ENOMEM:
            strncpy(msg, "<numakind> Call to je_mallocx() failed", size);
            break;
        default:
            snprintf(msg, size, "<numakind> Undefined error number: %i", err);
            break;
    }
    if (size > 0) {
        msg[size-1] = '\0';
    }
}

int numakind_create(const struct numakind_ops *ops, const char *name)
{
    int err = 0;
    int i;
    struct numakind *kind;

    pthread_mutex_lock(&(numakind_registry_g.lock));
    if (numakind_registry_g.num_kind == NUMAKIND_MAX_KIND) {
        err = NUMAKIND_ERROR_TOOMANY;
    }
    if (!err) {
        for (i = 0; i < numakind_registry_g.num_kind; ++i) {
            if (strcmp(name, numakind_registry_g.partition_map[i]->name) == 0) {
                err = NUMAKIND_ERROR_REPNAME;
            }
        }
    }
    if (!err) {
        kind = (struct numakind *)je_malloc(sizeof(struct numakind));
        if (!kind) {
            err = NUMAKIND_ERROR_MALLOC;
        }
    }
    if (!err) {
        kind->partition = numakind_registry_g.num_kind;
        err = ops->create(kind, ops, name);
    }
    if (!err) {
        numakind_registry_g.partition_map[numakind_registry_g.num_kind] = kind;
        ++numakind_registry_g.num_kind;
    }
    pthread_mutex_unlock(&(numakind_registry_g.lock));
    return err;
}

void numakind_finalize(void)
{
    struct numakind *kind;
    int i;

    pthread_mutex_lock(&(numakind_registry_g.lock));
    for (i = 0; i < numakind_registry_g.num_kind; ++i) {
        kind = numakind_registry_g.partition_map[i];
        if (kind) {
            kind->ops->destroy(kind);
            je_free(kind);
            numakind_registry_g.partition_map[i] = NULL;
        }
    }
    pthread_mutex_unlock(&(numakind_registry_g.lock));
}

int numakind_get_num_kind(int *num_kind)
{
    *num_kind = numakind_registry_g.num_kind;
    return 0;
}

int numakind_get_kind_by_partition(int partition, struct numakind **kind)
{
    int err = 0;

    if (partition < NUMAKIND_NUM_BASE_KIND &&
        numakind_registry_g.partition_map[partition] == NULL) {
        numakind_init();
    }
    if (partition < NUMAKIND_MAX_KIND &&
        numakind_registry_g.partition_map[partition] != NULL) {
        *kind = numakind_registry_g.partition_map[partition];
    }
    else {
        *kind = NULL;
        err = NUMAKIND_ERROR_UNAVAILABLE;
    }
    return err;
}

int numakind_get_kind_by_name(const char *name, struct numakind **kind)
{
    int err = 0;
    int i;

    *kind = NULL;
    for (i = 0; i < numakind_registry_g.num_kind; ++i) {
        if (strcmp(name, numakind_registry_g.partition_map[i]->name) == 0) {
            *kind = numakind_registry_g.partition_map[i];
            break;
        }
    }
    if (*kind == NULL) {
        err = NUMAKIND_ERROR_UNAVAILABLE;
    }
    return err;
}

int numakind_partition_is_available(int partition)
{
    struct numakind *kind;
    int err;
    int result;

    err = numakind_get_kind_by_partition(partition, &kind);
    if (!err) {
        result = kind->ops->is_available(kind);
    }
    else {
        result = 0;
    }
    return result;
}

int numakind_partition_get_mmap_flags(int partition, int *flags)
{
    struct numakind *kind;
    int err = 0;

    err = numakind_get_kind_by_partition(partition, &kind);
    if (!err) {
        err = kind->ops->get_mmap_flags(kind, flags);
    }
    else {
        *flags = 0;
    }
    return err;
}

int numakind_partition_mbind(int partition, void *addr, size_t size)
{
    struct numakind *kind;
    int err = 0;

    err = numakind_get_kind_by_partition(partition, &kind);
    if (!err) {
        err = kind->ops->mbind(kind, addr, size);
    }
    return err;
}

int numakind_is_available(struct numakind *kind)
{
    return kind->ops->is_available(kind);
}

void *numakind_malloc(struct numakind *kind, size_t size)
{
    pthread_once(&numakind_init_once_g, numakind_init_once);
    return kind->ops->malloc(kind, size);
}

void *numakind_calloc(struct numakind *kind, size_t num, size_t size)
{
    pthread_once(&numakind_init_once_g, numakind_init_once);
    return kind->ops->calloc(kind, num, size);
}

int numakind_posix_memalign(struct numakind *kind, void **memptr, size_t alignment,
                            size_t size)
{
    pthread_once(&numakind_init_once_g, numakind_init_once);
    return kind->ops->posix_memalign(kind, memptr, alignment, size);
}

void *numakind_realloc(struct numakind *kind, void *ptr, size_t size)
{
    pthread_once(&numakind_init_once_g, numakind_init_once);
    return kind->ops->realloc(kind, ptr, size);
}

void numakind_free(struct numakind *kind, void *ptr)
{
    kind->ops->free(kind, ptr);
}

int numakind_get_size(numakind_t kind, size_t *total, size_t *free)
{
    return kind->ops->get_size(kind, total, free);
}

static void numakind_init_once(void)
{
    numakind_arena_create_map(NUMAKIND_HBW);
    numakind_arena_create_map(NUMAKIND_HBW_HUGETLB);
    numakind_arena_create_map(NUMAKIND_HBW_PREFERRED);
    numakind_arena_create_map(NUMAKIND_HBW_PREFERRED_HUGETLB);
}

