// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2021 Intel Corporation. */

#include <limits.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/tbb_mem_pool_policy.h>
#include <memkind/internal/tbb_wrapper.h>

#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

void *(*pool_malloc)(void *, size_t);
void *(*pool_realloc)(void *, void *, size_t);
void *(*pool_aligned_malloc)(void *, size_t, size_t);
bool (*pool_free)(void *, void *);
int (*pool_create_v1)(intptr_t, const struct MemPoolPolicy *, void **);
bool (*pool_destroy)(void *);
void *(*pool_identify)(void *object);
size_t (*pool_msize)(void *, void *);

static void *tbb_handle = NULL;
static bool TBBInitDone = false;

void load_tbb_symbols(void)
{
    const char so_name[] = "libtbbmalloc.so.2";
    tbb_handle = dlopen(so_name, RTLD_LAZY);
    if (!tbb_handle) {
        log_fatal("%s not found.", so_name);
        abort();
    }

    pool_malloc = dlsym(tbb_handle, "_ZN3rml11pool_mallocEPNS_10MemoryPoolEm");
    pool_realloc =
        dlsym(tbb_handle, "_ZN3rml12pool_reallocEPNS_10MemoryPoolEPvm");
    pool_aligned_malloc =
        dlsym(tbb_handle, "_ZN3rml19pool_aligned_mallocEPNS_10MemoryPoolEmm");
    pool_free = dlsym(tbb_handle, "_ZN3rml9pool_freeEPNS_10MemoryPoolEPv");
    pool_create_v1 = dlsym(
        tbb_handle,
        "_ZN3rml14pool_create_v1ElPKNS_13MemPoolPolicyEPPNS_10MemoryPoolE");
    pool_destroy = dlsym(tbb_handle, "_ZN3rml12pool_destroyEPNS_10MemoryPoolE");
    pool_identify = dlsym(tbb_handle, "_ZN3rml13pool_identifyEPv");
    pool_msize = dlsym(tbb_handle, "_ZN3rml10pool_msizeEPNS_10MemoryPoolEPv");

    if (!pool_malloc || !pool_realloc || !pool_aligned_malloc || !pool_free ||
        !pool_create_v1 || !pool_destroy || !pool_identify) {
        log_fatal("Could not find symbols in %s.", so_name);
        dlclose(tbb_handle);
        abort();
    }
    TBBInitDone = true;
}

// Granularity of raw_alloc allocations
#define GRANULARITY 2 * 1024 * 1024
static void *raw_alloc(intptr_t pool_id, size_t *bytes /*=n*GRANULARITY*/)
{
    void *ptr = kind_mmap((struct memkind *)pool_id, NULL, *bytes);
    return (ptr == MAP_FAILED) ? NULL : ptr;
}

static int raw_free(intptr_t pool_id, void *raw_ptr, size_t raw_bytes)
{
    return munmap(raw_ptr, raw_bytes);
}

static void *tbb_pool_malloc(struct memkind *kind, size_t size)
{
    if (size_out_of_bounds(size))
        return NULL;
    void *result = pool_malloc(kind->priv, size);
    if (!result)
        errno = ENOMEM;
    return result;
}

static void *tbb_pool_calloc(struct memkind *kind, size_t num, size_t size)
{
    if (size_out_of_bounds(num) || size_out_of_bounds(size))
        return NULL;

    const size_t array_size = num * size;
    if (array_size / num != size) {
        errno = ENOMEM;
        return NULL;
    }
    void *result = pool_malloc(kind->priv, array_size);
    if (result) {
        memset(result, 0, array_size);
    } else {
        errno = ENOMEM;
    }
    return result;
}

static void *tbb_pool_common_realloc(void *pool, void *ptr, size_t size)
{
    if (size_out_of_bounds(size)) {
        pool_free(pool, ptr);
        return NULL;
    }
    void *result = pool_realloc(pool, ptr, size);
    if (!result)
        errno = ENOMEM;
    return result;
}

static void *tbb_pool_realloc(struct memkind *kind, void *ptr, size_t size)
{
    return tbb_pool_common_realloc(kind->priv, ptr, size);
}

void *tbb_pool_realloc_with_kind_detect(void *ptr, size_t size)
{
    if (!ptr) {
        errno = EINVAL;
        return NULL;
    }
    return tbb_pool_common_realloc(pool_identify(ptr), ptr, size);
}

int tbb_get_global_stat(memkind_stat_type stat, size_t *value)
{
    log_err("Get allocator statistic is not supported by TBB");
    return MEMKIND_ERROR_OPERATION_FAILED;
}

static int tbb_get_kind_stat(memkind_t kind, memkind_stat_type stat,
                             size_t *value)
{
    log_err("Get kind statistic is not supported by TBB");
    return MEMKIND_ERROR_OPERATION_FAILED;
}

