// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2020 Intel Corporation. */
#include <memkind/internal/memkind_hbw.h>

#include "common.h"
#include "allocator_perf_tool/GTestAdapter.hpp"
#include "numa.h"

#include <memory>

namespace TestPolicy
{
    typedef std::unique_ptr<struct bitmask, decltype(&numa_free_nodemask)>
    unique_bitmask_ptr;

    unique_bitmask_ptr make_nodemask_ptr()
    {
        return unique_bitmask_ptr(numa_allocate_nodemask(), numa_free_nodemask);
    }

    unique_bitmask_ptr make_cpumask_ptr()
    {
        return unique_bitmask_ptr(numa_allocate_cpumask(), numa_free_nodemask);

    }

    int get_num_of_pages(const size_t size, const size_t page_size)
    {
        size_t pages_number = size / page_size;
        pages_number += size % page_size ? 1 : 0;
        return pages_number;
    }

    std::vector<void *> get_address_of_pages(const void *ptr,
                                             const size_t pages_number, const size_t page_size)
    {
        std::vector<void *> address(pages_number);
        const size_t page_mask = ~(page_size-1);
        address[0] = (void *)((uintptr_t)ptr &
                              page_mask); //aligned address of first page
        for (size_t page_num = 1; page_num < pages_number; page_num++) {
            address[page_num] = (char *)address[page_num-1] + page_size;
        }
        return address;
    }

    void record_page_association(const void *ptr, const size_t size,
                                 const size_t page_size)
    {
        size_t pages_number = get_num_of_pages(size, page_size);
        std::vector<void *> address = get_address_of_pages(ptr, pages_number,
                                                           page_size);

        int max_node_id = numa_max_node();
        std::vector<int> nodes(pages_number);
        std::vector<int> pages_on_node(max_node_id+1);

        if (move_pages(0, pages_number, address.data(), NULL, nodes.data(),
                       MPOL_MF_MOVE)) {
            fprintf(stderr, "Error: move_pages() returned %s\n", strerror(errno));
            return;
        }

        for (size_t i = 0; i < pages_number; i++) {
            if (nodes[i] < 0) {
                fprintf(stderr,"Error: status of page %p is %d\n", address[i], nodes[i]);
                return;
            } else {
                pages_on_node[nodes[i]]++;
            }
        }

        for (size_t i = 0; i < (size_t)max_node_id + 1; i++) {
            if (pages_on_node[i] > 0) {
                char buffer[1024];
                snprintf(buffer, sizeof(buffer), "Node%zd", i);
                GTestAdapter::RecordProperty(buffer, pages_on_node[i]);
            }
        }
    }

    void check_numa_nodes(unique_bitmask_ptr &expected_bitmask, int policy,
                          void *ptr, size_t size)
    {

        const size_t page_size = sysconf(_SC_PAGESIZE);
        size_t pages_number = get_num_of_pages(size, page_size);
        std::vector<void *> address = get_address_of_pages(ptr, pages_number,
                                                           page_size);
        unique_bitmask_ptr returned_bitmask = make_nodemask_ptr();
        int status = -1;

        for (size_t page_num = 0; page_num < address.size(); page_num++) {
            ASSERT_EQ(0, get_mempolicy(&status, returned_bitmask->maskp,
                                       returned_bitmask->size, address[page_num], MPOL_F_ADDR));
            ASSERT_EQ(policy, status);
            switch(policy) {
                case MPOL_INTERLEAVE:
                    EXPECT_TRUE(numa_bitmask_equal(expected_bitmask.get(), returned_bitmask.get()));
                    break;
                case MPOL_DEFAULT:
                    break;
                case MPOL_BIND:
                case MPOL_PREFERRED:
                    for(int i=0; i < numa_num_possible_nodes(); i++) {
                        if(numa_bitmask_isbitset(returned_bitmask.get(), i)) {
                            EXPECT_TRUE(numa_bitmask_isbitset(expected_bitmask.get(), i));
                        }
                    }
                    break;
                default:
                    assert(!"Unknown policy\n");
            }
        }
    }

    void check_hbw_numa_nodes(int policy, void *ptr, size_t size)
    {
        unique_bitmask_ptr expected_bitmask = make_nodemask_ptr();

        memkind_hbw_all_get_mbind_nodemask(NULL, expected_bitmask->maskp,
                                           expected_bitmask->size);
        check_numa_nodes(expected_bitmask, policy, ptr, size);
    }

    void check_all_numa_nodes(int policy, void *ptr, size_t size)
    {
        if (policy != MPOL_INTERLEAVE && policy != MPOL_DEFAULT) return;

        unique_bitmask_ptr expected_bitmask = make_nodemask_ptr();

        for(int i=0; i < numa_num_configured_nodes(); i++) {
            numa_bitmask_setbit(expected_bitmask.get(), i);
        }

        check_numa_nodes(expected_bitmask, policy, ptr, size);
    }

    std::set<int> get_regular_numa_nodes(void)
    {
        struct bitmask *cpu_mask = numa_allocate_cpumask();
        std::set<int> regular_nodes;

        const int MAXNODE_ID = numa_max_node();
        for (int id = 0; id <= MAXNODE_ID; ++id) {
            numa_node_to_cpus(id, cpu_mask);

            if (numa_bitmask_weight(cpu_mask) != 0) {
                regular_nodes.insert(id);
            }
        }
        numa_free_cpumask(cpu_mask);

        return regular_nodes;
    }

    std::set<int> get_highest_capacity_nodes(void)
    {
        std::set<int> highest_capacity_nodes;
        long long max_capacity = 0;

        const int MAXNODE_ID = numa_max_node();
        for (int i = 0; i <= MAXNODE_ID; ++i) {
            long long capacity = numa_node_size64(i, NULL);
            if (capacity == -1) {
                continue;
            }
            if (capacity > max_capacity) {
                highest_capacity_nodes.clear();
                highest_capacity_nodes.insert(i);
            } else if (capacity == max_capacity) {
                highest_capacity_nodes.insert(i);
            }
        }
        return highest_capacity_nodes;
    }

    //TODO unify this with dax_kmem_tests.cpp
    size_t get_free_space(std::set<int> nodes)
    {
        size_t sum_of_free_space = 0;
        long long free_space;

        for(auto const &node: nodes) {
            int result = numa_node_size64(node, &free_space);
            if (result == -1)
                continue;
            sum_of_free_space += free_space;
        }

        return sum_of_free_space;
    }
}
