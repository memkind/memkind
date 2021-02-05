// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include "memkind/internal/memkind_dax_kmem.h"
#include <memkind.h>

#include <numa.h>
#include <numaif.h>

#include "common.h"

/* This test is run with overridden MEMKIND_DAX_KMEM_NODES environment variable
 * and tries to perform allocation on DRAM using memkind_malloc()
 */
int main()
{
    struct bitmask *expected_nodemask = nullptr;
    struct bitmask *returned_nodemask = nullptr;
    void *ptr = nullptr;
    int ret = 0;
    int status = 0;

    ptr = memkind_malloc(MEMKIND_DAX_KMEM, KB);
    if (ptr != nullptr) {
        printf("Error: allocation to DAX_KMEM should fail\n");
        goto exit;
    }

    ptr = memkind_malloc(MEMKIND_DEFAULT, KB);
    if (ptr == nullptr) {
        printf("Error: allocation failed\n");
        goto exit;
    }

    expected_nodemask = numa_allocate_nodemask();
    status = memkind_dax_kmem_all_get_mbind_nodemask(
        nullptr, expected_nodemask->maskp, expected_nodemask->size);
    if (status != MEMKIND_ERROR_ENVIRON) {
        printf(
            "Error: wrong return value from memkind_dax_kmem_all_get_mbind_nodemask()\n");
        printf("Expected: %d\n", MEMKIND_ERROR_ENVIRON);
        printf("Actual: %d\n", status);
        goto exit;
    }

    returned_nodemask = numa_allocate_nodemask();
    status = get_mempolicy(nullptr, returned_nodemask->maskp,
                           returned_nodemask->size, ptr, MPOL_F_ADDR);
    if (status) {
        printf("Error: get_mempolicy() returned %d\n", status);
        goto exit;
    }

    ret = numa_bitmask_equal(returned_nodemask, expected_nodemask);
    if (!ret) {
        printf(
            "Error: Memkind dax kmem and allocated pointer nodemasks are not equal\n");
    }

exit:
    if (expected_nodemask) {
        numa_free_nodemask(expected_nodemask);
    }
    if (returned_nodemask) {
        numa_free_nodemask(returned_nodemask);
    }
    if (ptr) {
        memkind_free(nullptr, ptr);
    }

    return ret;
}
