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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <jemalloc/jemalloc.h>
#include <utmpx.h>
#include <sched.h>
#include <smmintrin.h>
#include <limits.h>
#include <sys/mman.h>

#include "config.h"
#include "config_tls.h"
#include <memkind.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_arena.h>

extern struct memkind *arena_registry_g[MEMKIND_MAX_ARENA];

static void *jemk_mallocx_check(size_t size, int flags);
static void *jemk_rallocx_check(void *ptr, size_t size, int flags);


void *arena_chunk_alloc(void *chunk, size_t size, size_t alignment,
                       bool *zero, bool *commit, unsigned arena_ind)
{
    int err;
    void *addr = NULL;

    struct memkind *kind;
    err = memkind_get_kind_by_arena(arena_ind, &kind);
    if (err) {
        goto exit;
    }

    err = memkind_check_available(kind);
    if (err) {
        goto exit;
    }

    if (kind->ops->mmap) {
        addr = kind->ops->mmap(kind, chunk, size);
    } else {
        addr = memkind_default_mmap(kind, chunk, size);
    }

    if (addr != MAP_FAILED) {
        if (addr != chunk) {
            /* wrong place */
            munmap(addr, size);
            addr = NULL;
            goto exit;
        }

        *zero = true;
        *commit = true;

        if ((uintptr_t)addr & (alignment-1)) {
            /* XXX - fix alignment */
            munmap(addr, size);
            addr = NULL;
        }
    } else {
        addr = NULL;
    }

exit:
    return addr;
}

bool arena_chunk_dalloc(void *chunk, size_t size, bool commited,
                        unsigned arena_ind)
{
    /* do nothing - report failure (opt-out) */
    return true;
}

