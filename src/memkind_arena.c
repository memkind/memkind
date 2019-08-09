/*
 * Copyright (C) 2014 - 2019 Intel Corporation.
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
#include <memkind/internal/memkind_log.h>

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
#include <limits.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <assert.h>

#include "config.h"

#define HUGE_PAGE_SIZE (1ull << MEMKIND_MASK_PAGE_SIZE_2MB)

static void *jemk_mallocx_check(size_t size, int flags);
static void *jemk_rallocx_check(void *ptr, size_t size, int flags);
static void tcache_finalize(void *args);

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

MEMKIND_EXPORT int memkind_set_arena_map_len(struct memkind *kind)
{
    if (kind->ops->get_arena == memkind_bijective_get_arena) {
        kind->arena_map_len = 1;
    } else if (kind->ops->get_arena == memkind_thread_get_arena) {
        char *arena_num_env = getenv("MEMKIND_ARENA_NUM_PER_KIND");

        if (arena_num_env) {
            unsigned long int arena_num_value = strtoul(arena_num_env, NULL, 10);

            if ((arena_num_value == 0) || (arena_num_value > INT_MAX)) {
                log_err("Wrong MEMKIND_ARENA_NUM_PER_KIND environment value: %lu.",
                        arena_num_value);
                return MEMKIND_ERROR_ENVIRON;
            }

            kind->arena_map_len = arena_num_value;
        } else {
            int calculated_arena_num = numa_num_configured_cpus() * 4;

#if ARENA_LIMIT_PER_KIND != 0
            calculated_arena_num = MIN((ARENA_LIMIT_PER_KIND), calculated_arena_num);
#endif
            kind->arena_map_len = calculated_arena_num;
        }

        kind->arena_map_len = round_pow2_up(kind->arena_map_len);
    }

    kind->arena_map_mask = kind->arena_map_len - 1;
    return 0;
}

static pthread_once_t arena_config_once = PTHREAD_ONCE_INIT;
static int arena_init_status;

static pthread_key_t tcache_key;
static bool memkind_hog_memory;

static void arena_config_init()
{
    const char *str = getenv("MEMKIND_HOG_MEMORY");
    memkind_hog_memory = str && str[0] == '1';

    arena_init_status = pthread_key_create(&tcache_key, tcache_finalize);
}

#define MALLOCX_ARENA_MAX           0xffe        // copy-pasted from jemalloc/internal/jemalloc_internal.h
#define DIRTY_DECAY_MS_DEFAULT      10000        // copy-pasted from jemalloc/internal/arena_types.h DIRTY_DECAY_MS_DEFAULT
#define DIRTY_DECAY_MS_CONSERVATIVE     0

static struct memkind *arena_registry_g[MALLOCX_ARENA_MAX];
static pthread_mutex_t arena_registry_write_lock;

struct memkind *get_kind_by_arena(unsigned arena_ind)
{
    // there is no way to obtain MALLOCX_ARENA_MAX from jemalloc
    // so this checks if arena_ind does not exceed assumed range
    assert(arena_ind < MALLOCX_ARENA_MAX);

    return arena_registry_g[arena_ind];
}

// Allocates size bytes aligned to alignment. Returns NULL if allocation fails.
static void *alloc_aligned_slow(size_t size, size_t alignment,
                                struct memkind *kind)
{
    size_t extended_size = size + alignment;
    void *ptr;

    ptr = kind_mmap(kind,  NULL, extended_size);

    if(ptr == MAP_FAILED) {
        return NULL;
    }

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned_addr = (addr + alignment) & ~(alignment - 1);

    size_t head_len = aligned_addr - addr;
    if (head_len > 0) {
        munmap(ptr, head_len);
    }

    uintptr_t tail = aligned_addr + size;
    size_t tail_len = (addr + extended_size) - (aligned_addr + size);
    if (tail_len > 0) {
        munmap((void *)tail, tail_len);
    }

    return (void *)aligned_addr;
}


void *arena_extent_alloc(extent_hooks_t *extent_hooks,
                         void *new_addr,
                         size_t size,
                         size_t alignment,
                         bool *zero,
                         bool *commit,
                         unsigned arena_ind)
{
    int err;
    void *addr = NULL;

    struct memkind *kind = get_kind_by_arena(arena_ind);

    err = memkind_check_available(kind);
    if (err) {
        return NULL;
    }

    addr = kind_mmap(kind, new_addr, size);
    if (addr == MAP_FAILED) {
        return NULL;
    }

    if (new_addr != NULL && addr != new_addr) {
        /* wrong place */
        munmap(addr, size);
        return NULL;
    }

    if ((uintptr_t)addr & (alignment-1)) {
        munmap(addr, size);
        addr = alloc_aligned_slow(size, alignment, kind);
        if(addr == NULL) {
            return NULL;
        }
    }

    *zero = true;
    *commit = true;

    return addr;
}

