// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

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

static const char *MEMORY_TYPE_NAMES[2] = {"DRAM", "DAX_KMEM"};
nodemask_t nodemasks[2];

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
