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

#include <memkind.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_private.h>

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

#include "config.h"
#include "config_tls.h"

static void *jemk_mallocx_check(size_t size, int flags);
static void *jemk_rallocx_check(void *ptr, size_t size, int flags);

static unsigned int integer_log2(unsigned int v)
{
    return (sizeof(unsigned) * 8) - (__builtin_clz(v) + 1);
}

static unsigned int round_pow2_up(unsigned int v)
{
        unsigned int v_log2 = integer_log2(v);

        if (v != 1 << v_log2) {
            v = 1 << (v_log2 + 1);
        }
        return v;
}

MEMKIND_EXPORT int memkind_arena_create(struct memkind *kind, const struct memkind_ops *ops, const char *name)
{
    int err = 0;

    err = memkind_default_create(kind, ops, name);
    if (!err) {
        err = memkind_arena_create_map(kind);
    }
    return err;
}

static int min_int(int a, int b)
{
    return a > b ? b : a;
}

MEMKIND_EXPORT int memkind_set_arena_map_len(struct memkind *kind)
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

#if ARENA_LIMIT_PER_KIND != 0
            calculated_arena_num = min_int(ARENA_LIMIT_PER_KIND, calculated_arena_num);
#endif
            kind->arena_map_len = calculated_arena_num;
        }

        kind->arena_map_len = round_pow2_up(kind->arena_map_len);
    }

    kind->arena_map_mask = kind->arena_map_len - 1;
    return 0;
}

MEMKIND_EXPORT int memkind_arena_create_map(struct memkind *kind)
{
    int err = 0;
    size_t unsigned_size = sizeof(unsigned int);
    unsigned int arenas_extendk_params[2];

    err = memkind_set_arena_map_len(kind);
    if(err) {
        return err;
    }
#ifdef MEMKIND_TLS
    if (kind->ops->get_arena == memkind_thread_get_arena) {
        pthread_key_create(&(kind->arena_key), jemk_free);
    }
#endif

    arenas_extendk_params[0] = kind->partition;
    arenas_extendk_params[1] = kind->arena_map_len;


    err = jemk_mallctl("arenas.extendk", &(kind->arena_zero),
            &unsigned_size, arenas_extendk_params,
            sizeof(unsigned int*));

    return err;
}

MEMKIND_EXPORT int memkind_arena_destroy(struct memkind *kind)
{
    char cmd[128];
    int i;

    if (kind->arena_map_len) {
        for (i = 0; i < kind->arena_map_len; ++i) {
            snprintf(cmd, 128, "arena.%u.purge", kind->arena_zero + i);
            jemk_mallctl(cmd, NULL, NULL, NULL, 0);
        }
#ifdef MEMKIND_TLS
        if (kind->ops->get_arena == memkind_thread_get_arena) {
            pthread_key_delete(kind->arena_key);
        }
#endif
    }

    memkind_default_destroy(kind);
    return 0;
}

MEMKIND_EXPORT void *memkind_arena_malloc(struct memkind *kind, size_t size)
{
    void *result = NULL;
    int err = 0;
    unsigned int arena;

    err = kind->ops->get_arena(kind, &arena, size);
    if (MEMKIND_LIKELY(!err)) {
        result = jemk_mallocx_check(size, MALLOCX_ARENA(arena));
    }
    return result;
}

MEMKIND_EXPORT void *memkind_arena_realloc(struct memkind *kind, void *ptr, size_t size)
{
    int err = 0;
    unsigned int arena;

    if (size == 0 && ptr != NULL) {
        memkind_free(kind, ptr);
        ptr = NULL;
    }
    else {
        err = kind->ops->get_arena(kind, &arena, size);
        if (MEMKIND_LIKELY(!err)) {
            if (ptr == NULL) {
                ptr = jemk_mallocx_check(size, MALLOCX_ARENA(arena));
            }
            else {
                ptr = jemk_rallocx_check(ptr, size, MALLOCX_ARENA(arena));
            }
        }
    }
    return ptr;
}

