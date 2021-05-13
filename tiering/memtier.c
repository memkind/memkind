// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "../config.h"
#include <memkind_memtier.h>

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

static int destructed;

static struct memtier_memory *current_memory;

MEMTIER_EXPORT void *malloc(size_t size)
{
    void *ret = NULL;

    if (MEMTIER_LIKELY(current_memory)) {
        ret = memtier_malloc(current_memory, size);
    } else if (destructed == 0) {
        ret = memkind_malloc(MEMKIND_DEFAULT, size);
    }

    // TODO after implementation of decorators memtier decorators this
    // logging call will be obsolete
    log_debug("malloc(%zu) = %p", size, ret);
    return ret;
}

MEMTIER_EXPORT void *calloc(size_t num, size_t size)
{
    void *ret = NULL;

    if (MEMTIER_LIKELY(current_memory)) {
        ret = memtier_calloc(current_memory, num, size);
    } else if (destructed == 0) {
        ret = memkind_calloc(MEMKIND_DEFAULT, num, size);
    }

    log_debug("calloc(%zu, %zu) = %p", num, size, ret);
    return ret;
}

MEMTIER_EXPORT void *realloc(void *ptr, size_t size)
{
    void *ret = NULL;

    if (MEMTIER_LIKELY(current_memory)) {
        ret = memtier_realloc(current_memory, ptr, size);
    } else if (destructed == 0) {
        ret = memkind_realloc(MEMKIND_DEFAULT, ptr, size);
    }

    log_debug("realloc(%p, %zu) = %p", ptr, size, ret);
    return ret;
}

// clang-format off
MEMTIER_EXPORT int posix_memalign(void **memptr, size_t alignment, size_t size)
{
    int ret = 0;

    if (MEMTIER_LIKELY(current_memory)) {
        ret = memtier_posix_memalign(current_memory, memptr, alignment,
                                          size);
    } else if (destructed == 0) {
        ret = memkind_posix_memalign(MEMKIND_DEFAULT, memptr, alignment,
                                     size);
    }

    log_debug("posix_memalign(%p, %zu, %zu) = %d",
              memptr, alignment, size, ret);
    return ret;
}
// clang-format on

MEMTIER_EXPORT void free(void *ptr)
{
    log_debug("free(%p)", ptr);

    if (MEMTIER_LIKELY(current_memory)) {
        memtier_realloc(current_memory, ptr, 0);
    } else if (destructed == 0) {
        memkind_free(MEMKIND_DEFAULT, ptr);
    }
}

MEMTIER_EXPORT size_t malloc_usable_size(void *ptr)
{
    log_debug("malloc_usable_size(%p)", ptr);

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
