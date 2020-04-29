// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <set>

class DaxKmem
{
public:
    DaxKmem();
    size_t get_free_space(void);
    std::set<int> get_closest_numa_nodes(int regular_node);
    size_t size(void);
    bool contains(int node);

private:
    std::set<int> dax_kmem_nodes;
    std::set<int> get_dax_kmem_nodes(void);
}; // DaxKmem