bool arena_chunk_commit(void *chunk, size_t size, size_t offset, size_t length,
                        unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

bool arena_chunk_decommit(void *chunk, size_t size, size_t offset, size_t length,
                          unsigned arena_ind)
{
    /* do nothing - report failure (opt-out) */
    return true;
}

bool arena_chunk_purge(void *chunk, size_t size, size_t offset, size_t length,
                       unsigned arena_ind)
{
    /* XXX - may need special implementation for hugetlb/gbtbl kinds */
    int err;

    if (offset + length > size) {
        return true;
    }

    err = madvise(chunk + offset, length, MADV_DONTNEED);
    return (err != 0);
}

bool arena_chunk_split(void *chunk, size_t size, size_t size_a, size_t size_b,
                       bool commited, unsigned arena_ind)
{
    /* XXX - may need special implementation for gbtbl kinds */

    /* do nothing - report success */
    return false;
}

bool arena_chunk_merge(void *chunk_a, size_t size_a, void *chunk_b,
                       size_t size_b, bool commited, unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

static chunk_hooks_t arena_chunk_hooks = {
    arena_chunk_alloc,
    arena_chunk_dalloc,
    arena_chunk_commit,
    arena_chunk_decommit,
    arena_chunk_purge,
    arena_chunk_split,
    arena_chunk_merge
};


int memkind_arena_create(struct memkind *kind, const struct memkind_ops *ops, const char *name)
{
    int err = 0;

    err = memkind_default_create(kind, ops, name);
    if (!err) {
        err = memkind_arena_create_map(kind);
    }
    return err;
}

int memkind_set_arena_map_len(struct memkind *kind)
{
    if (kind->ops->get_arena == memkind_bijective_get_arena) {
        kind->arena_map_len = 1;

    }
    else if (kind->ops->get_arena == memkind_thread_get_arena) {
        char *arena_num_env = getenv("MEMKIND_ARENA_NUM_PER_KIND");

        if (arena_num_env) {
            unsigned long int arena_num_value = strtoul(arena_num_env, NULL, 10);

            if ((arena_num_value == 0) || (arena_num_value > INT_MAX)) {
                return MEMKIND_ERROR_ENVIRON;
            }

            kind->arena_map_len = arena_num_value;
        }
        else {
            int calculated_arena_num = numa_num_configured_cpus() * 4;

#if ARENA_LIMIT_PER_KIND > 0
            kind->arena_map_len = ((ARENA_LIMIT_PER_KIND < calculated_arena_num) ? ARENA_LIMIT_PER_KIND : calculated_arena_num);
#else
            kind->arena_map_len = calculated_arena_num;
#endif

        }
    }

    return 0;
}

int memkind_arena_create_map(struct memkind *kind)
{
    int err = 0;
    int i;
    size_t unsigned_size = sizeof(unsigned int);
    char cmd[128];
    chunk_hooks_t *hooks;
    size_t hooks_size = sizeof(chunk_hooks_t);

    if (kind->arena_map_len == 0) {
        err = memkind_set_arena_map_len(kind);
        if (err) {
            goto exit;
        }
    }

#ifndef MEMKIND_TLS
    if (kind->ops->get_arena == memkind_thread_get_arena) {
        if (pthread_key_create(&(kind->arena_key), jemk_free)) {
            err = MEMKIND_ERROR_PTHREAD;
            goto exit;
        }
    }
#endif

    if (kind->arena_map_len) {
        kind->arena_map = (unsigned int *)jemk_malloc(sizeof(unsigned int) * kind->arena_map_len);
        if (kind->arena_map == NULL) {
            err = MEMKIND_ERROR_MALLOC;
            goto exit;
        }
        kind->tcache_map = (unsigned int *)jemk_malloc(sizeof(unsigned int) * kind->arena_map_len);
        if (kind->tcache_map == NULL) {
            err = MEMKIND_ERROR_MALLOC;
            goto exit;
        }
    }

    for (i = 0; i < kind->arena_map_len; ++i) {
        kind->arena_map[i] = UINT_MAX;
    }

    for (i = 0; i < kind->arena_map_len; ++i) {
        err = jemk_mallctl("arenas.extend", kind->arena_map + i,
                           &unsigned_size, NULL, 0);
        if (err) {
            err = MEMKIND_ERROR_MALLCTL;
            goto exit;
        }

        err = jemk_mallctl("tcache.create", kind->tcache_map + i,
                           &unsigned_size, NULL, 0);
        if (err) {
            err = MEMKIND_ERROR_MALLCTL;
            goto exit;
        }

        if (kind->ops->chunk_hooks) {
            hooks = kind->ops->chunk_hooks;
        } else {
            hooks = &arena_chunk_hooks;
        }

        snprintf(cmd, 128, "arena.%u.chunk_hooks", kind->arena_map[i]);
        err = jemk_mallctl(cmd, NULL, NULL, hooks, hooks_size);
        if (err) {
            err = MEMKIND_ERROR_MALLCTL;
            goto exit;
        }

        arena_registry_g[kind->arena_map[i]] = kind;
    }
    return err;

exit:
    if (kind->arena_map) {
        jemk_free(kind->arena_map);
    }
    if (kind->tcache_map) {
        jemk_free(kind->tcache_map);
    }
    return err;
}

int memkind_arena_destroy(struct memkind *kind)
{
    char cmd[128];
    int i;

    if (kind->arena_map_len) {
        for (i = 0; i < kind->arena_map_len; ++i) {
            snprintf(cmd, 128, "arena.%u.purge", kind->arena_map[i]);
            jemk_mallctl(cmd, NULL, NULL, NULL, 0);
        }
        jemk_free(kind->arena_map);
        kind->arena_map = NULL;
        jemk_free(kind->tcache_map);
        kind->tcache_map = NULL;
#ifndef MEMKIND_TLS
        if (kind->ops->get_arena == memkind_thread_get_arena) {
            pthread_key_delete(kind->arena_key);
        }
#endif
    }

    memkind_default_destroy(kind);
    return 0;
}

void *memkind_arena_malloc(struct memkind *kind, size_t size)
{
    void *result = NULL;
    int err = 0;
    unsigned int arena;
    unsigned int tcache;

    if (!err) {
        err = kind->ops->get_arena(kind, &arena, &tcache, size);
    }
    if (!err) {
        result = jemk_mallocx_check(size, MALLOCX_ARENA(arena) | MALLOCX_TCACHE(tcache));
    }
    return result;
}

void *memkind_arena_realloc(struct memkind *kind, void *ptr, size_t size)
{
    int err = 0;
    unsigned int arena;
    unsigned int tcache;

    if (size == 0 && ptr != NULL) {
        memkind_free(kind, ptr);
        ptr = NULL;
    }
    else {
        err = kind->ops->get_arena(kind, &arena, &tcache, size);
        if (!err) {
            if (ptr == NULL) {
                ptr = jemk_mallocx_check(size, MALLOCX_ARENA(arena) | MALLOCX_TCACHE(tcache));
            }
            else {
                ptr = jemk_rallocx_check(ptr, size, MALLOCX_ARENA(arena) | MALLOCX_TCACHE(tcache));
            }
        }
    }
    return ptr;
}

void *memkind_arena_calloc(struct memkind *kind, size_t num, size_t size)
{
    void *result = NULL;
    int err = 0;
    unsigned int arena;
    unsigned int tcache;

    err = kind->ops->get_arena(kind, &arena, &tcache, size);
    if (!err) {
        result = jemk_mallocx_check(num * size, MALLOCX_ARENA(arena) | MALLOCX_TCACHE(tcache) | MALLOCX_ZERO);
    }
    return result;
}

int memkind_arena_posix_memalign(struct memkind *kind, void **memptr, size_t alignment,
                                 size_t size)
{
    int err = 0;
    unsigned int arena;
    unsigned int tcache;
    int errno_before;

    *memptr = NULL;
    err = kind->ops->get_arena(kind, &arena, &tcache, size);
    if (!err) {
        err = memkind_posix_check_alignment(kind, alignment);
    }
    if (!err) {
        /* posix_memalign should not change errno.
           Set it to its previous value after calling jemalloc */
        errno_before = errno;
        *memptr = jemk_mallocx_check(size, MALLOCX_ALIGN(alignment) | MALLOCX_ARENA(arena) | MALLOCX_TCACHE(tcache));
        errno = errno_before;
        err = *memptr ? 0 : ENOMEM;
    }
    return err;
}

int memkind_bijective_get_arena(struct memkind *kind, unsigned int *arena, unsigned int *tcache, size_t size)
{
    int err = 0;

    if (kind->arena_map != NULL) {
        *arena = *(kind->arena_map);
        *tcache = *(kind->tcache_map);
        if (*arena == UINT_MAX) {
            err = MEMKIND_ERROR_MALLCTL;
        }
    }
    else {
        err = MEMKIND_ERROR_RUNTIME;
    }
    return err;
}

#ifdef MEMKIND_TLS
int memkind_thread_get_arena(struct memkind *kind, unsigned int *arena, unsigned int *tcache, size_t size)
{
    static __thread unsigned int MEMKIND_TLS_MODEL arena_tls = UINT_MAX;
    int err = 0;

    if (arena_tls == UINT_MAX) {
        arena_tls = _mm_crc32_u64(0, (uint64_t)pthread_self());
    }
    if (kind->arena_map != NULL) {
        *arena = kind->arena_map[arena_tls % kind->arena_map_len];
        *tcache = kind->tcache_map[arena_tls % kind->arena_map_len];
    }
    else {
        err = MEMKIND_ERROR_RUNTIME;
    }
    return err;
}
#else
int memkind_thread_get_arena(struct memkind *kind, unsigned int *arena, unsigned int *tcache, size_t size)
{
    int err = 0;
    unsigned int *arena_tsd;

    arena_tsd = pthread_getspecific(kind->arena_key);
    if (arena_tsd == NULL) {
        arena_tsd = jemk_malloc(sizeof(unsigned int));
        if (arena_tsd == NULL) {
            err = MEMKIND_ERROR_MALLOC;
        }
        if (!err) {
            *arena_tsd = _mm_crc32_u64(0, (uint64_t)pthread_self()) %
                         kind->arena_map_len;
            err = pthread_setspecific(kind->arena_key, arena_tsd) ?
                  MEMKIND_ERROR_PTHREAD : 0;
        }
    }
    if (!err) {
        *arena = kind->arena_map[*arena_tsd];
        *tcache = kind->tcache_map[*arena_tsd];
        if (*arena == UINT_MAX) {
            err = MEMKIND_ERROR_MALLCTL;
        }
    }
    return err;
}
#endif /* MEMKIND_TLS */

static void *jemk_mallocx_check(size_t size, int flags)
{
    /*
     * Checking for out of range size due to unhandled error in
     * jemk_mallocx().  Size invalid for the range
     * LLONG_MAX <= size <= ULLONG_MAX
     * which is the result of passing a negative signed number as size
     */
    void *result = NULL;

    if (size >= LLONG_MAX) {
        errno = ENOMEM;
    }
    else if (size != 0) {
        result = jemk_mallocx(size, flags);
    }
    return result;
}

static void *jemk_rallocx_check(void *ptr, size_t size, int flags)
{
    /*
     * Checking for out of range size due to unhandled error in
     * jemk_mallocx().  Size invalid for the range
     * LLONG_MAX <= size <= ULLONG_MAX
     * which is the result of passing a negative signed number as size
     */
    void *result = NULL;

    if (size >= LLONG_MAX) {
        errno = ENOMEM;
    }
    else {
        result = jemk_rallocx(ptr, size, flags);
    }
    return result;
}
