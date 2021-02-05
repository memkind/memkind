// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2021 Intel Corporation. */

#define MEMKIND_VERSION_MAJOR 1
#define MEMKIND_VERSION_MINOR 11
#define MEMKIND_VERSION_PATCH 0

#include <memkind.h>
#include <memkind/internal/heap_manager.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_capacity.h>
#include <memkind/internal/memkind_dax_kmem.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_gbtlb.h>
#include <memkind/internal/memkind_hbw.h>
#include <memkind/internal/memkind_hugetlb.h>
#include <memkind/internal/memkind_interleave.h>
#include <memkind/internal/memkind_local.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_pmem.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/memkind_regular.h>
#include <memkind/internal/tbb_wrapper.h>

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <numa.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <unistd.h>

#ifdef MEMKIND_ENABLE_HEAP_MANAGER
#define m_detect_kind(ptr) heap_manager_detect_kind(ptr)
#define m_free(ptr) heap_manager_free(ptr)
#define m_realloc(ptr, size) heap_manager_realloc(ptr, size)
#define m_usable_size(ptr) heap_manager_malloc_usable_size(ptr)
#define m_defrag_reallocate(ptr) heap_manager_defrag_reallocate(ptr)
#define m_get_global_stat(stat, value) heap_manager_get_stat(stat, value)
#define m_update_cached_stats heap_manager_update_cached_stats
#define m_init heap_manager_init
#define m_set_bg_threads(state) heap_manager_set_bg_threads(state)
#else
#define m_detect_kind(ptr) memkind_arena_detect_kind(ptr)
#define m_free(ptr) memkind_arena_free_with_kind_detect(ptr)
#define m_realloc(ptr, size) memkind_arena_realloc_with_kind_detect(ptr, size)
#define m_usable_size(ptr) memkind_default_malloc_usable_size(NULL, ptr)
#define m_defrag_reallocate(ptr)                                               \
    memkind_arena_defrag_reallocate_with_kind_detect(ptr)
#define m_get_global_stat(stat, value)                                         \
    memkind_arena_get_global_stat(stat, value)
#define m_update_cached_stats memkind_arena_update_cached_stats
#define m_init memkind_arena_init
#define m_set_bg_threads(state) memkind_arena_set_bg_threads(state)
#endif

/* Clear bits in x, but only this specified in mask. */
#define CLEAR_BIT(x, mask) ((x) &= (~(mask)))

extern struct memkind_ops MEMKIND_HBW_GBTLB_OPS;
extern struct memkind_ops MEMKIND_HBW_PREFERRED_GBTLB_OPS;
extern struct memkind_ops MEMKIND_GBTLB_OPS;

static struct memkind MEMKIND_DEFAULT_STATIC = {
    .ops = &MEMKIND_DEFAULT_OPS,
    .partition = MEMKIND_PARTITION_DEFAULT,
    .name = "memkind_default",
    .init_once = PTHREAD_ONCE_INIT,
    .arena_zero = 0,
    .arena_map_len = ARENA_LIMIT_DEFAULT_KIND - 1,
};

