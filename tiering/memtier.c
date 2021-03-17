// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "../config.h"
#include <memkind_memtier.h>

#include <tiering/ctl.h>
#include <tiering/memtier_log.h>

#include <pthread.h>
#include <string.h>

// TODO - remove this after logging cleanup
#include <memkind/internal/memkind_private.h>

#define MEMTIER_EXPORT __attribute__((visibility("default")))
#define MEMTIER_INIT   __attribute__((constructor))
#define MEMTIER_FINI   __attribute__((destructor))

#define MEMTIER_LIKELY(x)   __builtin_expect((x), 1)
#define MEMTIER_UNLIKELY(x) __builtin_expect((x), 0)

static int initialized;
static int destructed;

// TODO create structure to keep kind + multiple tiers
static struct memtier_kind *current_kind;
static struct memtier_tier *current_tier;

MEMTIER_EXPORT void *malloc(size_t size)
{
    void *ret = NULL;

    if (MEMTIER_LIKELY(initialized)) {
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

    if (MEMTIER_LIKELY(initialized)) {
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

    if (MEMTIER_LIKELY(initialized)) {
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

    if (MEMTIER_LIKELY(initialized)) {
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

    if (MEMTIER_LIKELY(initialized)) {
        memtier_free(ptr);
    } else if (destructed == 0) {
        memkind_free(MEMKIND_DEFAULT, ptr);
    }
}

static memkind_t get_kind(const char *str, const char *pmem_path,
                          const char *pmem_size)
{
    memkind_t kind = NULL;
    if (strcmp(str, "DRAM") == 0) {
        kind = MEMKIND_DEFAULT;
    } else if (strcmp(str, "FS_DAX") == 0) {
        // TODO handle FS_DAX here
    }

    log_debug("kind_name: %s", kind->name);
    log_debug("pmem_path: %s", pmem_path);
    log_debug("pmem_size: %s", pmem_size);

    return kind;
}

// TODO move this logic to ctl
static int create_tiered_kind_from_env(char *env_var_string)
{
    char *kind_name = NULL;
    char *pmem_path = NULL;
    char *pmem_size = NULL;
    unsigned ratio_value = -1;
    memtier_policy_t policy = MEMTIER_POLICY_MAX_VALUE;

    int ret = ctl_load_config(env_var_string, &kind_name, &pmem_path,
                              &pmem_size, &ratio_value, &policy);
    if (ret != 0) {
        return -1;
    }

    memkind_t kind = get_kind(kind_name, pmem_path, pmem_size);
    if (kind == NULL) {
        return -1;
    }

    log_debug("ratio_value: %u", ratio_value);
    log_debug("policy: %s", ctl_policy_to_str(policy));

    current_tier = memtier_tier_new(kind);
    if (current_tier == NULL) {
        return -1;
    }

    struct memtier_builder *builder = memtier_builder_new();
    if (!builder) {
        ret = -1;
        goto tier_delete;
    }

    ret = memtier_builder_add_tier(builder, current_tier, ratio_value);
    if (ret != 0) {
        goto builder_delete;
    }

    ret = memtier_builder_set_policy(builder, policy);
    if (ret != 0) {
        goto builder_delete;
    }

    ret = memtier_builder_construct_kind(builder, &current_kind);
    if (ret != 0) {
        goto builder_delete;
    }
    memtier_builder_delete(builder);

    return ret;

builder_delete:
    memtier_builder_delete(builder);

tier_delete:
    memtier_tier_delete(current_tier);

    return ret;
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
            abort();
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