void *arena_extent_alloc_hugetlb(extent_hooks_t *extent_hooks,
                                 void *new_addr,
                                 size_t size,
                                 size_t alignment,
                                 bool *zero,
                                 bool *commit,
                                 unsigned arena_ind)
{
    //round up to huge page size
    size = (size + (HUGE_PAGE_SIZE - 1)) & ~(HUGE_PAGE_SIZE - 1);
    return arena_extent_alloc(extent_hooks, new_addr, size, alignment, zero, commit,
                              arena_ind);
}

bool arena_extent_dalloc(extent_hooks_t *extent_hooks,
                         void *addr,
                         size_t size,
                         bool committed,
                         unsigned arena_ind)
{
    return true;
}

bool arena_extent_commit(extent_hooks_t *extent_hooks,
                         void *addr,
                         size_t size,
                         size_t offset,
                         size_t length,
                         unsigned arena_ind)
{
    return false;
}

bool arena_extent_decommit(extent_hooks_t *extent_hooks,
                           void *addr,
                           size_t size,
                           size_t offset,
                           size_t length,
                           unsigned arena_ind)
{
    return true;
}

bool arena_extent_purge(extent_hooks_t *extent_hooks,
                        void *addr,
                        size_t size,
                        size_t offset,
                        size_t length,
                        unsigned arena_ind)
{
    int err;

    if (memkind_hog_memory) {
        return true;
    }

    err = madvise(addr + offset, length, MADV_DONTNEED);
    return (err != 0);
}

bool arena_extent_split(extent_hooks_t *extent_hooks,
                        void *addr,
                        size_t size,
                        size_t size_a,
                        size_t size_b,
                        bool committed,
                        unsigned arena_ind)
{
    return false;
}

bool arena_extent_merge(extent_hooks_t *extent_hooks,
                        void *addr_a,
                        size_t size_a,
                        void *addr_b,
                        size_t size_b,
                        bool committed,
                        unsigned arena_ind)
{
    return false;
}

extent_hooks_t arena_extent_hooks = {
    .alloc = arena_extent_alloc,
    .dalloc = arena_extent_dalloc,
    .commit = arena_extent_commit,
    .decommit = arena_extent_decommit,
    .purge_lazy = arena_extent_purge,
    .split = arena_extent_split,
    .merge = arena_extent_merge
};

extent_hooks_t arena_extent_hooks_hugetlb = {
    .alloc = arena_extent_alloc_hugetlb,
    .dalloc = arena_extent_dalloc,
    .commit = arena_extent_commit,
    .decommit = arena_extent_decommit,
    .purge_lazy = arena_extent_purge,
    .split = arena_extent_split,
    .merge = arena_extent_merge
};

extent_hooks_t *get_extent_hooks_by_kind(struct memkind *kind)
{
    if (kind == MEMKIND_HUGETLB
        || kind == MEMKIND_HBW_HUGETLB
        || kind == MEMKIND_HBW_ALL_HUGETLB
        || kind == MEMKIND_HBW_PREFERRED_HUGETLB) {
        return &arena_extent_hooks_hugetlb;
    } else {
        return &arena_extent_hooks;
    }
}

MEMKIND_EXPORT int memkind_arena_create_map(struct memkind *kind,
                                            extent_hooks_t *hooks)
{
    int err = 0;
    size_t unsigned_size = sizeof(unsigned int);

    pthread_once(&arena_config_once, arena_config_init);
    if(arena_init_status) {
        return arena_init_status;
    }

    err = memkind_set_arena_map_len(kind);
    if(err) {
        return err;
    }
#ifdef MEMKIND_TLS
    if (kind->ops->get_arena == memkind_thread_get_arena) {
        pthread_key_create(&(kind->arena_key), jemk_free);
    }
#endif

    if (pthread_mutex_lock(&arena_registry_write_lock) != 0)
        assert(0 && "failed to acquire mutex");
    unsigned i = 0;
    kind->arena_zero = UINT_MAX;
    for(i = 0; i<kind->arena_map_len; i++) {
        //create new arena with consecutive index
        unsigned arena_index;
        err = jemk_mallctl("arenas.create", (void *)&arena_index, &unsigned_size, NULL,
                           0);
        if(err) {
            log_err("Could not create arena.");
            err = MEMKIND_ERROR_ARENAS_CREATE;
            goto exit;
        }
        //store arena with lowest index (arenas could be created in descending/ascending order)
        if(kind->arena_zero > arena_index) {
            kind->arena_zero = arena_index;
        }
        //setup extent_hooks for newly created arena
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "arena.%u.extent_hooks", arena_index);
        err = jemk_mallctl(cmd, NULL, NULL, (void *)&hooks, sizeof(extent_hooks_t *));
        if(err) {
            goto exit;
        }
        arena_registry_g[arena_index] = kind;
    }

