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

static int destructed;

static struct memtier_kind *current_kind;

MEMTIER_EXPORT void *malloc(size_t size)
{
    void *ret = NULL;

    if (MEMTIER_LIKELY(current_kind)) {
        ret = memtier_kind_malloc(current_kind, size);
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

    if (MEMTIER_LIKELY(current_kind)) {
        ret = memtier_kind_calloc(current_kind, num, size);
    } else if (destructed == 0) {
        ret = memkind_calloc(MEMKIND_DEFAULT, num, size);
    }

    log_debug("calloc(%zu, %zu) = %p", num, size, ret);
    return ret;
}

MEMTIER_EXPORT void *realloc(void *ptr, size_t size)
{
    void *ret = NULL;

    if (MEMTIER_LIKELY(current_kind)) {
        ret = memtier_kind_realloc(current_kind, ptr, size);
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

    if (MEMTIER_LIKELY(current_kind)) {
        ret = memtier_kind_posix_memalign(current_kind, memptr, alignment,
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

    if (MEMTIER_LIKELY(current_kind)) {
        memtier_free(ptr);
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

    char *env_var = utils_get_env("MEMKIND_MEM_TIERING_CONFIG");
    if (env_var) {
        current_kind = ctl_create_tier_kind_from_env(env_var);
        if (current_kind) {
            return;
        }
        log_err("Error with parsing MEMKIND_MEM_TIERING_CONFIG");
    } else {
        log_err("Missing MEMKIND_MEM_TIERING_CONFIG env var");
    }
    abort();
}

static MEMTIER_FINI void memtier_fini(void)
{
    log_info("Unloading memkind memtier lib!");

    ctl_destroy_kind(current_kind);
    current_kind = NULL;

    destructed = 1;
}