int tbb_update_cached_stats(void)
{
    log_err("Update cached statistics is not supported by TBB");
    return MEMKIND_ERROR_OPERATION_FAILED;
}

void *tbb_pool_defrag_reallocate_with_kind_detect(void *ptr)
{
    log_err("Defrag reallocate method is not supported by TBB");
    return NULL;
}

static void *tbb_defrag_reallocate(struct memkind *kind, void *ptr)
{
    log_err("Defrag reallocate method is not supported by TBB");
    return NULL;
}

int tbb_set_bg_threads(bool state)
{
    log_err("Setting background threads is not supported by TBB");
    return MEMKIND_ERROR_OPERATION_FAILED;
}

struct memkind *tbb_detect_kind(void *ptr)
{
    if (!ptr) {
        return NULL;
    }
    struct memkind *kind = NULL;
    unsigned i;
    void *pool = pool_identify(ptr);
    for (i = 0; i < MEMKIND_MAX_KIND; ++i) {
        int err = memkind_get_kind_by_partition(i, &kind);
        if (!err && kind->priv == pool) {
            break;
        }
    }

    return kind;
}

static int tbb_pool_posix_memalign(struct memkind *kind, void **memptr,
                                   size_t alignment, size_t size)
{
    // Check if alignment is "at least as large as sizeof(void *)".
    if (!alignment && (0 != (alignment & (alignment - sizeof(void *)))))
        return EINVAL;
    // Check if alignment is "a power of 2".
    if (alignment & (alignment - 1))
        return EINVAL;
    if (size_out_of_bounds(size)) {
        *memptr = NULL;
        return 0;
    }
    void *result = pool_aligned_malloc(kind->priv, size, alignment);
    if (!result) {
        return ENOMEM;
    }
    *memptr = result;
    return 0;
}

void tbb_pool_free_with_kind_detect(void *ptr)
{
    if (ptr) {
        pool_free(pool_identify(ptr), ptr);
    }
}

static void tbb_pool_free(struct memkind *kind, void *ptr)
{
    pool_free(kind->priv, ptr);
}

static size_t tbb_pool_common_malloc_usable_size(void *pool, void *ptr)
{
    if (pool_msize) {
        return pool_msize(pool, ptr);
    } else {
        log_err(
            "memkind_malloc_usable_size() is not supported by this TBB version.");
        return 0;
    }
}

static size_t tbb_pool_malloc_usable_size(struct memkind *kind, void *ptr)
{
    return tbb_pool_common_malloc_usable_size(kind->priv, ptr);
}

size_t tbb_pool_malloc_usable_size_with_kind_detect(void *ptr)
{
    size_t size = 0;
    if (ptr) {
        size = tbb_pool_common_malloc_usable_size(pool_identify(ptr), ptr);
    }
    return size;
}

static int tbb_update_memory_usage_policy(struct memkind *kind,
                                          memkind_mem_usage_policy policy)
{
    log_info("Memkind memory usage policies have no effect in TBB manager.");
    return MEMKIND_SUCCESS;
}

static int tbb_destroy(struct memkind *kind)
{
    bool pool_destroy_ret = pool_destroy(kind->priv);
    dlclose(tbb_handle);

    if (!pool_destroy_ret) {
        log_err("TBB pool destroy failure.");
        return MEMKIND_ERROR_OPERATION_FAILED;
    }
    return MEMKIND_SUCCESS;
}

void tbb_initialize(struct memkind *kind)
{
    if (!kind || !TBBInitDone) {
        log_fatal("Failed to initialize TBB.");
        abort();
    }

    struct MemPoolPolicy policy = {.pAlloc = raw_alloc,
                                   .pFree = raw_free,
                                   .granularity = GRANULARITY,
                                   .version = 1,
                                   .fixedPool = false,
                                   .keepAllMemory = false,
                                   .reserved = 0};

    pool_create_v1((intptr_t)kind, &policy, &kind->priv);
    if (!kind->priv) {
        log_fatal("Unable to create TBB memory pool.");
        abort();
    }

    kind->ops->malloc = tbb_pool_malloc;
    kind->ops->calloc = tbb_pool_calloc;
    kind->ops->posix_memalign = tbb_pool_posix_memalign;
    kind->ops->realloc = tbb_pool_realloc;
    kind->ops->free = tbb_pool_free;
    kind->ops->finalize = tbb_destroy;
    kind->ops->malloc_usable_size = tbb_pool_malloc_usable_size;
    kind->ops->update_memory_usage_policy = tbb_update_memory_usage_policy;
    kind->ops->get_stat = tbb_get_kind_stat;
    kind->ops->defrag_reallocate = tbb_defrag_reallocate;
}
