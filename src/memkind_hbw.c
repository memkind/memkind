// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2021 Intel Corporation. */

#include <memkind/internal/heap_manager.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_bitmask.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_hbw.h>
#include <memkind/internal/memkind_hugetlb.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_mem_attributes.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <numa.h>
#include <numaif.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmpx.h>

MEMKIND_EXPORT struct memkind_ops MEMKIND_HBW_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hbw_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HBW_ALL_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hbw_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_all_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_all_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HBW_HUGETLB_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hbw_hugetlb_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_hugetlb_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HBW_ALL_HUGETLB_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hbw_hugetlb_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_mbind_mode = memkind_default_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_all_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_all_hugetlb_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HBW_PREFERRED_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hbw_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_preferred_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_preferred_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HBW_PREFERRED_HUGETLB_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hbw_hugetlb_check_available,
    .mbind = memkind_default_mbind,
    .get_mmap_flags = memkind_hugetlb_get_mmap_flags,
    .get_mbind_mode = memkind_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_preferred_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_preferred_hugetlb_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

MEMKIND_EXPORT struct memkind_ops MEMKIND_HBW_INTERLEAVE_OPS = {
    .create = memkind_arena_create,
    .destroy = memkind_default_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .check_available = memkind_hbw_check_available,
    .mbind = memkind_default_mbind,
    .madvise = memkind_nohugepage_madvise,
    .get_mmap_flags = memkind_default_get_mmap_flags,
    .get_mbind_mode = memkind_interleave_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_all_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_interleave_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

struct hbw_numanode_t {
    int init_err;
    void *numanode;
};

static struct hbw_numanode_t memkind_hbw_numanode_g[NODE_VARIANT_MAX_EXT];
static pthread_once_t memkind_hbw_numanode_once_g[NODE_VARIANT_MAX_EXT] = {
    PTHREAD_ONCE_INIT};

static void memkind_hbw_closest_numanode_init(void);
static void memkind_hbw_closest_preferred_numanode_init(void);
static void memkind_hbw_all_numanode_init(void);

// This declaration is necessary, cause it's missing in headers from
// libnuma 2.0.8
extern unsigned int numa_bitmask_weight(const struct bitmask *bmp);

MEMKIND_EXPORT int memkind_hbw_check_available(struct memkind *kind)
{
    return kind->ops->get_mbind_nodemask(kind, NULL, 0);
}

MEMKIND_EXPORT int memkind_hbw_hugetlb_check_available(struct memkind *kind)
{
    int err = memkind_hbw_check_available(kind);
    if (!err) {
        err = memkind_hugetlb_check_available_2mb(kind);
    }
    return err;
}

MEMKIND_EXPORT int memkind_hbw_get_mbind_nodemask(struct memkind *kind,
                                                  unsigned long *nodemask,
                                                  unsigned long maxnode)
{
    struct hbw_numanode_t *g = &memkind_hbw_numanode_g[NODE_VARIANT_MULTIPLE];
    pthread_once(&memkind_hbw_numanode_once_g[NODE_VARIANT_MULTIPLE],
                 memkind_hbw_closest_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err)) {
        g->init_err =
            set_bitmask_for_current_numanode(nodemask, maxnode, g->numanode);
    }
    return g->init_err;
}

int memkind_hbw_get_preferred_mbind_nodemask(struct memkind *kind,
                                             unsigned long *nodemask,
                                             unsigned long maxnode)
{
    struct hbw_numanode_t *g = &memkind_hbw_numanode_g[NODE_VARIANT_SINGLE];
    pthread_once(&memkind_hbw_numanode_once_g[NODE_VARIANT_SINGLE],
                 memkind_hbw_closest_preferred_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err)) {
        g->init_err =
            set_bitmask_for_current_numanode(nodemask, maxnode, g->numanode);
    }
    return g->init_err;
}