exit:
    if (pthread_mutex_unlock(&arena_registry_write_lock) != 0)
        assert(0 && "failed to release mutex");
    return err;
}

MEMKIND_EXPORT int memkind_arena_create(struct memkind *kind,
                                        struct memkind_ops *ops, const char *name)
{
    int err = 0;

    err = memkind_default_create(kind, ops, name);
    if (!err) {
        err = memkind_arena_create_map(kind, get_extent_hooks_by_kind(kind));
    }
    return err;
}

MEMKIND_EXPORT int memkind_arena_destroy(struct memkind *kind)
{
    char cmd[128];
    unsigned int i;

    if (kind->arena_map_len) {

        if (pthread_mutex_lock(&arena_registry_write_lock) != 0)
            assert(0 && "failed to acquire mutex");

        for (i = 0; i < kind->arena_map_len; ++i) {
            snprintf(cmd, 128, "arena.%u.destroy", kind->arena_zero + i);
            jemk_mallctl(cmd, NULL, NULL, NULL, 0);
            arena_registry_g[kind->arena_zero + i] = NULL;
        }

        if (pthread_mutex_unlock(&arena_registry_write_lock) != 0)
            assert(0 && "failed to release mutex");

#ifdef MEMKIND_TLS
        if (kind->ops->get_arena == memkind_thread_get_arena) {
            pthread_key_delete(kind->arena_key);
        }
#endif
    }

    memkind_default_destroy(kind);
    return 0;
}

int memkind_arena_finalize(struct memkind *kind)
{
    return memkind_arena_destroy(kind);
}

// max allocation size to be cached by tcache mechanism
// should be aligned with jemalloc opt.lg_tcache_max
#define TCACHE_MAX (1<<12)

static void tcache_finalize(void *args)
{
    int i;
    unsigned *tcache_map = args;
    for(i = 0; i<MEMKIND_NUM_BASE_KIND; i++) {
        if(tcache_map[i] != 0) {
            jemk_mallctl("tcache.destroy", NULL, NULL, (void *)&tcache_map[i],
                         sizeof(unsigned));
        }
    }
}

static inline int memkind_lookup_arena(void *ptr, unsigned int *arena)
{
    size_t sz = sizeof(unsigned);
    unsigned temp_arena;
    int err = jemk_mallctl("arenas.lookup", &temp_arena, &sz, &ptr, sizeof(ptr));

    if (err) {
        log_err("Could not found arena, err=%d", err);
        return 1;
    }

    *arena = temp_arena;
    return 0;
}

MEMKIND_EXPORT struct memkind *memkind_arena_detect_kind(void *ptr)
{
    if (!ptr) {
        return NULL;
    }
    struct memkind *kind = NULL;
    unsigned arena;
    int err = memkind_lookup_arena(ptr, &arena);
    if (MEMKIND_LIKELY(!err)) {
        kind = get_kind_by_arena(arena);
    }

    /* if no kind was associated with arena it means that allocation doesn't come from
       jemk_*allocx API - it is jemk_*alloc API (MEMKIND_DEFAULT) */

    return (kind) ? kind : MEMKIND_DEFAULT;
}

