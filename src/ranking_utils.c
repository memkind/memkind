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
#include <unistd.h>

#define MAX_RETRIES 5

_Static_assert(DRAM == 0 && DAX_KMEM == 1, "Check values in ranking_utils.h");

static nodemask_t nodemasks[2];
static const char *MEMORY_TYPE_NAMES[2] = {"DRAM", "DAX_KMEM"};

static pthread_once_t nodemask_inits[2] = {PTHREAD_ONCE_INIT,
                                           PTHREAD_ONCE_INIT};

static void init_dram_nodemask_once();
static void init_dax_kmem_nodemask_once();

void (*init_routines[2])(void) = {init_dram_nodemask_once,
                                  init_dax_kmem_nodemask_once};

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

    copy_bitmask_to_nodemask(dram_nodes_bm, &nodemasks[DRAM]);
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
        NULL, nodemasks[DAX_KMEM].n, NUMA_NUM_NODES);
    if (status == MEMKIND_ERROR_OPERATION_FAILED) {
        log_fatal(
            "memkind wasn't build with a minimum required libdaxctl-devel version");
        abort();
    } else if (status != MEMKIND_SUCCESS) {
        log_fatal("Failed to get DAX KMEM nodes nodemask");
        abort();
    }

    copy_nodemask_to_bitmask(&nodemasks[DAX_KMEM], bitmask);
    bm_weight = numa_bitmask_weight(bitmask);
    if (bm_weight == 0) {
        log_fatal(
            "Page movement to DAX_KMEM failed: no DAX_KMEM nodes detected");
        abort();
    }
}

MEMKIND_EXPORT int move_page_metadata(uintptr_t start_addr,
                                      memory_type_t memory_type)
{
    int ret = 0;
    int prev_errno = errno;

    const size_t page_size = traced_pagesize_get_sysytem_pagesize();
    if (start_addr % page_size != 0) {
        log_fatal(
            "Page movement failed: address is not aligned with the system page size");
        abort();
    }

    pthread_once(&nodemask_inits[memory_type], init_routines[memory_type]);

    size_t retry_idx = 0;
    for (retry_idx = 0; retry_idx < MAX_RETRIES; ++retry_idx) {
        ret = mbind((void *)start_addr, TRACED_PAGESIZE, MPOL_BIND,
                    nodemasks[memory_type].n, NUMA_NUM_NODES,
                    MPOL_MF_MOVE | MPOL_MF_STRICT);
        if (ret == 0)
            break;
        log_info("Page movement to %s failed with mbind() errno=%d",
                 MEMORY_TYPE_NAMES[memory_type], errno);
    }
    if (ret) {
        log_err("Page movement to %s totally failed after %lu retries",
                MEMORY_TYPE_NAMES[memory_type], retry_idx);
        ret = MEMKIND_ERROR_MBIND;
    }

    errno = prev_errno;
    return ret;
}