MEMKIND_EXPORT void *memkind_arena_calloc(struct memkind *kind, size_t num, size_t size)
{
    void *result = NULL;
    int err = 0;
    unsigned int arena;

    err = kind->ops->get_arena(kind, &arena, size);
    if (MEMKIND_LIKELY(!err)) {
        result = jemk_mallocx_check(num * size, MALLOCX_ARENA(arena) | MALLOCX_ZERO);
    }
    return result;
}

MEMKIND_EXPORT int memkind_arena_posix_memalign(struct memkind *kind, void **memptr, size_t alignment,
                                 size_t size)
{
    int err = 0;
    unsigned int arena;
    int errno_before;

    *memptr = NULL;
    err = kind->ops->get_arena(kind, &arena, size);
    if (MEMKIND_LIKELY(!err)) {
        err = memkind_posix_check_alignment(kind, alignment);
    }
    if (MEMKIND_LIKELY(!err)) {
        /* posix_memalign should not change errno.
           Set it to its previous value after calling jemalloc */
        errno_before = errno;
        *memptr = jemk_mallocx_check(size, MALLOCX_ALIGN(alignment) | MALLOCX_ARENA(arena));
        errno = errno_before;
        err = *memptr ? 0 : ENOMEM;
    }
    return err;
}

MEMKIND_EXPORT int memkind_bijective_get_arena(struct memkind *kind, unsigned int *arena, size_t size)
{
    *arena = kind->arena_zero;
    return 0;
}

#ifdef MEMKIND_TLS
MEMKIND_EXPORT int memkind_thread_get_arena(struct memkind *kind, unsigned int *arena, size_t size)
{
    int err = 0;
    unsigned int *arena_tsd;
    arena_tsd = pthread_getspecific(kind->arena_key);

    if (MEMKIND_UNLIKELY(arena_tsd == NULL)) {
        arena_tsd = jemk_malloc(sizeof(unsigned int));
        if (arena_tsd == NULL) {
            err = MEMKIND_ERROR_MALLOC;
        }
        if (!err) {
            *arena_tsd = _mm_crc32_u64(0, (uint64_t)pthread_self()) %
                         kind->arena_map_len;
            err = pthread_setspecific(kind->arena_key, arena_tsd) ?
                  MEMKIND_ERROR_RUNTIME : 0;
        }
    }
    *arena = kind->arena_zero + *arena_tsd;
    return err;
}

#else

/*
 *
 * We use thread control block as unique thread identifier
 * For more read: https://www.akkadia.org/drepper/tls.pdf
 * We could consider using rdfsbase when it will arrive to linux kernel
 *
 */
static uintptr_t get_fs_base() {
    uintptr_t fs_base;
    asm ("movq %%fs:0, %0" : "=r" (fs_base));
    return fs_base;
}

MEMKIND_EXPORT int memkind_thread_get_arena(struct memkind *kind, unsigned int *arena, size_t size)
{
    unsigned int arena_idx;
    // it's likely that each thread control block lies on diffrent page
    // so we extracting page number with >> 12 to improve hashing
    arena_idx = (get_fs_base() >> 12) & kind->arena_map_mask;
    *arena = kind->arena_zero + arena_idx;
    return 0;
}
#endif //MEMKIND_TLS

static void *jemk_mallocx_check(size_t size, int flags)
{
    /*
     * Checking for out of range size due to unhandled error in
     * jemk_mallocx().  Size invalid for the range
     * LLONG_MAX <= size <= ULLONG_MAX
     * which is the result of passing a negative signed number as size
     */
    void *result = NULL;

    if (MEMKIND_UNLIKELY(size >= LLONG_MAX)) {
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

    if (MEMKIND_UNLIKELY(size >= LLONG_MAX)) {
        errno = ENOMEM;
    }
    else {
        result = jemk_rallocx(ptr, size, flags);
    }
    return result;

}
