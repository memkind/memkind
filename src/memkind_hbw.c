// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include <memkind/internal/memkind_hbw.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_hugetlb.h>
#include <memkind/internal/memkind_bandwidth.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/heap_manager.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utmpx.h>
#include <sched.h>
#include <stdint.h>
#include <assert.h>
#include "config.h"
#ifdef MEMKIND_HWLOC
#include <hwloc.h>

#define MEMKIND_HBW_THRESHOLD_DEFAULT (200 * 1024) // Default threshold is 200 GB/s
#endif

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
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

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
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

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
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

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
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

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
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_preferred_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

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
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_arena = memkind_thread_get_arena,
    .init_once = memkind_hbw_preferred_hugetlb_init_once,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_arena_finalize,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

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
    .defrag_reallocate = memkind_arena_defrag_reallocate
};

struct hbw_closest_numanode_t {
    int init_err;
    int num_cpu;
    struct vec_cpu_node *closest_numanode;
};

static struct hbw_closest_numanode_t memkind_hbw_closest_numanode_g;
static pthread_once_t memkind_hbw_closest_numanode_once_g = PTHREAD_ONCE_INIT;

static void memkind_hbw_closest_numanode_init(void);

// This declaration is necessary, cause it's missing in headers from libnuma 2.0.8
extern unsigned int numa_bitmask_weight(const struct bitmask *bmp );

static int fill_bandwidth_values_heuristically (int *bandwidth);

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
    struct hbw_closest_numanode_t *g = &memkind_hbw_closest_numanode_g;
    pthread_once(&memkind_hbw_closest_numanode_once_g,
                 memkind_hbw_closest_numanode_init);
    if (MEMKIND_LIKELY(!g->init_err)) {
        g->init_err = set_bitmask_for_current_closest_numanode(nodemask, maxnode,
                                                               g->closest_numanode, g->num_cpu);
    }
    return g->init_err;
}

