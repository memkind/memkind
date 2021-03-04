// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "../config.h"
#include <memkind_memtier.h>

#include <tiering/ctl.h>
#include <tiering/memtier_log.h>

#include <pthread.h>

#define MEMTIER_EXPORT __attribute__((visibility("default")))
#define MEMTIER_INIT   __attribute__((constructor))
#define MEMTIER_FINI   __attribute__((destructor))

#define MEMTIER_LIKELY(x)   __builtin_expect((x), 1)
#define MEMTIER_UNLIKELY(x) __builtin_expect((x), 0)

static int initialized = 0;
static int destructed = 0;

// TODO create structure to keep kind + multiple tiers
struct memtier_kind *current_kind;
struct memtier_tier *current_tier;

MEMTIER_EXPORT void *malloc(size_t size)
{
    void *ret = NULL;

    if (MEMTIER_LIKELY(initialized)) {
        ret = memtier_kind_malloc(current_kind, size);
    } else if (destructed == 0) {
        ret = memkind_malloc(MEMKIND_DEFAULT, size);
    }

#ifdef MEMKIND_DECORATION_ENABLED
    log_debug("malloc(%zu) = %p", size, ret);
#endif
    return ret;
}

MEMTIER_EXPORT void *calloc(size_t num, size_t size)
{
    void *ret = NULL;

    if (MEMTIER_LIKELY(initialized)) {
        ret = memtier_kind_calloc(current_kind, num, size);
    } else if (destructed == 0) {
        ret = memkind_calloc(MEMKIND_DEFAULT, num, size);
    }

#ifdef MEMKIND_DECORATION_ENABLED
    log_debug("calloc(%zu, %zu) = %p", num, size, ret);
#endif
    return ret;
}

MEMTIER_EXPORT void *realloc(void *ptr, size_t size)
{
    void *ret = NULL;

    if (MEMTIER_LIKELY(initialized)) {
        ret = memtier_kind_realloc(current_kind, ptr, size);
    } else if (destructed == 0) {
        ret = memkind_realloc(MEMKIND_DEFAULT, ptr, size);
    }

#ifdef MEMKIND_DECORATION_ENABLED
    log_debug("realloc(%p, %zu) = %p", ptr, size, ret);
#endif
    return ret;
}

// clang-format off
MEMTIER_EXPORT int posix_memalign(void **memptr, size_t alignment, size_t size)
{
    int ret = 0;

    if (MEMTIER_LIKELY(initialized)) {
        ret = memtier_kind_posix_memalign(current_kind, memptr, alignment,
                                          size);
    } else if (destructed == 0) {
        ret = memkind_posix_memalign(MEMKIND_DEFAULT, memptr, alignment,
                                     size);
    }

#ifdef MEMKIND_DECORATION_ENABLED
    log_debug("posix_memalign(%p, %zu, %zu) = %d",
              memptr, alignment, size, ret);
#endif
    return ret;
}
// clang-format on

MEMTIER_EXPORT void free(void *ptr)
{
#ifdef MEMKIND_DECORATION_ENABLED
    log_debug("free(%p)", ptr);
#endif

    if (MEMTIER_LIKELY(initialized)) {
        memtier_free(ptr);
    } else if (destructed == 0) {
        memkind_free(MEMKIND_DEFAULT, ptr);
    }
}

static int create_tiered_kind_from_env(char *env_var_string)
{
    char *kind_name = NULL;
    char *pmem_path = NULL;
    char *pmem_size = NULL;
    unsigned ratio_value = -1;
    int ret = ctl_load_config(env_var_string, &kind_name, &pmem_path,
                              &pmem_size, &ratio_value);
    if (ret != 0) {
        return -1;
    }

    log_debug("kind_name: %s", kind_name);
    log_debug("pmem_path: %s", pmem_path);
    log_debug("pmem_size: %s", pmem_size);
    log_debug("ratio_value: %u", ratio_value);

    // TODO add "DRAM" -> MEMKIND_DEFAULT etc. mapping logic
    current_tier = memtier_tier_new(MEMKIND_DEFAULT);
    if (current_tier == NULL) {
        return -1;
    }

    struct memtier_builder *builder = memtier_builder();
    if (builder == NULL) {
        return -1;
    }

    ret = memtier_builder_add_tier(builder, current_tier, ratio_value);
    if (ret == -1) {
        // TODO call sth like buildier_delete?
        return -1;
    }

    ret = memtier_builder_set_policy(builder, MEMTIER_DUMMY_VALUE);
    if (ret == -1) {
        return -1;
    }

    return memtier_builder_construct_kind(builder, &current_kind);
}

static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static MEMTIER_INIT void memtier_init(void)
{
    pthread_once(&init_once, log_init_once);
    log_info("Memkind memtier lib loaded!");

    // TODO: Handle failure when this variable (or config variable) is not
    // present
    char *env_var = utils_get_env("MEMKIND_MEM_TIERING_CONFIG");
    if (env_var) {
        int ret = create_tiered_kind_from_env(env_var);
        if (ret != 0) {
            log_err("Couldn't load MEMKIND_MEM_TIERING_CONFIG env var");
        } else {
            initialized = 1;
        }
    }
}

static MEMTIER_FINI void memtier_fini(void)
{
    log_info("Unloading memkind memtier lib!");

    if (current_kind) {
        // TODO currently we handle one tier
        memtier_tier_delete(current_tier);
        memtier_delete_kind(current_kind);
    }

    destructed = 1;
    initialized = 0;
}
