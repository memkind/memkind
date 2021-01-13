// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <unordered_set>

class DaxKmem
{
public:
    DaxKmem();
    size_t get_free_space(void);
    std::unordered_set<int> get_closest_numa_nodes(int regular_node);
    size_t size(void);
    bool contains(int node);

private:
    std::unordered_set<int> dax_kmem_nodes;
    std::unordered_set<int> get_dax_kmem_nodes(void);
}; // DaxKmem