MEMKIND_EXPORT int memkind_hbw_all_get_mbind_nodemask(struct memkind *kind,
                                                      unsigned long *nodemask,
                                                      unsigned long maxnode)
{
    struct hbw_numanode_t *g = &memkind_hbw_numanode_g[NODE_VARIANT_ALL];
    pthread_once(&memkind_hbw_numanode_once_g[NODE_VARIANT_ALL],
                 memkind_hbw_all_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err)) {
        g->init_err =
            set_bitmask_for_current_numanode(nodemask, maxnode, g->numanode);
    }
    return g->init_err;
}

typedef struct registers_t {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} registers_t;

inline static void cpuid_asm(int leaf, int subleaf, registers_t *registers)
{
#ifdef __x86_64__
    asm volatile("cpuid"
                 : "=a"(registers->eax), "=b"(registers->ebx),
                   "=c"(registers->ecx), "=d"(registers->edx)
                 : "0"(leaf), "2"(subleaf));
#else
    // Non-x86 can't possibly be Knight's Landing nor Knight's Mill.
    registers->eax = 0;
#endif
}

#define CPUID_MODEL_SHIFT (4)
#define CPUID_MODEL_MASK (0xf)
#define CPUID_EXT_MODEL_MASK (0xf)
#define CPUID_EXT_MODEL_SHIFT (16)
#define CPUID_FAMILY_MASK (0xf)
#define CPUID_FAMILY_SHIFT (8)
#define CPU_MODEL_KNL (0x57)
#define CPU_MODEL_KNM (0x85)
#define CPU_FAMILY_INTEL (0x06)

#define NO_NUMA_NODES_FLAT_MODE_OTHER (2)
#define NO_NUMA_NODES_FLAT_MODE_SNC_2 (4)
#define NO_NUMA_NODES_FLAT_MODE_SNC_4 (8)

typedef struct {
    uint32_t model;
    uint32_t family;
} cpu_model_data_t;

static bool is_hbm_legacy_supported(void)
{
    registers_t registers;
    cpuid_asm(1, 0, &registers);
    uint32_t model = (registers.eax >> CPUID_MODEL_SHIFT) & CPUID_MODEL_MASK;
    uint32_t model_ext =
        (registers.eax >> CPUID_EXT_MODEL_SHIFT) & CPUID_EXT_MODEL_MASK;

    cpu_model_data_t data;
    data.model = model | (model_ext << 4);
    data.family = (registers.eax >> CPUID_FAMILY_SHIFT) & CPUID_FAMILY_MASK;
    return data.family == CPU_FAMILY_INTEL &&
        (data.model == CPU_MODEL_KNL || data.model == CPU_MODEL_KNM);
}