static struct memkind MEMKIND_HUGETLB_STATIC = {
    .ops = &MEMKIND_HUGETLB_OPS,
    .partition = MEMKIND_PARTITION_HUGETLB,
    .name = "memkind_hugetlb",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_INTERLEAVE_STATIC = {
    .ops = &MEMKIND_INTERLEAVE_OPS,
    .partition = MEMKIND_PARTITION_INTERLEAVE,
    .name = "memkind_interleave",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HBW_STATIC = {
    .ops = &MEMKIND_HBW_OPS,
    .partition = MEMKIND_PARTITION_HBW,
    .name = "memkind_hbw",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HBW_ALL_STATIC = {
    .ops = &MEMKIND_HBW_ALL_OPS,
    .partition = MEMKIND_PARTITION_HBW_ALL,
    .name = "memkind_hbw_all",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HBW_PREFERRED_STATIC = {
    .ops = &MEMKIND_HBW_PREFERRED_OPS,
    .partition = MEMKIND_PARTITION_HBW_PREFERRED,
    .name = "memkind_hbw_preferred",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HBW_HUGETLB_STATIC = {
    .ops = &MEMKIND_HBW_HUGETLB_OPS,
    .partition = MEMKIND_PARTITION_HBW_HUGETLB,
    .name = "memkind_hbw_hugetlb",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HBW_ALL_HUGETLB_STATIC = {
    .ops = &MEMKIND_HBW_ALL_HUGETLB_OPS,
    .partition = MEMKIND_PARTITION_HBW_ALL_HUGETLB,
    .name = "memkind_hbw_all_hugetlb",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HBW_PREFERRED_HUGETLB_STATIC = {
    .ops = &MEMKIND_HBW_PREFERRED_HUGETLB_OPS,
    .partition = MEMKIND_PARTITION_HBW_PREFERRED_HUGETLB,
    .name = "memkind_hbw_preferred_hugetlb",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HBW_GBTLB_STATIC = {
    .ops = &MEMKIND_HBW_GBTLB_OPS,
    .partition = MEMKIND_PARTITION_HBW_GBTLB,
    .name = "memkind_hbw_gbtlb",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HBW_PREFERRED_GBTLB_STATIC = {
    .ops = &MEMKIND_HBW_PREFERRED_GBTLB_OPS,
    .partition = MEMKIND_PARTITION_HBW_PREFERRED_GBTLB,
    .name = "memkind_hbw_preferred_gbtlb",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_GBTLB_STATIC = {
    .ops = &MEMKIND_GBTLB_OPS,
    .partition = MEMKIND_PARTITION_GBTLB,
    .name = "memkind_gbtlb",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HBW_INTERLEAVE_STATIC = {
    .ops = &MEMKIND_HBW_INTERLEAVE_OPS,
    .partition = MEMKIND_PARTITION_HBW_INTERLEAVE,
    .name = "memkind_hbw_interleave",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_REGULAR_STATIC = {
    .ops = &MEMKIND_REGULAR_OPS,
    .partition = MEMKIND_PARTITION_REGULAR,
    .name = "memkind_regular",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_DAX_KMEM_STATIC = {
    .ops = &MEMKIND_DAX_KMEM_OPS,
    .partition = MEMKIND_PARTITION_DAX_KMEM,
    .name = "memkind_dax_kmem",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_DAX_KMEM_ALL_STATIC = {
    .ops = &MEMKIND_DAX_KMEM_ALL_OPS,
    .partition = MEMKIND_PARTITION_DAX_KMEM_ALL,
    .name = "memkind_dax_kmem_all",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_DAX_KMEM_PREFERRED_STATIC = {
    .ops = &MEMKIND_DAX_KMEM_PREFERRED_OPS,
    .partition = MEMKIND_PARTITION_DAX_KMEM_PREFERRED,
    .name = "memkind_dax_kmem_preferred",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_DAX_KMEM_INTERLEAVE_STATIC = {
    .ops = &MEMKIND_DAX_KMEM_INTERLEAVE_OPS,
    .partition = MEMKIND_PARTITION_DAX_KMEM_INTERLEAVE,
    .name = "memkind_dax_kmem_interleave",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HIGHEST_CAPACITY_STATIC = {
    .ops = &MEMKIND_HIGHEST_CAPACITY_OPS,
    .partition = MEMKIND_PARTITION_HIGHEST_CAPACITY,
    .name = "memkind_highest_capacity",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HIGHEST_CAPACITY_PREFERRED_STATIC = {
    .ops = &MEMKIND_HIGHEST_CAPACITY_PREFERRED_OPS,
    .partition = MEMKIND_PARTITION_HIGHEST_CAPACITY_PREFERRED,
    .name = "memkind_highest_capacity_preferred",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HIGHEST_CAPACITY_LOCAL_STATIC = {
    .ops = &MEMKIND_HIGHEST_CAPACITY_LOCAL_OPS,
    .partition = MEMKIND_PARTITION_HIGHEST_CAPACITY_LOCAL,
    .name = "memkind_highest_capacity_local",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED_STATIC = {
    .ops = &MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED_OPS,
    .partition = MEMKIND_PARTITION_HIGHEST_CAPACITY_LOCAL_PREFERRED,
    .name = "memkind_highest_capacity_local_preferred",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_LOWEST_LATENCY_LOCAL_STATIC = {
    .ops = &MEMKIND_LOWEST_LATENCY_LOCAL_OPS,
    .partition = MEMKIND_PARTITION_LOWEST_LATENCY_LOCAL,
    .name = "memkind_lowest_latency_local",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED_STATIC = {
    .ops = &MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED_OPS,
    .partition = MEMKIND_PARTITION_LOWEST_LATENCY_LOCAL_PREFERRED,
    .name = "memkind_lowest_latency_local_preferred",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HIGHEST_BANDWIDTH_LOCAL_STATIC = {
    .ops = &MEMKIND_HIGHEST_BANDWIDTH_LOCAL_OPS,
    .partition = MEMKIND_PARTITION_HIGHEST_BANDWIDTH_LOCAL,
    .name = "memkind_highest_bandwidth_local",
    .init_once = PTHREAD_ONCE_INIT,
};

static struct memkind MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED_STATIC = {
    .ops = &MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED_OPS,
    .partition = MEMKIND_PARTITION_HIGHEST_BANDWIDTH_LOCAL_PREFERRED,
    .name = "memkind_highest_bandwidth_local_preferred",
    .init_once = PTHREAD_ONCE_INIT,
};

MEMKIND_EXPORT struct memkind *MEMKIND_DEFAULT = &MEMKIND_DEFAULT_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HUGETLB = &MEMKIND_HUGETLB_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_INTERLEAVE = &MEMKIND_INTERLEAVE_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HBW = &MEMKIND_HBW_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HBW_ALL = &MEMKIND_HBW_ALL_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HBW_PREFERRED =
    &MEMKIND_HBW_PREFERRED_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HBW_HUGETLB =
    &MEMKIND_HBW_HUGETLB_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HBW_ALL_HUGETLB =
    &MEMKIND_HBW_ALL_HUGETLB_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HBW_PREFERRED_HUGETLB =
    &MEMKIND_HBW_PREFERRED_HUGETLB_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HBW_GBTLB = &MEMKIND_HBW_GBTLB_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HBW_PREFERRED_GBTLB =
    &MEMKIND_HBW_PREFERRED_GBTLB_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_GBTLB = &MEMKIND_GBTLB_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HBW_INTERLEAVE =
    &MEMKIND_HBW_INTERLEAVE_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_REGULAR = &MEMKIND_REGULAR_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_DAX_KMEM = &MEMKIND_DAX_KMEM_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_DAX_KMEM_ALL =
    &MEMKIND_DAX_KMEM_ALL_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_DAX_KMEM_PREFERRED =
    &MEMKIND_DAX_KMEM_PREFERRED_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_DAX_KMEM_INTERLEAVE =
    &MEMKIND_DAX_KMEM_INTERLEAVE_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HIGHEST_CAPACITY =
    &MEMKIND_HIGHEST_CAPACITY_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HIGHEST_CAPACITY_PREFERRED =
    &MEMKIND_HIGHEST_CAPACITY_PREFERRED_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HIGHEST_CAPACITY_LOCAL =
    &MEMKIND_HIGHEST_CAPACITY_LOCAL_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED =
    &MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_LOWEST_LATENCY_LOCAL =
    &MEMKIND_LOWEST_LATENCY_LOCAL_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED =
    &MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HIGHEST_BANDWIDTH_LOCAL =
    &MEMKIND_HIGHEST_BANDWIDTH_LOCAL_STATIC;
MEMKIND_EXPORT struct memkind *MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED =
    &MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED_STATIC;

struct memkind_registry {
    struct memkind *partition_map[MEMKIND_MAX_KIND];
    int num_kind;
    pthread_mutex_t lock;
};

static struct memkind_registry memkind_registry_g = {
    {
        [MEMKIND_PARTITION_DEFAULT] = &MEMKIND_DEFAULT_STATIC,
        [MEMKIND_PARTITION_HBW] = &MEMKIND_HBW_STATIC,
        [MEMKIND_PARTITION_HBW_PREFERRED] = &MEMKIND_HBW_PREFERRED_STATIC,
        [MEMKIND_PARTITION_HBW_HUGETLB] = &MEMKIND_HBW_HUGETLB_STATIC,
        [MEMKIND_PARTITION_HBW_PREFERRED_HUGETLB] =
            &MEMKIND_HBW_PREFERRED_HUGETLB_STATIC,
        [MEMKIND_PARTITION_HUGETLB] = &MEMKIND_HUGETLB_STATIC,
        [MEMKIND_PARTITION_HBW_GBTLB] = &MEMKIND_HBW_GBTLB_STATIC,
        [MEMKIND_PARTITION_HBW_PREFERRED_GBTLB] =
            &MEMKIND_HBW_PREFERRED_GBTLB_STATIC,
        [MEMKIND_PARTITION_GBTLB] = &MEMKIND_GBTLB_STATIC,
        [MEMKIND_PARTITION_HBW_INTERLEAVE] = &MEMKIND_HBW_INTERLEAVE_STATIC,
        [MEMKIND_PARTITION_INTERLEAVE] = &MEMKIND_INTERLEAVE_STATIC,
        [MEMKIND_PARTITION_REGULAR] = &MEMKIND_REGULAR_STATIC,
        [MEMKIND_PARTITION_HBW_ALL] = &MEMKIND_HBW_ALL_STATIC,
        [MEMKIND_PARTITION_HBW_ALL_HUGETLB] = &MEMKIND_HBW_ALL_HUGETLB_STATIC,
        [MEMKIND_PARTITION_DAX_KMEM] = &MEMKIND_DAX_KMEM_STATIC,
        [MEMKIND_PARTITION_DAX_KMEM_ALL] = &MEMKIND_DAX_KMEM_ALL_STATIC,
        [MEMKIND_PARTITION_DAX_KMEM_PREFERRED] =
            &MEMKIND_DAX_KMEM_PREFERRED_STATIC,
        [MEMKIND_PARTITION_DAX_KMEM_INTERLEAVE] =
            &MEMKIND_DAX_KMEM_INTERLEAVE_STATIC,
        [MEMKIND_PARTITION_HIGHEST_CAPACITY] = &MEMKIND_HIGHEST_CAPACITY_STATIC,
        [MEMKIND_PARTITION_HIGHEST_CAPACITY_PREFERRED] =
            &MEMKIND_HIGHEST_CAPACITY_PREFERRED_STATIC,
        [MEMKIND_PARTITION_HIGHEST_CAPACITY_LOCAL] =
            &MEMKIND_HIGHEST_CAPACITY_LOCAL_STATIC,
        [MEMKIND_PARTITION_HIGHEST_CAPACITY_LOCAL_PREFERRED] =
            &MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED_STATIC,
        [MEMKIND_PARTITION_LOWEST_LATENCY_LOCAL] =
            &MEMKIND_LOWEST_LATENCY_LOCAL_STATIC,
        [MEMKIND_PARTITION_LOWEST_LATENCY_LOCAL_PREFERRED] =
            &MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED_STATIC,
        [MEMKIND_PARTITION_HIGHEST_BANDWIDTH_LOCAL] =
            &MEMKIND_HIGHEST_BANDWIDTH_LOCAL_STATIC,
        [MEMKIND_PARTITION_HIGHEST_BANDWIDTH_LOCAL_PREFERRED] =
            &MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED_STATIC,
    },
    MEMKIND_NUM_BASE_KIND,
    PTHREAD_MUTEX_INITIALIZER};

void *kind_mmap(struct memkind *kind, void *addr, size_t size)
{
    if (MEMKIND_LIKELY(kind->ops->mmap == NULL)) {
        return memkind_default_mmap(kind, addr, size);
    } else {
        return kind->ops->mmap(kind, addr, size);
    }
}

static int validate_memtype_bits(memkind_memtype_t memtype)
{
    if (memtype == 0)
        return -1;

    CLEAR_BIT(memtype, MEMKIND_MEMTYPE_DEFAULT);
    CLEAR_BIT(memtype, MEMKIND_MEMTYPE_HIGH_BANDWIDTH);

    if (memtype != 0)
        return -1;
    return 0;
}

static int validate_flags_bits(memkind_bits_t flags)
{
    CLEAR_BIT(flags, MEMKIND_MASK_PAGE_SIZE_2MB);

    if (flags != 0)
        return -1;
    return 0;
}

static int validate_policy(memkind_policy_t policy)
{
    if (policy >= MEMKIND_POLICY_MAX_VALUE)
        return -1;
    return 0;
}

struct create_args {
    memkind_t kind;
    memkind_policy_t policy;
    memkind_bits_t flags;
    memkind_memtype_t memtype_flags;
};

static struct create_args supported_args[] = {

    {&MEMKIND_HBW_STATIC, MEMKIND_POLICY_BIND_LOCAL, 0,
     MEMKIND_MEMTYPE_HIGH_BANDWIDTH},
    {&MEMKIND_HBW_HUGETLB_STATIC, MEMKIND_POLICY_BIND_LOCAL,
     MEMKIND_MASK_PAGE_SIZE_2MB, MEMKIND_MEMTYPE_HIGH_BANDWIDTH},
    {&MEMKIND_HBW_ALL_STATIC, MEMKIND_POLICY_BIND_ALL, 0,
     MEMKIND_MEMTYPE_HIGH_BANDWIDTH},
    {&MEMKIND_HBW_ALL_HUGETLB_STATIC, MEMKIND_POLICY_BIND_ALL,
     MEMKIND_MASK_PAGE_SIZE_2MB, MEMKIND_MEMTYPE_HIGH_BANDWIDTH},
    {&MEMKIND_HBW_PREFERRED_STATIC, MEMKIND_POLICY_PREFERRED_LOCAL, 0,
     MEMKIND_MEMTYPE_HIGH_BANDWIDTH},
    {&MEMKIND_HBW_PREFERRED_HUGETLB_STATIC, MEMKIND_POLICY_PREFERRED_LOCAL,
     MEMKIND_MASK_PAGE_SIZE_2MB, MEMKIND_MEMTYPE_HIGH_BANDWIDTH},
    {&MEMKIND_HBW_INTERLEAVE_STATIC, MEMKIND_POLICY_INTERLEAVE_ALL, 0,
     MEMKIND_MEMTYPE_HIGH_BANDWIDTH},
    {&MEMKIND_DEFAULT_STATIC, MEMKIND_POLICY_PREFERRED_LOCAL, 0,
     MEMKIND_MEMTYPE_DEFAULT},
    {&MEMKIND_HUGETLB_STATIC, MEMKIND_POLICY_PREFERRED_LOCAL,
     MEMKIND_MASK_PAGE_SIZE_2MB, MEMKIND_MEMTYPE_DEFAULT},
    {&MEMKIND_INTERLEAVE_STATIC, MEMKIND_POLICY_INTERLEAVE_ALL, 0,
     MEMKIND_MEMTYPE_HIGH_BANDWIDTH | MEMKIND_MEMTYPE_DEFAULT},
};

/* Kind creation */
MEMKIND_EXPORT int memkind_create_kind(memkind_memtype_t memtype_flags,
                                       memkind_policy_t policy,
                                       memkind_bits_t flags, memkind_t *kind)
{
    if (validate_memtype_bits(memtype_flags) != 0) {
        log_err("Cannot create kind: incorrect memtype_flags.");
        return MEMKIND_ERROR_INVALID;
    }

    if (validate_flags_bits(flags) != 0) {
        log_err("Cannot create kind: incorrect flags.");
        return MEMKIND_ERROR_INVALID;
    }

    if (validate_policy(policy) != 0) {
        log_err("Cannot create kind: incorrect policy.");
        return MEMKIND_ERROR_INVALID;
    }

    if (kind == NULL) {
        log_err("Cannot create kind: 'kind' is NULL pointer.");
        return MEMKIND_ERROR_INVALID;
    }

    int i,
        num_supported_args =
            sizeof(supported_args) / sizeof(struct create_args);
    for (i = 0; i < num_supported_args; i++) {
        if ((supported_args[i].memtype_flags == memtype_flags) &&
            (supported_args[i].policy == policy) &&
            (supported_args[i].flags == flags)) {

            if (memkind_check_available(supported_args[i].kind) == 0) {
                *kind = supported_args[i].kind;
                return MEMKIND_SUCCESS;
            } else if (policy == MEMKIND_POLICY_PREFERRED_LOCAL) {
                *kind = MEMKIND_DEFAULT;
                return MEMKIND_SUCCESS;
            }
            log_err(
                "Cannot create kind: requested memory type is not available.");
            return MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE;
        }
    }

    log_err("Cannot create kind: unsupported set of capabilities.");
    return MEMKIND_ERROR_INVALID;
}

static void memkind_destroy_dynamic_kind_from_register(unsigned int i,
                                                       memkind_t kind)
{
    if (i >= MEMKIND_NUM_BASE_KIND) {
        memkind_registry_g.partition_map[i] = NULL;
        --memkind_registry_g.num_kind;
        jemk_free(kind);
    }
}

/* Kind destruction. */
MEMKIND_EXPORT int memkind_destroy_kind(memkind_t kind)
{
    if (pthread_mutex_lock(&memkind_registry_g.lock) != 0)
        assert(0 && "failed to acquire mutex");
    unsigned i;
    int err = kind->ops->destroy(kind);
    for (i = MEMKIND_NUM_BASE_KIND; i < MEMKIND_MAX_KIND; ++i) {
        if (memkind_registry_g.partition_map[i] &&
            strcmp(kind->name, memkind_registry_g.partition_map[i]->name) ==
                0) {
            memkind_destroy_dynamic_kind_from_register(i, kind);
            break;
        }
    }
    if (pthread_mutex_unlock(&memkind_registry_g.lock) != 0)
        assert(0 && "failed to release mutex");
    return err;
}

MEMKIND_EXPORT memkind_t memkind_detect_kind(void *ptr)
{
    return m_detect_kind(ptr);
}

/* Declare weak symbols for allocator decorators */
extern void memkind_malloc_pre(struct memkind **, size_t *)
    __attribute__((weak));
extern void memkind_malloc_post(struct memkind *, size_t, void **)
    __attribute__((weak));
extern void memkind_calloc_pre(struct memkind **, size_t *, size_t *)
    __attribute__((weak));
extern void memkind_calloc_post(struct memkind *, size_t, size_t, void **)
    __attribute__((weak));
extern void memkind_posix_memalign_pre(struct memkind **, void **, size_t *,
                                       size_t *) __attribute__((weak));
extern void memkind_posix_memalign_post(struct memkind *, void **, size_t,
                                        size_t, int *) __attribute__((weak));
extern void memkind_realloc_pre(struct memkind **, void **, size_t *)
    __attribute__((weak));
extern void memkind_realloc_post(struct memkind *, void *, size_t, void **)
    __attribute__((weak));
extern void memkind_free_pre(struct memkind **, void **) __attribute__((weak));
extern void memkind_free_post(struct memkind *, void *) __attribute__((weak));

MEMKIND_EXPORT int memkind_get_version()
{
    return MEMKIND_VERSION_MAJOR * 1000000 + MEMKIND_VERSION_MINOR * 1000 +
        MEMKIND_VERSION_PATCH;
}

MEMKIND_EXPORT void memkind_error_message(int err, char *msg, size_t size)
{
    switch (err) {
        case MEMKIND_ERROR_UNAVAILABLE:
            strncpy(msg, "<memkind> Requested memory kind is not available",
                    size);
            break;
        case MEMKIND_ERROR_MBIND:
            strncpy(msg, "<memkind> Call to mbind() failed", size);
            break;
        case MEMKIND_ERROR_MMAP:
            strncpy(msg, "<memkind> Call to mmap() failed", size);
            break;
        case MEMKIND_ERROR_MALLOC:
            strncpy(msg, "<memkind> Call to malloc() failed", size);
            break;
        case MEMKIND_ERROR_ENVIRON:
            strncpy(msg,
                    "<memkind> Error parsing environment variable (MEMKIND_*)",
                    size);
            break;
        case MEMKIND_ERROR_INVALID:
            strncpy(msg, "<memkind> Invalid input arguments to memkind routine",
                    size);
            break;
        case MEMKIND_ERROR_TOOMANY:
            snprintf(
                msg, size,
                "<memkind> Attempted to initialize more than maximum (%i) number of kinds",
                MEMKIND_MAX_KIND);
            break;
        case MEMKIND_ERROR_RUNTIME:
            strncpy(msg, "<memkind> Unspecified run-time error", size);
            break;
        case EINVAL:
            strncpy(
                msg,
                "<memkind> Alignment must be a power of two and larger than sizeof(void *)",
                size);
            break;
        case ENOMEM:
            strncpy(msg, "<memkind> Call to jemk_mallocx() failed", size);
            break;
        case MEMKIND_ERROR_HUGETLB:
            strncpy(msg, "<memkind> unable to allocate huge pages", size);
            break;
        case MEMKIND_ERROR_BADOPS:
            strncpy(
                msg,
                "<memkind> memkind_ops structure is poorly formed (missing or incorrect functions)",
                size);
            break;
        case MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE:
            strncpy(msg, "<memkind> Requested memory type is not available",
                    size);
            break;
        case MEMKIND_ERROR_OPERATION_FAILED:
            strncpy(msg, "<memkind> Operation failed", size);
            break;
        case MEMKIND_ERROR_ARENAS_CREATE:
            strncpy(msg, "<memkind> Call to jemalloc's arenas.create () failed",
                    size);
            break;
        default:
            snprintf(msg, size, "<memkind> Undefined error number: %i", err);
            break;
    }
    if (size > 0) {
        msg[size - 1] = '\0';
    }
}

void memkind_init(memkind_t kind, bool check_numa)
{
    log_info("Initializing kind %s.", kind->name);
    m_init(kind);
    if (check_numa) {
        int err = numa_available();
        if (err) {
            log_fatal("[%s] NUMA not available (error code:%d).", kind->name,
                      err);
            abort();
        }
    }
}

char *memkind_get_env(const char *name)
{
#ifdef MEMKIND_HAVE_SECURE_GETENV
    return secure_getenv(name);
#else
    return getenv(name);
#endif
}

static void nop(void)
{}

static int memkind_create(struct memkind_ops *ops, const char *name,
                          struct memkind **kind)
{
    int err;
    unsigned i;
    unsigned id_kind = 0;

    *kind = NULL;
    if (pthread_mutex_lock(&memkind_registry_g.lock) != 0)
        assert(0 && "failed to acquire mutex");

    if (memkind_registry_g.num_kind == MEMKIND_MAX_KIND) {
        log_err(
            "Attempted to initialize more than maximum (%i) number of kinds.",
            MEMKIND_MAX_KIND);
        err = MEMKIND_ERROR_TOOMANY;
        goto exit;
    }
    if (ops->create == NULL || ops->destroy == NULL || ops->malloc == NULL ||
        ops->calloc == NULL || ops->realloc == NULL ||
        ops->posix_memalign == NULL || ops->free == NULL ||
        ops->init_once != NULL) {
        err = MEMKIND_ERROR_BADOPS;
        goto exit;
    }
    for (i = 0; i < MEMKIND_MAX_KIND; ++i) {
        if (memkind_registry_g.partition_map[i] == NULL) {
            id_kind = i;
            break;
        } else if (strcmp(name, memkind_registry_g.partition_map[i]->name) ==
                   0) {
            log_err("Kind with the name %s already exists", name);
            err = MEMKIND_ERROR_INVALID;
            goto exit;
        }
    }
    *kind = (struct memkind *)jemk_calloc(1, sizeof(struct memkind));
    if (!*kind) {
        err = MEMKIND_ERROR_MALLOC;
        log_err("calloc() failed.");
        goto exit;
    }

    (*kind)->partition = memkind_registry_g.num_kind;
    err = ops->create(*kind, ops, name);
    if (err) {
        jemk_free(*kind);
        goto exit;
    }
    memkind_registry_g.partition_map[id_kind] = *kind;
    ++memkind_registry_g.num_kind;

    (*kind)->init_once = PTHREAD_ONCE_INIT;
    pthread_once(&(*kind)->init_once,
                 nop); // this is done to avoid init_once for dynamic kinds
exit:
    if (pthread_mutex_unlock(&memkind_registry_g.lock) != 0)
        assert(0 && "failed to release mutex");

    return err;
}

static int memkind_use_other_heap_manager(void)
{
#ifdef MEMKIND_ENABLE_HEAP_MANAGER
    const char *env = memkind_get_env("MEMKIND_HEAP_MANAGER");
    if (env && strcmp(env, "TBB") == 0) {
        load_tbb_symbols();
        return 1;
    }
#endif
    return 0;
}

#ifdef __GNUC__
__attribute__((constructor))
#endif
static void
memkind_construct(void)
{
    if (!memkind_use_other_heap_manager()) {
        const char *env = memkind_get_env("MEMKIND_BACKGROUND_THREAD_LIMIT");
        if (env) {
            char *end;
            errno = 0;
            size_t thread_limit = strtoul(env, &end, 10);
            if (thread_limit > UINT_MAX || *end != '\0' || errno != 0) {
                log_fatal(
                    "Error: Wrong value of MEMKIND_BACKGROUND_THREAD_LIMIT=%s",
                    env);
                abort();
            }
            memkind_arena_set_max_bg_threads(thread_limit);
        }
    }
}

#ifdef __GNUC__
__attribute__((destructor))
#endif
static int
memkind_finalize(void)
{
    struct memkind *kind;
    unsigned i;
    int err = MEMKIND_SUCCESS;

    if (pthread_mutex_lock(&memkind_registry_g.lock) != 0)
        assert(0 && "failed to acquire mutex");

    for (i = 0; i < MEMKIND_MAX_KIND; ++i) {
        kind = memkind_registry_g.partition_map[i];
        if (kind && kind->ops->finalize) {
            err = kind->ops->finalize(kind);
            if (err) {
                goto exit;
            }
            memkind_destroy_dynamic_kind_from_register(i, kind);
        }
    }
    assert(memkind_registry_g.num_kind == MEMKIND_NUM_BASE_KIND);

exit:
    if (pthread_mutex_unlock(&memkind_registry_g.lock) != 0)
        assert(0 && "failed to release mutex");

    return err;
}

MEMKIND_EXPORT int memkind_check_available(struct memkind *kind)
{
    int err = MEMKIND_SUCCESS;

    if (MEMKIND_LIKELY(kind->ops->check_available)) {
        err = kind->ops->check_available(kind);
    }
    return err;
}

MEMKIND_EXPORT size_t memkind_malloc_usable_size(struct memkind *kind,
                                                 void *ptr)
{
    if (!kind) {
        return m_usable_size(ptr);
    } else {
        return kind->ops->malloc_usable_size(kind, ptr);
    }
}

MEMKIND_EXPORT void *memkind_malloc(struct memkind *kind, size_t size)
{
    void *result;

    pthread_once(&kind->init_once, kind->ops->init_once);

#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_malloc_pre) {
        memkind_malloc_pre(&kind, &size);
    }
#endif

    result = kind->ops->malloc(kind, size);

#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_malloc_post) {
        memkind_malloc_post(kind, size, &result);
    }
#endif

    return result;
}

MEMKIND_EXPORT void *memkind_calloc(struct memkind *kind, size_t num,
                                    size_t size)
{
    void *result;

    pthread_once(&kind->init_once, kind->ops->init_once);

#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_calloc_pre) {
        memkind_calloc_pre(&kind, &num, &size);
    }
#endif

    result = kind->ops->calloc(kind, num, size);

#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_calloc_post) {
        memkind_calloc_post(kind, num, size, &result);
    }
#endif

    return result;
}

MEMKIND_EXPORT int memkind_posix_memalign(struct memkind *kind, void **memptr,
                                          size_t alignment, size_t size)
{
    int err;

    pthread_once(&kind->init_once, kind->ops->init_once);

#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_posix_memalign_pre) {
        memkind_posix_memalign_pre(&kind, memptr, &alignment, &size);
    }
#endif

    err = kind->ops->posix_memalign(kind, memptr, alignment, size);

#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_posix_memalign_post) {
        memkind_posix_memalign_post(kind, memptr, alignment, size, &err);
    }
#endif

    return err;
}

MEMKIND_EXPORT void *memkind_realloc(struct memkind *kind, void *ptr,
                                     size_t size)
{
    void *result;

#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_realloc_pre) {
        memkind_realloc_pre(&kind, &ptr, &size);
    }
#endif

    if (!kind) {
        result = m_realloc(ptr, size);
    } else {
        pthread_once(&kind->init_once, kind->ops->init_once);
        result = kind->ops->realloc(kind, ptr, size);
    }

#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_realloc_post) {
        memkind_realloc_post(kind, ptr, size, &result);
    }
#endif

    return result;
}

MEMKIND_EXPORT void memkind_free(struct memkind *kind, void *ptr)
{
#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_free_pre) {
        memkind_free_pre(&kind, &ptr);
    }
#endif
    if (!kind) {
        m_free(ptr);
    } else {
        pthread_once(&kind->init_once, kind->ops->init_once);
        kind->ops->free(kind, ptr);
    }

#ifdef MEMKIND_DECORATION_ENABLED
    if (memkind_free_post) {
        memkind_free_post(kind, ptr);
    }
#endif
}

MEMKIND_EXPORT struct memkind_config *memkind_config_new(void)
{
    struct memkind_config *cfg =
        (struct memkind_config *)malloc(sizeof(struct memkind_config));
    return cfg;
}

MEMKIND_EXPORT void memkind_config_delete(struct memkind_config *cfg)
{
    free(cfg);
}

MEMKIND_EXPORT void memkind_config_set_path(struct memkind_config *cfg,
                                            const char *pmem_dir)
{
    cfg->pmem_dir = pmem_dir;
}

MEMKIND_EXPORT void memkind_config_set_size(struct memkind_config *cfg,
                                            size_t pmem_size)
{
    cfg->pmem_size = pmem_size;
}

MEMKIND_EXPORT void
memkind_config_set_memory_usage_policy(struct memkind_config *cfg,
                                       memkind_mem_usage_policy policy)
{
    cfg->policy = policy;
}

MEMKIND_EXPORT int memkind_create_pmem(const char *dir, size_t max_size,
                                       struct memkind **kind)
{
    int oerrno;

    if (max_size && max_size < MEMKIND_PMEM_MIN_SIZE) {
        log_err("Cannot create pmem: invalid size.");
        return MEMKIND_ERROR_INVALID;
    }

    if (max_size) {
        /* round up to a multiple of jemalloc chunk size */
        max_size = roundup(max_size, MEMKIND_PMEM_CHUNK_SIZE);
    }

    int fd = -1;
    char name[16];

    int err = memkind_pmem_create_tmpfile(dir, &fd);
    if (err) {
        goto exit;
    }

    snprintf(name, sizeof(name), "pmem%08x", fd);

    err = memkind_create(&MEMKIND_PMEM_OPS, name, kind);
    if (err) {
        goto exit;
    }

    struct memkind_pmem *priv = (*kind)->priv;

    priv->fd = fd;
    priv->offset = 0;
    priv->current_size = 0;
    priv->max_size = max_size;
    priv->dir = jemk_malloc(strlen(dir) + 1);
    if (!priv->dir) {
        goto exit;
    }
    memcpy(priv->dir, dir, strlen(dir));

    return err;

exit:
    oerrno = errno;
    if (fd != -1) {
        (void)close(fd);
    }
    errno = oerrno;
    return err;
}

MEMKIND_EXPORT int memkind_create_pmem_with_config(struct memkind_config *cfg,
                                                   struct memkind **kind)
{
    int status = memkind_create_pmem(cfg->pmem_dir, cfg->pmem_size, kind);
    if (MEMKIND_LIKELY(!status)) {
        status = (*kind)->ops->update_memory_usage_policy(*kind, cfg->policy);
    }

    return status;
}

static int memkind_get_kind_by_partition_internal(int partition,
                                                  struct memkind **kind)
{
    int err = MEMKIND_SUCCESS;

    if (MEMKIND_LIKELY(partition >= 0 && partition < MEMKIND_MAX_KIND &&
                       memkind_registry_g.partition_map[partition] != NULL)) {
        *kind = memkind_registry_g.partition_map[partition];
    } else {
        *kind = NULL;
        err = MEMKIND_ERROR_UNAVAILABLE;
    }
    return err;
}

MEMKIND_EXPORT int memkind_get_kind_by_partition(int partition,
                                                 struct memkind **kind)
{
    return memkind_get_kind_by_partition_internal(partition, kind);
}

MEMKIND_EXPORT int memkind_update_cached_stats(void)
{
    return m_update_cached_stats();
}

MEMKIND_EXPORT void *memkind_defrag_reallocate(memkind_t kind, void *ptr)
{
    if (!kind) {
        return m_defrag_reallocate(ptr);
    } else {
        return kind->ops->defrag_reallocate(kind, ptr);
    }
}

MEMKIND_EXPORT int memkind_get_stat(memkind_t kind, memkind_stat_type stat,
                                    size_t *value)
{
    if (MEMKIND_UNLIKELY(stat >= MEMKIND_STAT_TYPE_MAX_VALUE)) {
        log_err("Unrecognized type of memory statistic %d", stat);
        return MEMKIND_ERROR_INVALID;
    }

    if (!kind) {
        return m_get_global_stat(stat, value);
    } else {
        return kind->ops->get_stat(kind, stat, value);
    }
}

MEMKIND_EXPORT int memkind_check_dax_path(const char *pmem_dir)
{
    return memkind_pmem_validate_dir(pmem_dir);
}

MEMKIND_EXPORT int memkind_set_bg_threads(bool state)
{
    return m_set_bg_threads(state);
}
