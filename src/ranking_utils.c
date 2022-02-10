// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/memkind_dax_kmem.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/ranking_utils.h>

#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <stdint.h>
#include <unistd.h>

static nodemask_t get_dram_nodemask()
{
    nodemask_t nodemask;

    unsigned i;
    unsigned nodes_num = (unsigned)numa_max_node() + 1;
    struct bitmask *node_cpus = numa_allocate_cpumask();
    struct bitmask *dram_nodes_mask = numa_allocate_nodemask();

    for (i = 0; i < nodes_num; i++) {
        int ret = numa_node_to_cpus(i, node_cpus);
        if (ret == 0 && numa_bitmask_weight(node_cpus))
            numa_bitmask_setbit(dram_nodes_mask, i);
    }
    numa_bitmask_free(node_cpus);

    copy_bitmask_to_nodemask(dram_nodes_mask, &nodemask);

    return nodemask;
}

static nodemask_t get_dax_kmem_nodemask()
{
    nodemask_t nodemask;

    int status = memkind_dax_kmem_all_get_mbind_nodemask(NULL, nodemask.n,
                                                         NUMA_NUM_NODES);
    if (status == MEMKIND_ERROR_OPERATION_FAILED) {
        log_fatal(
            "memkind wasn't build with a minimum required libdaxctl-devel version");
        abort();
    } else if (status != MEMKIND_SUCCESS) {
        log_fatal("Failed to get DAX KMEM nodes nodemask");
        abort();
    }

    return nodemask;
}

MEMKIND_EXPORT int move_page_metadata(void *page_metadata_addr, size_t size,
                                      memory_type_t memory_type)
{
#define use_mbind 1
    nodemask_t nodemask;
    struct bitmask *bitmask = NULL;
    int ret;
    int prev_errno = errno;
    int bm_weight = 0;
#ifndef use_mbind
    int max_node = numa_max_node();
    int *status = memkind_malloc(MEMKIND_DEFAULT, sizeof(int));
    int *nodemask_int = memkind_calloc(MEMKIND_DEFAULT, max_node, sizeof(int));
#endif

    int err = numa_available();
    if (err) {
        log_fatal("NUMA is not supported (error code:%d)", err);
        abort();
    }
    bitmask = numa_allocate_nodemask();

#ifndef use_mbind
    struct bitmask nodemask_dax_kmem = {NUMA_NUM_NODES, nodemask.n};
    numa_bitmask_clearall(&nodemask_dax_kmem);
#endif
    if ((uintptr_t)page_metadata_addr % sysconf(_SC_PAGESIZE) != 0) {
        log_fatal("Page metadata address is not aligned with the page size");
        abort();
    }

    if (memory_type == DRAM) {
        nodemask = get_dram_nodemask();

        ret = mbind(page_metadata_addr, size, MPOL_BIND, nodemask.n,
                    NUMA_NUM_NODES, MPOL_MF_MOVE | MPOL_MF_STRICT);
        if (ret) {
            log_err("Page movement to DRAM failed with mbind() errno=%d",
                    errno);
            ret = MEMKIND_ERROR_MBIND;
        }
    } else {
        nodemask = get_dax_kmem_nodemask();

        copy_nodemask_to_bitmask(&nodemask, bitmask);
        bm_weight = numa_bitmask_weight(bitmask);
        if (bm_weight == 0) {
            log_err(
                "Page movement to DAX_KMEM failed: no DAX_KMEM nodes detected");
            return -1;
        }

#ifdef use_mbind
        ret = mbind(page_metadata_addr, size, MPOL_BIND, nodemask.n,
                    NUMA_NUM_NODES, MPOL_MF_MOVE | MPOL_MF_STRICT);
        if (ret) {
            log_err("Page movement to DAX_KMEM failed with mbind() errno=%d",
                    errno);
            ret = MEMKIND_ERROR_MBIND;
        }
#else
        int i;
        for (i = 0; i < NUMA_NUM_NODES; ++i) {
            if (numa_bitmask_isbitset(&nodemask_dax_kmem, i)) {
                nodemask_int[i] = 1;
                log_info("nodemask[%d]=%d", i, nodemask_int[i]);
            }
        }
        *status = -1;
        ret =
            move_pages(0, 1, &os_page_addr, nodemask_int, status, MPOL_MF_MOVE);
        log_info("move_pages status=%d", *status);
        if (ret) {
            log_err(
                "Page movement to DAX_KMEM failed with move_pages() errno=%d, status=%d",
                errno, *status);
            ret = -1;
        }
#endif
    }

    errno = prev_errno;
    return ret;
}
