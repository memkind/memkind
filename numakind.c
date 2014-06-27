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
#include <string.h>
#include <numa.h>
#include <numaif.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <jemalloc/jemalloc.h>

#include "numakind.h"
#include "numakind_hbw.h"

struct numakind_arena_map_t {
    int init_err;
    int num_cpu;
    unsigned int *arena_map;
};

static struct numakind_arena_map_t numakind_arena_map_g;
static pthread_once_t numakind_arena_map_once_g = PTHREAD_ONCE_INIT;

static void numakind_arena_map_init(void);

static int numakind_getarena(numakind_t kind, int *arena);

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
        case NUMAKIND_ERROR_HBW:
            strncpy(msg, "<numakind> Initializing for HBW failed", size);
            break;
        case NUMAKIND_ERROR_PMTT:
            snprintf(msg, size, "<numakind> Unable to parse bandwidth table: %s", NUMAKIND_BANDWIDTH_PATH);
            break;
        case NUMAKIND_ERROR_TIEDISTANCE:
            strncpy(msg, "<numakind> Two NUMA memory nodes are equidistant from target cpu node", size);
            break;
        case NUMAKIND_ERROR_ALIGNMENT:
            strncpy(msg, "<numakind> Alignment must be a power of two and larger than sizeof(void *)", size);
            break;
        case NUMAKIND_ERROR_ALLOCM:
            strncpy(msg, "<numakind> Call to je_allocm() failed", size);
            break;
        case NUMAKIND_ERROR_ENVIRON:
            strncpy(msg, "<numakind> Error parsing environment variable (NUMAKIND_*)", size);
            break;
        default:
            snprintf(msg, size, "<numakind> Undefined error number: %i", err);
            break;
    }
    if (size > 0)
        msg[size-1] = '\0';
}

int numakind_is_available(int kind)
{
    int result;
    switch (kind) {
        case NUMAKIND_HBW:
        case NUMAKIND_HBW_HUGETLB:
        case NUMAKIND_HBW_PREFERRED:
        case NUMAKIND_HBW_PREFERRED_HUGETLB:
            result = numakind_hbw_is_available();
            break;
        case NUMAKIND_DEFAULT:
            result = 1;
            break;
        default:
            result = 0;
            fprintf(stderr, "WARNING: Unknown numakind_t (%i)\n", (int)kind);
            break;
    }
    return result;
}

int numakind_get_mmap_flags(int kind, int *flags)
{
    int err = 0;
    switch (kind) {
        case NUMAKIND_HBW_HUGETLB:
        case NUMAKIND_HBW_PREFERRED_HUGETLB:
            *flags = MAP_HUGETLB | MAP_ANON;
            break;
        case NUMAKIND_HBW:
        case NUMAKIND_HBW_PREFERRED:
            *flags = MAP_ANON;
            break;
        case NUMAKIND_DEFAULT:
            *flags = 0;
            break;
        default:
            *flags = 0;
            err = NUMAKIND_ERROR_UNAVAILABLE;
            break;
    }
    return err;
}

int numakind_get_nodemask(int kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};
    int err = 0;

    switch (kind) {
        case NUMAKIND_HBW:
        case NUMAKIND_HBW_HUGETLB:
        case NUMAKIND_HBW_PREFERRED:
        case NUMAKIND_HBW_PREFERRED_HUGETLB:
            err = numakind_hbw_get_nodemask(nodemask, maxnode);
            break;
        case NUMAKIND_DEFAULT:
            numa_bitmask_clearall(&nodemask_bm);
            numa_bitmask_setbit(&nodemask_bm, numa_preferred());
            break;
        default:
            numa_bitmask_clearall(&nodemask_bm);
            err = NUMAKIND_ERROR_UNAVAILABLE;
            break;
    }
    return err;
}

int numakind_mbind(int kind, void *addr, size_t size)
{
    nodemask_t nodemask;
    int err = 0;
    int mode;
    err = numakind_get_nodemask(kind, nodemask.n, NUMA_NUM_NODES);
    if (!err) {
        switch (kind) {
            case NUMAKIND_HBW:
            case NUMAKIND_HBW_HUGETLB:
                mode = MPOL_BIND;
                break;
            case NUMAKIND_HBW_PREFERRED:
            case NUMAKIND_HBW_PREFERRED_HUGETLB:
            case NUMAKIND_DEFAULT:
                mode = MPOL_PREFERRED;
                break;
            default:
                err = NUMAKIND_ERROR_UNAVAILABLE;
                break;
        }
        if (!err) {
            err = mbind(addr, size, mode, nodemask.n, NUMA_NUM_NODES, 0);
            err = err ? NUMAKIND_ERROR_MBIND : 0;
        }
    }
    return err;
}

