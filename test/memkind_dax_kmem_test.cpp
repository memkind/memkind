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

#include <assert.h>
#include <climits>
#include <numa.h>
#include <set>
#include <unistd.h>

#include "common.h"

static std::set<int> get_dax_kmem_nodes(void)
{
    struct bitmask *cpu_mask = numa_allocate_cpumask();
    std::set<int> dax_kmem_nodes;
    long long free_space;

    const int MAXNODE_ID = numa_num_configured_nodes();
    for (int id = 0; id < MAXNODE_ID; ++id) {
        numa_node_to_cpus(id, cpu_mask);

        // Check if numa node exists and if it is NUMA node created from persistent memory
        if (numa_node_size64(id, &free_space) > 0 &&
            numa_bitmask_weight(cpu_mask) == 0) {
            dax_kmem_nodes.insert(id);
        }
    }
    numa_free_cpumask(cpu_mask);

    return dax_kmem_nodes;
}

static size_t get_free_dax_kmem_space(void)
{
    size_t sum_of_dax_kmem_free_space = 0;
    long long free_space;
    std::set<int> dax_kmem_nodes = get_dax_kmem_nodes();

    for(auto const &node: dax_kmem_nodes) {
        numa_node_size64(node, &free_space);
        sum_of_dax_kmem_free_space += free_space;
    }

    return sum_of_dax_kmem_free_space;
}

static std::set<int> get_closest_dax_kmem_numa_nodes(void)
{
    int min_distance = INT_MAX;
    int process_cpu = sched_getcpu();
    int process_node = numa_node_of_cpu(process_cpu);
    std::set<int> closest_numa_ids;
    std::set<int> dax_kmem_nodes = get_dax_kmem_nodes();

    for (auto const &node: dax_kmem_nodes) {
        int distance_to_i_node = numa_distance(process_node, node);

        if (distance_to_i_node <= min_distance) {
            min_distance = distance_to_i_node;
            closest_numa_ids.insert(node);
        }
    }

    return closest_numa_ids;
}

static size_t get_used_swap_space(void)
{
    size_t vm_swap = 0;
    bool status = false;
    FILE *fp = fopen("/proc/self/status", "r");

    if (fp) {
        char buffer[BUFSIZ];
        while(fgets(buffer, sizeof (buffer), fp)) {
            if (sscanf(buffer, "VmSwap: %zu kB", &vm_swap) == 1) {
                status = true;
                break;
            }
        }
        fclose(fp);
    }
    assert(status && "Couldn't access swap space");

    return vm_swap * KB;
}

class MemkindDaxKmemTests: public ::testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindDaxKmemTests,
       test_TC_MEMKIND_MEMKIND_DAX_KMEM_free_with_NULL_kind_4096_bytes)
{
    void *ptr = memkind_malloc(MEMKIND_DAX_KMEM, 4096);
    ASSERT_NE(nullptr, ptr);
    memkind_free(nullptr, ptr);

    // TODO: remove four below lines, when adding more tests
    get_used_swap_space();
    get_dax_kmem_nodes();
    get_closest_dax_kmem_numa_nodes();
    get_free_dax_kmem_space();
}
