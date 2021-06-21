// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "../config.h"
#include <memkind/internal/memkind_memtier.h>

#include <tiering/ctl.h>
#include <tiering/memtier_log.h>

#include <pthread.h>
#include <string.h>

#define MEMTIER_EXPORT __attribute__((visibility("default")))
#define MEMTIER_INIT   __attribute__((constructor))
#define MEMTIER_FINI   __attribute__((destructor))

#define MEMTIER_LIKELY(x)   __builtin_expect(!!(x), 1)
#define MEMTIER_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define MT_SYMBOL2(a, b) a##b
#define MT_SYMBOL1(a, b) MT_SYMBOL2(a, b)
#define MT_SYMBOL(b)     MT_SYMBOL1(MEMTIER_ALLOC_PREFIX, b)

#define mt_malloc             MT_SYMBOL(malloc)
#define mt_calloc             MT_SYMBOL(calloc)
#define mt_realloc            MT_SYMBOL(realloc)
#define mt_free               MT_SYMBOL(free)
#define mt_posix_memalign     MT_SYMBOL(posix_memalign)
#define mt_malloc_usable_size MT_SYMBOL(malloc_usable_size)

#ifdef MEMKIND_DECORATION_ENABLED
#include <memkind/internal/memkind_private.h>
MEMTIER_EXPORT void memtier_kind_malloc_post(struct memkind *kind, size_t size,
                                             void **result)
{
    log_info("kind: %s, malloc:(%zu) = %p", kind->name, size, *result);
}

MEMTIER_EXPORT void memtier_kind_calloc_post(memkind_t kind, size_t num,
                                             size_t size, void **result)
{
    log_info("kind: %s, calloc:(%zu, %zu) = %p", kind->name, num, size,
             *result);
}

MEMTIER_EXPORT void memtier_kind_realloc_post(struct memkind *kind, void *ptr,
                                              size_t size, void **result)
{
    log_info("kind: %s, realloc(%p, %zu) = %p", kind->name, ptr, size, *result);
}

MEMTIER_EXPORT void memtier_kind_posix_memalign_post(memkind_t kind,
                                                     void **memptr,
                                                     size_t alignment,
                                                     size_t size, int *err)
{
    log_info("kind: %s, posix_memalign(%p, %zu, %zu) = %d", kind->name, *memptr,
             alignment, size, err);
}

MEMTIER_EXPORT void memtier_kind_free_pre(void **ptr)
{
    struct memkind *kind = memkind_detect_kind(*ptr);
    if (kind)
        log_info("kind: %s, free(%p)", kind->name, *ptr);
    else
        log_info("free(%p)", *ptr);
}

MEMTIER_EXPORT void memtier_kind_usable_size_post(void **ptr, size_t size)
{
    struct memkind *kind = memkind_detect_kind(*ptr);
    if (kind)
        log_info("kind: %s, malloc_usable_size(%p) = %zu", kind->name, *ptr,
                 size);
    else
        log_info("malloc_usable_size(%p) = %zu", *ptr, size);
}
#endif

static int destructed;

static struct memtier_memory *current_memory;

MEMTIER_EXPORT void *malloc(size_t size)
{
    if (MEMTIER_LIKELY(current_memory)) {
        return memtier_malloc(current_memory, size);
    } else if (destructed == 0) {
        return memkind_malloc(MEMKIND_DEFAULT, size);
    }
    return NULL;
}

MEMTIER_EXPORT void *calloc(size_t num, size_t size)
{
    if (MEMTIER_LIKELY(current_memory)) {
        return memtier_calloc(current_memory, num, size);
    } else if (destructed == 0) {
        return memkind_calloc(MEMKIND_DEFAULT, num, size);
    }
    return NULL;
}

MEMTIER_EXPORT void *realloc(void *ptr, size_t size)
{
    if (MEMTIER_LIKELY(current_memory)) {
        return memtier_realloc(current_memory, ptr, size);
    } else if (destructed == 0) {
        return memkind_realloc(MEMKIND_DEFAULT, ptr, size);
    }
    return NULL;
}

// clang-format off
MEMTIER_EXPORT int posix_memalign(void **memptr, size_t alignment, size_t size)
{
    if (MEMTIER_LIKELY(current_memory)) {
        return memtier_posix_memalign(current_memory, memptr, alignment,
                                          size);
    } else if (destructed == 0) {
        return memkind_posix_memalign(MEMKIND_DEFAULT, memptr, alignment,
                                     size);
    }
    return 0;
}
// clang-format on

MEMTIER_EXPORT void free(void *ptr)
{
    if (MEMTIER_LIKELY(current_memory)) {
        memtier_realloc(current_memory, ptr, 0);
    } else if (destructed == 0) {
        memkind_free(MEMKIND_DEFAULT, ptr);
    }
}

MEMTIER_EXPORT size_t malloc_usable_size(void *ptr)
{
    return memtier_usable_size(ptr);
}

static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static MEMTIER_INIT void memtier_init(void)
{
    pthread_once(&init_once, log_init_once);
    log_info("Memkind memtier lib loaded!");

    char *env_var = utils_get_env("MEMKIND_MEM_TIERS");
    if (env_var) {
        current_memory = ctl_create_tier_memory_from_env(env_var);
        if (current_memory) {
            return;
        }
        log_err("Error with parsing MEMKIND_MEM_TIERS");
    } else {
        log_err("Missing MEMKIND_MEM_TIERS env var");
    }
    abort();
}

static MEMTIER_FINI void memtier_fini(void)
{
    log_info("Unloading memkind memtier lib!");

    ctl_destroy_tier_memory(current_memory);
    current_memory = NULL;

    destructed = 1;
}

MEMTIER_EXPORT void *mt_malloc(size_t size)
{
    return malloc(size);
}

MEMTIER_EXPORT void *mt_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

MEMTIER_EXPORT void *mt_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

MEMTIER_EXPORT void mt_free(void *ptr)
{
    free(ptr);
}

MEMTIER_EXPORT int mt_posix_memalign(void **memptr, size_t alignment,
                                     size_t size)
{
    return posix_memalign(memptr, alignment, size);
}

MEMTIER_EXPORT size_t mt_malloc_usable_size(void *ptr)
{
    return malloc_usable_size(ptr);
}