void *numakind_malloc(numakind_t kind, size_t size)
{
    void *result = NULL;
    int err = 0;
    int arena;

    if (kind == NUMAKIND_DEFAULT) {
        result = je_malloc(size);
    }
    else {
        err = numakind_getarena(kind, &arena);
        if (!err) {
            err = je_allocm(&result, NULL, size, ALLOCM_ARENA(arena));
        }
        if (err) {
            result = NULL;
        }
    }
    return result;
}

void *numakind_realloc(numakind_t kind, void *ptr, size_t size)
{
    int err = 0;
    int arena;

    if (kind == NUMAKIND_DEFAULT) {
        ptr = je_realloc(ptr, size);
    }
    else {
        if (size == 0 && ptr != NULL) {
            numakind_free(kind, ptr);
            ptr = NULL;
        }
        else {
            err = numakind_getarena(kind, &arena);
            if (!err) {
                if (ptr == NULL) {
                    err = je_allocm(&ptr, NULL, size, ALLOCM_ARENA(arena));
                }
                else {
                    err = je_rallocm(&ptr, NULL, size, 0, ALLOCM_ARENA(arena));
                }
            }
            if (err) {
                ptr = NULL;
            }
        }
    }
    return ptr;
}

void *numakind_calloc(numakind_t kind, size_t num, size_t size)
{
    void *result = NULL;
    int err = 0;
    int arena;

    if (kind == NUMAKIND_DEFAULT) {
        result = je_calloc(num, size);
    }
    else {
        err = numakind_getarena(kind, &arena);
        if (!err) {
            err = je_allocm(&result, NULL, num * size,
                            ALLOCM_ARENA(arena) | MALLOCX_ZERO);
        }
        if (err) {
            result = NULL;
        }
    }
    return result;
}

int numakind_posix_memalign(numakind_t kind, void **memptr, size_t alignment,
                            size_t size)
{
    int err = 0;
    int arena;

    *memptr = NULL;
    if (kind == NUMAKIND_DEFAULT) {
        err = je_posix_memalign(memptr, alignment, size);
        err = err ? NUMAKIND_ERROR_MEMALIGN : 0;
    }
    else {
        err = numakind_getarena(kind, &arena);
        if (!err) {
            if ( (alignment < sizeof(void*)) ||
                 (((alignment - 1) & alignment) != 0) ) {
                err = NUMAKIND_ERROR_ALIGNMENT;
            }
        }
        if (!err) {
            err = je_allocm(memptr, NULL, size,
                            ALLOCM_ALIGN(alignment) | ALLOCM_ARENA(arena));
            err = err ? NUMAKIND_ERROR_ALLOCM : 0;
        }
    }
    if (err) {
        *memptr = NULL;
    }
    return err;
}

void numakind_free(numakind_t kind, void *ptr)
{
    je_free(ptr);
}

static void numakind_arena_map_init(void)
{
    struct numakind_arena_map_t *g = &numakind_arena_map_g;
    unsigned int kind_select;
    int i, j;
    size_t unsigned_size = sizeof(unsigned);
    unsigned int map_len;

    g->init_err = 0;
    g->num_cpu = numa_num_configured_cpus();
    map_len = (NUMAKIND_NUM_KIND - 1) * g->num_cpu;
    g->arena_map = je_malloc(sizeof(unsigned int) * map_len);
    if (g->arena_map) {
        for (kind_select = 1, j = 0;
             kind_select < NUMAKIND_NUM_KIND && !g->init_err;
             ++kind_select) {
            for (i = 0; i < g->num_cpu && !g->init_err; ++i, ++j) {
                g->init_err = je_mallctl("arenas.extendk", g->arena_map + j,
                                         &unsigned_size, &kind_select,
                                         unsigned_size);
            }
        }
        g->init_err = g->init_err ? NUMAKIND_ERROR_MALLCTL : 0;
    }
    else {
        g->init_err = NUMAKIND_ERROR_MALLOC;
    }
    if (g->init_err) {
        g->num_cpu = 0;
        if (g->arena_map) {
            je_free(g->arena_map);
            g->arena_map = NULL;
        }
    }
}

static int numakind_getarena(numakind_t kind, int *arena)
{
    int err = 0;
    int cpu_id, arena_index;
    struct numakind_arena_map_t *g = &numakind_arena_map_g;
    pthread_once(&numakind_arena_map_once_g, numakind_arena_map_init);

    if (!g->init_err) {
        cpu_id = sched_getcpu();
        if (cpu_id >= g->num_cpu) {
            err = NUMAKIND_ERROR_GETCPU;
        }
        else {
            arena_index = ((int)kind - 1) * g->num_cpu + cpu_id;
            *arena = g->arena_map[arena_index];
        }
    }
    else {
        err = g->init_err;
    }
    return err;
}