static inline int get_tcache_flag(unsigned partition, size_t size)
{

    // do not cache allocation larger than tcache_max nor those comming from non-static kinds
    if(size > TCACHE_MAX || partition >= MEMKIND_NUM_BASE_KIND) {
        return MALLOCX_TCACHE_NONE;
    }

    unsigned *tcache_map = pthread_getspecific(tcache_key);
    if(tcache_map == NULL) {
        tcache_map = jemk_calloc(MEMKIND_NUM_BASE_KIND, sizeof(unsigned));
        if(tcache_map == NULL) {
            return MALLOCX_TCACHE_NONE;
        }
        pthread_setspecific(tcache_key, (void *)tcache_map);
    }

    if(MEMKIND_UNLIKELY(tcache_map[partition] == 0)) {
        size_t unsigned_size = sizeof(unsigned);
        int err = jemk_mallctl("tcache.create", (void *)&tcache_map[partition],
                               &unsigned_size, NULL, 0);
        if(err) {
            log_err("Could not acquire tcache, err=%d", err);
            return MALLOCX_TCACHE_NONE;
        }
    }
    return MALLOCX_TCACHE(tcache_map[partition]);
}

MEMKIND_EXPORT void *memkind_arena_malloc(struct memkind *kind, size_t size)
{
    void *result = NULL;
    int err = 0;
    unsigned int arena;

    err = kind->ops->get_arena(kind, &arena, size);
    if (MEMKIND_LIKELY(!err)) {
        result = jemk_mallocx_check(size,
                                    MALLOCX_ARENA(arena) | get_tcache_flag(kind->partition, size));
    }
    return result;
}

MEMKIND_EXPORT void memkind_arena_free(struct memkind *kind, void *ptr)
{
    if (kind == MEMKIND_DEFAULT) {
        jemk_free(ptr);
    } else if (ptr != NULL) {
        jemk_dallocx(ptr, get_tcache_flag(kind->partition, 0));
    }
}

MEMKIND_EXPORT void memkind_arena_free_with_kind_detect(void *ptr)
{
    struct memkind *kind = memkind_arena_detect_kind(ptr);

    memkind_arena_free(kind, ptr);
}

MEMKIND_EXPORT size_t memkind_arena_malloc_usable_size(void *ptr)
{
    return memkind_default_malloc_usable_size(NULL, ptr);
}

MEMKIND_EXPORT void *memkind_arena_realloc(struct memkind *kind, void *ptr,
                                           size_t size)
{
    int err = 0;
    unsigned int arena;

    if (size == 0 && ptr != NULL) {
        memkind_arena_free(kind, ptr);
        ptr = NULL;
    } else {
        err = kind->ops->get_arena(kind, &arena, size);
        if (MEMKIND_LIKELY(!err)) {
            if (ptr == NULL) {
                ptr = jemk_mallocx_check(size,
                                         MALLOCX_ARENA(arena) | get_tcache_flag(kind->partition, size));
            } else {
                ptr = jemk_rallocx_check(ptr, size,
                                         MALLOCX_ARENA(arena) | get_tcache_flag(kind->partition, size));
            }
        }
    }
    return ptr;
}

MEMKIND_EXPORT void *memkind_arena_realloc_with_kind_detect(void *ptr,
                                                            size_t size)
{
    if (!ptr) {
        errno = EINVAL;
        return NULL;
    }
    struct memkind *kind = memkind_arena_detect_kind(ptr);
    if (kind == MEMKIND_DEFAULT) {
        return memkind_default_realloc(kind, ptr, size);
    } else {
        return memkind_arena_realloc(kind, ptr, size);
    }
}

MEMKIND_EXPORT int memkind_arena_update_memory_usage_policy(
    struct memkind *kind, memkind_mem_usage_policy policy)
{
    int err = MEMKIND_SUCCESS;
    unsigned i;
    ssize_t dirty_decay_val = 0;
    switch ( policy ) {
        case MEMKIND_MEM_USAGE_POLICY_DEFAULT:
            dirty_decay_val = DIRTY_DECAY_MS_DEFAULT;
            break;
        case MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE:
            dirty_decay_val = DIRTY_DECAY_MS_CONSERVATIVE;
            break;
        default:
            log_err("Unrecognized memory policy %d", policy);
            return MEMKIND_ERROR_INVALID;
    }

    for (i = 0; i < kind->arena_map_len; ++i) {
        char cmd[64];

        snprintf(cmd, sizeof(cmd), "arena.%u.dirty_decay_ms", kind->arena_zero + i);
        err = jemk_mallctl(cmd, NULL, NULL, (void *)&dirty_decay_val, sizeof(ssize_t));
        if ( err ) {
            log_err("Incorrect dirty_decay_ms value %zu", dirty_decay_val);
            return MEMKIND_ERROR_INVALID;
        }
    }

    return err;
}