static int get_legacy_hbw_nodes_mask(struct bitmask **hbw_node_mask)
{
    struct bitmask *node_cpumask;
    int i;

    // Check if NUMA configuration is supported.
    int nodes_num = numa_num_configured_nodes();
    if (nodes_num != NO_NUMA_NODES_FLAT_MODE_OTHER &&
        nodes_num != NO_NUMA_NODES_FLAT_MODE_SNC_2 &&
        nodes_num != NO_NUMA_NODES_FLAT_MODE_SNC_4) {
        log_err(
            "High Bandwidth Memory is not supported by this NUMA configuration.");
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    node_cpumask = numa_allocate_cpumask();
    if (!node_cpumask) {
        log_err("numa_allocate_cpumask() failed.");
        return MEMKIND_ERROR_UNAVAILABLE;
    }
    *hbw_node_mask = numa_bitmask_alloc(nodes_num);
    if (*hbw_node_mask == NULL) {
        log_err("numa_bitmask_alloc() failed.");
        numa_bitmask_free(node_cpumask);
        return MEMKIND_ERROR_UNAVAILABLE;
    }
    assert(node_cpumask->size >= nodes_num);
    for (i = 0; i < nodes_num; ++i) {
        numa_node_to_cpus(i, node_cpumask);
        if (numa_bitmask_weight(node_cpumask) == 0) {
            // NUMA nodes without CPU are HBW nodes.
            numa_bitmask_setbit(*hbw_node_mask, i);
        }
    }

    if (2 * numa_bitmask_weight(*hbw_node_mask) == nodes_num) {
        numa_bitmask_free(node_cpumask);
        log_info("Detected High Bandwidth Memory.");
        return MEMKIND_SUCCESS;
    }

    numa_bitmask_free(*hbw_node_mask);
    return MEMKIND_ERROR_UNAVAILABLE;
}

static int memkind_hbw_get_nodemask(struct bitmask **bm)
{
    char *nodes_env = memkind_get_env("MEMKIND_HBW_NODES");
    if (nodes_env) {
        return memkind_env_get_nodemask(nodes_env, bm);
    } else {
        return get_legacy_hbw_nodes_mask(bm);
    }
}

static bool is_hmat_supported(void)
{
    if (memkind_get_env("MEMKIND_HBW_NODES") || is_hbm_legacy_supported())
        return false;
    return true;
}

static void memkind_hbw_closest_numanode_init(void)
{
    struct hbw_numanode_t *g = &memkind_hbw_numanode_g[NODE_VARIANT_MULTIPLE];
    g->numanode = NULL;
    if (!is_hmat_supported()) {
        g->init_err = set_closest_numanode(memkind_hbw_get_nodemask,
                                           &g->numanode, NODE_VARIANT_MULTIPLE);
    } else {
        g->init_err =
            set_closest_numanode_mem_attr(&g->numanode, NODE_VARIANT_MULTIPLE);
    }
}

static void memkind_hbw_closest_preferred_numanode_init(void)
{
    struct hbw_numanode_t *g = &memkind_hbw_numanode_g[NODE_VARIANT_SINGLE];
    g->numanode = NULL;
    if (!is_hmat_supported()) {
        g->init_err = set_closest_numanode(memkind_hbw_get_nodemask,
                                           &g->numanode, NODE_VARIANT_SINGLE);
    } else {
        g->init_err =
            set_closest_numanode_mem_attr(&g->numanode, NODE_VARIANT_SINGLE);
    }
}

static void memkind_hbw_all_numanode_init(void)
{
    struct hbw_numanode_t *g = &memkind_hbw_numanode_g[NODE_VARIANT_ALL];
    g->numanode = NULL;
    if (!is_hmat_supported()) {
        g->init_err = set_closest_numanode(memkind_hbw_get_nodemask,
                                           &g->numanode, NODE_VARIANT_ALL);
    } else {
        g->init_err =
            set_closest_numanode_mem_attr(&g->numanode, NODE_VARIANT_ALL);
    }
}

MEMKIND_EXPORT void memkind_hbw_init_once(void)
{
    memkind_init(MEMKIND_HBW, true);
}

MEMKIND_EXPORT void memkind_hbw_all_init_once(void)
{
    memkind_init(MEMKIND_HBW_ALL, true);
}

MEMKIND_EXPORT void memkind_hbw_hugetlb_init_once(void)
{
    memkind_init(MEMKIND_HBW_HUGETLB, true);
}

MEMKIND_EXPORT void memkind_hbw_all_hugetlb_init_once(void)
{
    memkind_init(MEMKIND_HBW_ALL_HUGETLB, true);
}

MEMKIND_EXPORT void memkind_hbw_preferred_init_once(void)
{
    memkind_init(MEMKIND_HBW_PREFERRED, true);
}

MEMKIND_EXPORT void memkind_hbw_preferred_hugetlb_init_once(void)
{
    memkind_init(MEMKIND_HBW_PREFERRED_HUGETLB, true);
}

MEMKIND_EXPORT void memkind_hbw_interleave_init_once(void)
{
    memkind_init(MEMKIND_HBW_INTERLEAVE, true);
}
