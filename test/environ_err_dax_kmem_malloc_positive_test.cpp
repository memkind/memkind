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

#include <climits>

/* This test is run with overridden MEMKIND_DAX_KMEM_NODES environment variable
 * and tries to perform allocation on persistent memory using memkind_malloc()
 */
int main()
{
    struct bitmask *all_dax_kmem_nodes_nodemask = NULL;
    void *ptr = NULL;
    int ret = 0;
    int expected_numa_id = -1;
    int returned_numa_id = -1;
    char *env_value_str = secure_getenv("MEMKIND_DAX_KMEM");
    int process_cpu = sched_getcpu();
    int process_node = numa_node_of_cpu(process_cpu);
    unsigned int numa_count = numa_bitmask_nbytes(all_dax_kmem_nodes_nodemask);

    ptr = memkind_malloc(MEMKIND_DAX_KMEM, KB);
    char *char_ptr = static_cast<char *>(ptr);
    if (ptr == NULL) {
        printf("Error: allocation failed\n");
        goto exit;
    }

    sprintf(char_ptr, "abc");

    if (!env_value_str) {
        printf("Error: MEMKIND_DAX_KMEM environment variable is empty.\n");
        goto exit;
    }

    all_dax_kmem_nodes_nodemask = numa_parse_nodestring(env_value_str);

    for (unsigned int i = 0; i < numa_count; i++) {
        int min_distance = INT_MAX;
        int distance_to_i_node;
        if (numa_bitmask_isbitset(all_dax_kmem_nodes_nodemask, i)) {
            distance_to_i_node = numa_distance(process_node, i);
            if (distance_to_i_node < min_distance) {
                min_distance = distance_to_i_node;
                expected_numa_id = i;
            }
        }
    }

    get_mempolicy(&returned_numa_id, NULL, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);

    ret = (expected_numa_id == returned_numa_id) ? 1 : 0;
    if (!ret) {
        printf("Error: Memkind dax kmem and allocated pointer node id's are not equal\n");
    }

exit:
    if (all_dax_kmem_nodes_nodemask) {
        numa_free_nodemask(all_dax_kmem_nodes_nodemask);
    }
    if (char_ptr) {
        memkind_free(NULL, char_ptr);
    }
    if (ptr) {
        memkind_free(NULL, char_ptr);
    }

    return ret;
}
