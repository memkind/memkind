// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/memkind_dax_kmem.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/pagesizes.h>
#include <memkind/internal/ranking_utils.h>

#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <stdint.h>
#include <unistd.h>

static nodemask_t dram_nodemask;
static nodemask_t dax_kmem_nodemask;
static pthread_once_t dram_init = PTHREAD_ONCE_INIT;
static pthread_once_t dax_kmem_init = PTHREAD_ONCE_INIT;

static void init_dram_nodemask_once()
{
    unsigned i;

    int err = numa_available();
    if (err) {
        log_fatal("NUMA is not supported (error code:%d)", err);
        abort();
    }

    unsigned nodes_num = (unsigned)numa_max_node() + 1;
    struct bitmask *node_cpus = numa_allocate_cpumask();
    struct bitmask *dram_nodes_bm = numa_allocate_nodemask();

    for (i = 0; i < nodes_num; i++) {
        int ret = numa_node_to_cpus(i, node_cpus);
        if (ret == 0 && numa_bitmask_weight(node_cpus))
            numa_bitmask_setbit(dram_nodes_bm, i);
    }
    numa_bitmask_free(node_cpus);

    copy_bitmask_to_nodemask(dram_nodes_bm, &dram_nodemask);
}

static void init_dax_kmem_nodemask_once()
{
    struct bitmask *bitmask = NULL;
    int bm_weight = 0;

    int err = numa_available();
    if (err) {
        log_fatal("NUMA is not supported (error code:%d)", err);
        abort();
    }
    bitmask = numa_allocate_nodemask();

    int status = memkind_dax_kmem_all_get_mbind_nodemask(
        NULL, dax_kmem_nodemask.n, NUMA_NUM_NODES);
    if (status == MEMKIND_ERROR_OPERATION_FAILED) {
        log_fatal(
            "memkind wasn't build with a minimum required libdaxctl-devel version");
        abort();
    } else if (status != MEMKIND_SUCCESS) {
        log_fatal("Failed to get DAX KMEM nodes nodemask");
        abort();
    }

    copy_nodemask_to_bitmask(&dax_kmem_nodemask, bitmask);
    bm_weight = numa_bitmask_weight(bitmask);
    if (bm_weight == 0) {
        log_fatal(
            "Page movement to DAX_KMEM failed: no DAX_KMEM nodes detected");
        abort();
    }
}

MEMKIND_EXPORT int move_page_metadata(void *page_metadata_addr,
                                      memory_type_t memory_type)
{
    int ret = 0;
    int prev_errno = errno;
    size_t traced_pagesize = traced_pagesize_get_sysytem_pagesize();

    if ((uintptr_t)page_metadata_addr % traced_pagesize != 0) {
        log_fatal("Page metadata address is not aligned with the page size");
        abort();
    }

    switch (memory_type) {
        case DRAM:
            pthread_once(&dram_init, init_dram_nodemask_once);

            ret = mbind(page_metadata_addr, TRACED_PAGESIZE, MPOL_BIND,
                        dram_nodemask.n, NUMA_NUM_NODES,
                        MPOL_MF_MOVE | MPOL_MF_STRICT);
            if (ret) {
                log_err("Page movement to DRAM failed with mbind() errno=%d",
                        errno);
                ret = MEMKIND_ERROR_MBIND;
            }
            break;
        case DAX_KMEM:
            pthread_once(&dax_kmem_init, init_dax_kmem_nodemask_once);

            ret = mbind(page_metadata_addr, TRACED_PAGESIZE, MPOL_BIND,
                        dax_kmem_nodemask.n, NUMA_NUM_NODES,
                        MPOL_MF_MOVE | MPOL_MF_STRICT);
            if (ret) {
                log_err(
                    "Page movement to DAX_KMEM failed with mbind() errno=%d",
                    errno);
                ret = MEMKIND_ERROR_MBIND;
            }
            break;
    }

    errno = prev_errno;
    return ret;
}
