// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */
#pragma once

#include <fstream>
#include <iostream>
#include <numa.h>
#include <stdexcept>
#include <string.h>
#include <vector>

/*
 * HugePageOrganizer sets hugepages per NUMA node and restore initial setup of
 * hugepages. It writes and reads from the same file, so using HugePageOrganizer
 * with parallel execution may cause undefined behaviour.
 */

class HugePageOrganizer
{
public:
    HugePageOrganizer(int nr_hugepages_per_node)
    {
        for (int node_id = 0; node_id < numa_num_configured_nodes();
             node_id++) {
            initial_nr_hugepages_per_nodes.push_back(get_nr_hugepages(node_id));
            if (set_nr_hugepages(nr_hugepages_per_node, node_id)) {
                restore();
                throw std::runtime_error(
                    "Error: Could not set the requested amount of huge pages.");
            }
        }
    }

    ~HugePageOrganizer()
    {
        restore();
    }

private:
    std::vector<int> initial_nr_hugepages_per_nodes;

    int get_nr_hugepages(int node_number)
    {
        std::string line;
        char path[128];
        sprintf(
            path,
            "/sys/devices/system/node/node%d/hugepages/hugepages-2048kB/nr_hugepages",
            node_number);
        std::ifstream file(path);
        if (!file) {
            return -1;
        }
        std::getline(file, line);
        return strtol(line.c_str(), 0, 10);
    }

    int set_nr_hugepages(int nr_hugepages, int node_number)
    {
        char cmd[128];
        sprintf(cmd, "sudo sh -c \"echo %d > \
            /sys/devices/system/node/node%d/hugepages/hugepages-2048kB/nr_hugepages\"",
                nr_hugepages, node_number);
        if (system(cmd) || (get_nr_hugepages(node_number) != nr_hugepages)) {
            return -1;
        }
        return 0;
    }

    void restore()
    {
        for (size_t node_id = 0;
             node_id < initial_nr_hugepages_per_nodes.size(); node_id++) {
            set_nr_hugepages(initial_nr_hugepages_per_nodes[node_id], node_id);
        }
    }
};