MEMKIND_EXPORT int memkind_hbw_all_get_mbind_nodemask(struct memkind *kind,
                                                      unsigned long *nodemask,
                                                      unsigned long maxnode)
{
    struct hbw_closest_numanode_t *g = &memkind_hbw_closest_numanode_g;
    pthread_once(&memkind_hbw_closest_numanode_once_g,
                 memkind_hbw_closest_numanode_init);

    if (MEMKIND_LIKELY(!g->init_err)) {
        set_bitmask_for_all_closest_numanodes(nodemask, maxnode, g->closest_numanode,
                                              g->num_cpu);
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
    asm volatile("cpuid":"=a"(registers->eax),
                 "=b"(registers->ebx),
                 "=c"(registers->ecx),
                 "=d"(registers->edx):"0"(leaf), "2"(subleaf));
#else
    // Non-x86 can't possibly be Knight's Landing nor Knight's Mill.
    registers->eax = 0;
#endif
}

#define CPUID_MODEL_SHIFT       (4)
#define CPUID_MODEL_MASK        (0xf)
#define CPUID_EXT_MODEL_MASK    (0xf)
#define CPUID_EXT_MODEL_SHIFT   (16)
#define CPUID_FAMILY_MASK       (0xf)
#define CPUID_FAMILY_SHIFT      (8)
#define CPU_MODEL_KNL           (0x57)
#define CPU_MODEL_KNM           (0x85)
#define CPU_FAMILY_INTEL        (0x06)

typedef struct {
    uint32_t model;
    uint32_t family;
} cpu_model_data_t;

static cpu_model_data_t get_cpu_model_data()
{
    registers_t registers;
    cpuid_asm(1, 0, &registers);
    uint32_t model = (registers.eax >> CPUID_MODEL_SHIFT) & CPUID_MODEL_MASK;
    uint32_t model_ext = (registers.eax >> CPUID_EXT_MODEL_SHIFT) &
                         CPUID_EXT_MODEL_MASK;

    cpu_model_data_t data;
    data.model = model | (model_ext << 4);
    data.family = (registers.eax >> CPUID_FAMILY_SHIFT) & CPUID_FAMILY_MASK;
    return data;
}

static bool is_hbm_legacy_supported(cpu_model_data_t cpu)
{
    return cpu.family == CPU_FAMILY_INTEL &&
           (cpu.model == CPU_MODEL_KNL || cpu.model == CPU_MODEL_KNM);
}

static int get_high_bandwidth_nodes(struct bitmask *hbw_node_mask)
{
    int nodes_num = numa_num_configured_nodes();
    // Check if NUMA configuration is supported.
    if(nodes_num == 2 || nodes_num == 4 || nodes_num == 8) {
        struct bitmask *node_cpus = numa_allocate_cpumask();

        assert(hbw_node_mask->size >= nodes_num);
        assert(node_cpus->size >= nodes_num);
        int i;
        for(i=0; i<nodes_num; i++) {
            numa_node_to_cpus(i, node_cpus);
            if(numa_bitmask_weight(node_cpus) == 0) {
                //NUMA nodes without CPU are HBW nodes.
                numa_bitmask_setbit(hbw_node_mask, i);
            }
        }

        numa_bitmask_free(node_cpus);

        if(2*numa_bitmask_weight(hbw_node_mask) == nodes_num) {
            return 0;
        }
    }

    return MEMKIND_ERROR_UNAVAILABLE;
}

static int get_high_bandwidth_nodes_hmat(struct bitmask *hbw_node_mask)
{
#ifdef MEMKIND_HWLOC
    const char *hbw_threshold_env = memkind_get_env("MEMKIND_HBW_THRESHOLD");
    size_t hbw_threshold;
    int err;
    hwloc_topology_t topology;
    hwloc_obj_t init_node = NULL;

    if (hbw_threshold_env) {
        log_info("Environment variable MEMKIND_HBW_THRESHOLD detected: %s.",
                 hbw_threshold_env);
        int ret = sscanf(hbw_threshold_env, "%zud", &hbw_threshold);
        if (ret == 0) {
            log_err("Environment variable MEMKIND_HBW_THRESHOLD is invalid value.");
            return MEMKIND_ERROR_ENVIRON;
        }
    } else {
        hbw_threshold = MEMKIND_HBW_THRESHOLD_DEFAULT;
    }

    err = hwloc_topology_init(&topology);
    if (err) {
        log_err("hwloc initialize failed");
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    err = hwloc_topology_load(topology);
    if (err) {
        log_err("hwloc topology load failed");
        hwloc_topology_destroy(topology);
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    struct bitmask *node_cpus = numa_allocate_cpumask();
    while ((init_node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                   init_node)) != NULL) {
        struct hwloc_location initiator;
        hwloc_obj_t target = NULL;

        numa_node_to_cpus(init_node->os_index, node_cpus);
        if (numa_bitmask_weight(node_cpus) == 0) {
            log_info("Node skipped %d - no CPU detected in initiator Node.",
                     init_node->os_index);
            continue;
        }

        initiator.type = HWLOC_LOCATION_TYPE_CPUSET;
        initiator.location.cpuset = init_node->cpuset;

        while ((target = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                    target)) != NULL) {
            hwloc_uint64_t bandwidth;
            err = hwloc_memattr_get_value(topology, HWLOC_MEMATTR_ID_BANDWIDTH, target,
                                          &initiator, 0, &bandwidth);
            if (err) {
                log_info("Node skipped - cannot read initiator Node %d and target Node %d.",
                         init_node->os_index, target->os_index);
                continue;
            }

            if (bandwidth >= hbw_threshold) {
                numa_bitmask_setbit(hbw_node_mask, target->os_index);
            }
        }
    }

    numa_bitmask_free(node_cpus);
    hwloc_topology_destroy(topology);
    if (numa_bitmask_weight(hbw_node_mask) == 0) {
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    return 0;
#else
    log_err("High Bandwidth NUMA nodes cannot be automatically detected.");
    return MEMKIND_ERROR_OPERATION_FAILED;
#endif
}

///This function tries to fill bandwidth array based on knowledge about known CPU models
static int fill_bandwidth_values_heuristically(int *bandwidth)
{
    int ret;
    cpu_model_data_t cpu = get_cpu_model_data();

    if(is_hbm_legacy_supported(cpu)) {
        ret = bandwidth_fill(bandwidth, get_high_bandwidth_nodes);
    } else {
        ret = bandwidth_fill(bandwidth, get_high_bandwidth_nodes_hmat);
    }

    if(ret != 0) {
        return MEMKIND_ERROR_UNAVAILABLE;
    }

    log_info("Detected High Bandwidth Memory.");
    return ret;
}

static void memkind_hbw_closest_numanode_init(void)
{
    struct hbw_closest_numanode_t *g = &memkind_hbw_closest_numanode_g;
    g->num_cpu = numa_num_configured_cpus();
    g->closest_numanode = NULL;
    g->init_err = set_closest_numanode(fill_bandwidth_values_heuristically,
                                       "MEMKIND_HBW_NODES", &g->closest_numanode, g->num_cpu, true);
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