MEMKIND_EXPORT int memkind_arena_update_background_thread(
    struct memkind *kind, memkind_background_thread background_thread)
{
    int err = MEMKIND_SUCCESS;
    bool background_thread_val = 0;
    switch ( background_thread ) {
        case MEMKIND_BACKGROUND_THREAD_DEFAULT:
            background_thread_val = 0;
            break;
        case MEMKIND_BACKGROUND_THREAD_ENABLED:
            background_thread_val = 1;
            break;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "background_thread");
    err = jemk_mallctl(cmd, NULL, NULL, &background_thread_val, sizeof(bool));
    if ( err ) {
        log_err("Error on setting background thread");
        return MEMKIND_ERROR_INVALID;
    }

    return err;
}

// TODO: function is workaround for PR#1302 in jemalloc upstream
// and it should be removed/replaced with memkind_arena_calloc()
// after PR will be merged
MEMKIND_EXPORT void *memkind_arena_pmem_calloc(struct memkind *kind, size_t num,
                                               size_t size)
{
    void *result = NULL;
    int err = 0;
    unsigned int arena;

    err = kind->ops->get_arena(kind, &arena, size);
    if (MEMKIND_LIKELY(!err)) {
        result = jemk_mallocx_check(num * size,
                                    MALLOCX_ARENA(arena) | get_tcache_flag(kind->partition, size));
        if (MEMKIND_LIKELY(result)) {
            memset(result, 0, size);
        }
    }
    return result;
}

MEMKIND_EXPORT void *memkind_arena_calloc(struct memkind *kind, size_t num,
                                          size_t size)
{
    void *result = NULL;
    int err = 0;
    unsigned int arena;

    err = kind->ops->get_arena(kind, &arena, size);
    if (MEMKIND_LIKELY(!err)) {
        result = jemk_mallocx_check(num * size,
                                    MALLOCX_ARENA(arena) | MALLOCX_ZERO | get_tcache_flag(kind->partition, size));
    }
    return result;
}

MEMKIND_EXPORT int memkind_arena_posix_memalign(struct memkind *kind,
                                                void **memptr, size_t alignment,
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
        if (MEMKIND_UNLIKELY(size_out_of_bounds(size))) {
            return 0;
        }
        /* posix_memalign should not change errno.
           Set it to its previous value after calling jemalloc */
        errno_before = errno;
        *memptr = jemk_mallocx_check(size,
                                     MALLOCX_ALIGN(alignment) | MALLOCX_ARENA(arena) | get_tcache_flag(
                                         kind->partition, size));
        errno = errno_before;
        err = *memptr ? 0 : ENOMEM;
    }
    return err;
}

MEMKIND_EXPORT int memkind_bijective_get_arena(struct memkind *kind,
                                               unsigned int *arena, size_t size)
{
    *arena = kind->arena_zero;
    return 0;
}

#ifdef MEMKIND_TLS
MEMKIND_EXPORT int memkind_thread_get_arena(struct memkind *kind,
                                            unsigned int *arena, size_t size)
{
    int err = 0;
    unsigned int *arena_tsd;
    arena_tsd = pthread_getspecific(kind->arena_key);

    if (MEMKIND_UNLIKELY(arena_tsd == NULL)) {
        arena_tsd = jemk_malloc(sizeof(unsigned int));
        if (arena_tsd == NULL) {
            err = MEMKIND_ERROR_MALLOC;
            log_err("jemk_malloc() failed.");
        }
        if (!err) {
            // On glibc pthread_self() is incremented by 0x801000 for every
            // thread (no matter the arch's word width).  This might change
            // in the future, but even in the worst case the hash will
            // degenerate to a single bucket with no loss of correctness.
            *arena_tsd = ((uint64_t)pthread_self() >> 12) %
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
 * This approach works only on glibc (and possibly similar implementations)
 * but that covers our current needs.
 *
 */
static uintptr_t get_fs_base()
{
    return (uintptr_t)pthread_self();
}

MEMKIND_EXPORT int memkind_thread_get_arena(struct memkind *kind,
                                            unsigned int *arena, size_t size)
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
    } else if (size != 0) {
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
    } else {
        result = jemk_rallocx(ptr, size, flags);
    }
    return result;

}

void memkind_arena_init(struct memkind *kind)
{
    int err = 0;
    if (kind != MEMKIND_DEFAULT) {
        err = memkind_arena_create_map(kind, get_extent_hooks_by_kind(kind));
        if (err) {
            log_fatal("[%s] Failed to create arena map (error code:%d).", kind->name, err);
            abort();
        }
    }
}
