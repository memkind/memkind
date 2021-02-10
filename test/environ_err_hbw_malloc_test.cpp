// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include "memkind/internal/memkind_hbw.h"
#include <hbwmalloc.h>
#include <memkind.h>

#include <numa.h>
#include <numaif.h>

#include "common.h"

/* This test is run with overridden MEMKIND_HBW_NODES environment variable
 * and tries to perform allocation from DRAM using hbw_malloc() using
 * default HBW_POLICY_PREFERRED policy.
 */
int main()
{
    struct bitmask *expected_nodemask = NULL;
    struct bitmask *returned_nodemask = NULL;
    void *ptr = NULL;
    int ret = 0;
    int status = 0;

    ptr = hbw_malloc(KB);
    if (ptr == NULL) {
        printf("Error: allocation failed\n");
        goto exit;
    }

    expected_nodemask = numa_allocate_nodemask();
    status = memkind_hbw_all_get_mbind_nodemask(NULL, expected_nodemask->maskp,
                                                expected_nodemask->size);
    if (status != MEMKIND_ERROR_ENVIRON) {
        printf(
            "Error: wrong return value from memkind_hbw_all_get_mbind_nodemask()\n");
        printf("Expected: %d\n", MEMKIND_ERROR_ENVIRON);
        printf("Actual: %d\n", status);
        goto exit;
    }

    returned_nodemask = numa_allocate_nodemask();
    status = get_mempolicy(NULL, returned_nodemask->maskp,
                           returned_nodemask->size, ptr, MPOL_F_ADDR);
    if (status) {
        printf("Error: get_mempolicy() returned %d\n", status);
        goto exit;
    }

    ret = numa_bitmask_equal(returned_nodemask, expected_nodemask);
    if (!ret) {
        printf(
            "Error: Memkind hbw and allocated pointer nodemasks are not equal\n");
    }

exit:
    if (expected_nodemask) {
        numa_free_nodemask(expected_nodemask);
    }
    if (returned_nodemask) {
        numa_free_nodemask(returned_nodemask);
    }
    if (ptr) {
        hbw_free(ptr);
    }

    return ret;
}
