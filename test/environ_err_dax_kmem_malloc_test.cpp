/*
 * Copyright (C) 2019 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memkind.h>
#include "memkind/internal/memkind_dax_kmem.h"

#include <numa.h>
#include <numaif.h>

#include "common.h"

/* This test is run with overridden MEMKIND_DAX_KMEM_NODES environment variable
 * and tries to perform allocation on DRAM using memkind_malloc()
 */
int main()
{
    struct bitmask *expected_nodemask = NULL;
    struct bitmask *returned_nodemask = NULL;
    void *ptr = NULL;
    int ret = 0;
    int status = 0;

    ptr = memkind_malloc(MEMKIND_DAX_KMEM, KB);
    if (ptr != NULL) {
        printf("Error: allocation to DAX_KMEM should fail\n");
        goto exit;
    }

    ptr = memkind_malloc(MEMKIND_DEFAULT, KB);
    if (ptr == NULL) {
        printf("Error: allocation failed\n");
        goto exit;
    }

    expected_nodemask = numa_allocate_nodemask();
    status = memkind_dax_kmem_all_get_mbind_nodemask(NULL, expected_nodemask->maskp,
                                                     expected_nodemask->size);
    if (status != MEMKIND_ERROR_ENVIRON) {
        printf("Error: wrong return value from memkind_dax_kmem_all_get_mbind_nodemask()\n");
        printf("Expected: %d\n", MEMKIND_ERROR_ENVIRON);
        printf("Actual: %d\n", status);
        goto exit;
    }

    returned_nodemask = numa_allocate_nodemask();
    status = get_mempolicy(NULL, returned_nodemask->maskp, returned_nodemask->size,
                           ptr, MPOL_F_ADDR);
    if (status) {
        printf("Error: get_mempolicy() returned %d\n", status);
        goto exit;
    }

    ret = numa_bitmask_equal(returned_nodemask, expected_nodemask);
    if (!ret) {
        printf("Error: Memkind dax kmem and allocated pointer nodemasks are not equal\n");
    }

exit:
    if (expected_nodemask) {
        numa_free_nodemask(expected_nodemask);
    }
    if (returned_nodemask) {
        numa_free_nodemask(returned_nodemask);
    }
    if (ptr) {
        memkind_free(NULL, ptr);
    }

    return ret;
}
