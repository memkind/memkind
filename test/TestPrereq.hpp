// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */
#pragma once

#include <climits>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include "numa.h"
#include "numaif.h"
#include "config.h"
#ifdef MEMKIND_HWLOC
#include "hwloc.h"
#endif
class TestPrereq
{
private:
    enum memory_var {HBM, PMEM};

#ifdef MEMKIND_HWLOC
    hwloc_topology_t topology;

    bool find_type(memory_var mem) const
    {
        hwloc_obj_t node = nullptr;
        while ((node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE,
                                                  node)) != nullptr) {
            if (mem == PMEM &&
                hwloc_obj_get_info_by_name(node, "DAXDevice") != nullptr) {
                return true;
            } else if (mem == HBM &&
                       node->subtype && !strcmp(node->subtype, "MCDRAM")) {
                return true;
            }
        }
        return false;
    }
#define HWLOC_SUPPORT           (true)
#else
#define find_type(memory_var)   (false)
#define HWLOC_SUPPORT           (false)
#endif

#ifdef MEMKIND_DAX_KMEM
#define DAXCTL_SUPPORT (true)
#else
#define DAXCTL_SUPPORT (false)
#endif

#define CPUID_MODEL_SHIFT       (4)
#define CPUID_MODEL_MASK        (0xf)
#define CPUID_EXT_MODEL_MASK    (0xf)
#define CPUID_EXT_MODEL_SHIFT   (16)
#define CPUID_FAMILY_MASK       (0xf)
#define CPUID_FAMILY_SHIFT      (8)
#define CPU_MODEL_KNL           (0x57)
#define CPU_MODEL_KNM           (0x85)
#define CPU_MODEL_CLX           (0x55)
#define CPU_FAMILY_INTEL        (0x06)

    typedef struct {
        uint32_t model;
        uint32_t family;
    } cpu_model_data_t;

    typedef struct registers_t {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    } registers_t;

    inline void cpuid_asm(int leaf, int subleaf, registers_t *registers) const
    {
#ifdef __x86_64__
        asm volatile("cpuid":"=a"(registers->eax),
                     "=b"(registers->ebx),
                     "=c"(registers->ecx),
                     "=d"(registers->edx):"0"(leaf), "2"(subleaf));
#else
        registers->eax = 0;
#endif
    }

    cpu_model_data_t get_cpu_model_data() const
    {
        registers_t registers;
        cpuid_asm(1, 0, &registers);
        uint32_t model = (registers.eax >> CPUID_MODEL_SHIFT) & CPUID_MODEL_MASK;
        uint32_t model_ext = (registers.eax >> CPUID_EXT_MODEL_SHIFT) &
                             CPUID_EXT_MODEL_MASK;

        cpu_model_data_t data;
        data.model = model | (model_ext << 4);
        data.family = (registers.eax >> CPUID_FAMILY_SHIFT) & CPUID_FAMILY_MASK;
        return data;
    }

    bool check_support(const char *env_var, memory_var mem_var) const
    {
        if (getenv(env_var) != NULL)
            return true;
        else if (!check_cpu(mem_var))
            return false;
        else if (find_type(mem_var))
            return true;
        auto mem_only_nodes = get_memory_only_numa_nodes();
        return mem_only_nodes.size() != 0;
    }
public:
#ifdef MEMKIND_HWLOC
    TestPrereq()
    {
        if (hwloc_topology_init(&topology))
            throw std::runtime_error("Cannot initialize hwloc topology.");
        if (hwloc_topology_load(topology))
            throw std::runtime_error("Cannot load hwloc topology.");
    }
    ~TestPrereq()
    {
        hwloc_topology_destroy(topology);
    }
#endif

    bool check_cpu(memory_var variant) const
    {
        switch(variant) {
            case HBM:
                return is_KNL_supported();
            case PMEM: {
                cpu_model_data_t cpu = get_cpu_model_data();
                return cpu.family == CPU_FAMILY_INTEL && (cpu.model == CPU_MODEL_CLX);
            }
            default:
                return false;
        }
    }

    std::unordered_set<int> get_closest_numa_nodes(int first_node,
                                                   std::unordered_set<int> nodes) const
    {
        int min_distance = INT_MAX;
        std::unordered_set<int> closest_numa_ids;

        for (auto const &node: nodes) {
            int distance_to_i_node = numa_distance(first_node, node);

            if (distance_to_i_node < min_distance) {
                min_distance = distance_to_i_node;
                closest_numa_ids.clear();
                closest_numa_ids.insert(node);
            } else if (distance_to_i_node == min_distance) {
                closest_numa_ids.insert(node);
            }
        }
        return closest_numa_ids;
    }

    std::unordered_set<int> get_highest_capacity_nodes(void)
    {
        std::unordered_set<int> highest_capacity_nodes;
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
                max_capacity = capacity;
            } else if (capacity == max_capacity) {
                highest_capacity_nodes.insert(i);
            }
        }
        return highest_capacity_nodes;
    }

    bool is_libhwloc_supported() const
    {
        return HWLOC_SUPPORT;
    }

    bool is_KNL_supported() const
    {
        cpu_model_data_t cpu = get_cpu_model_data();
        return cpu.family == CPU_FAMILY_INTEL &&
               (cpu.model == CPU_MODEL_KNL || cpu.model == CPU_MODEL_KNM);
    }

    bool is_libdaxctl_supported() const
    {
        return DAXCTL_SUPPORT;
    }

    std::unordered_set<int> get_memory_only_numa_nodes() const
    {
        struct bitmask *cpu_mask = numa_allocate_cpumask();
        std::unordered_set<int> mem_only_nodes;

        const int MAXNODE_ID = numa_max_node();
        for (int id = 0; id <= MAXNODE_ID; ++id) {
            int res = numa_node_to_cpus(id, cpu_mask);
            if (res == -1) {
                continue;
            }

            if (numa_node_size64(id, nullptr) > 0 &&
                numa_bitmask_weight(cpu_mask) == 0) {
                mem_only_nodes.insert(id);
            }
        }
        numa_free_cpumask(cpu_mask);

        return mem_only_nodes;
    }

    std::unordered_set<int> get_regular_numa_nodes() const
    {
        struct bitmask *cpu_mask = numa_allocate_cpumask();
        std::unordered_set<int> regular_nodes;

        const int MAXNODE_ID = numa_max_node();
        for (int id = 0; id <= MAXNODE_ID; ++id) {
            int res = numa_node_to_cpus(id, cpu_mask);
            if (res == -1) {
                continue;
            }
            if (numa_bitmask_weight(cpu_mask) != 0) {
                regular_nodes.insert(id);
            }
        }
        numa_free_cpumask(cpu_mask);

        return regular_nodes;
    }

    size_t get_free_space(std::unordered_set<int> nodes) const
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

    bool is_kind_preferred(memkind_t kind) const
    {
        return (kind == MEMKIND_HBW_PREFERRED ||
                kind == MEMKIND_HBW_PREFERRED_HUGETLB ||
                kind == MEMKIND_HBW_PREFERRED_GBTLB ||
                kind == MEMKIND_DAX_KMEM_PREFERRED ||
                kind == MEMKIND_HIGHEST_CAPACITY_PREFERRED ||
                kind == MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED ||
                kind == MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED ||
                kind == MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED);
    }

    bool is_preferred_supported() const
    {
        auto regular_nodes = get_regular_numa_nodes();
        auto mem_only_nodes = get_memory_only_numa_nodes();
        for (auto const &node: regular_nodes) {
            auto closest_mem_only_nodes = get_closest_numa_nodes(node, mem_only_nodes);
            if (closest_mem_only_nodes.size() > 1) {
                std::cout << "More than one NUMA Node are in the same distance to: " << node <<
                          std::endl;
                return false;
            }
        }
        return true;
    }

    bool is_MCDRAM_supported() const
    {
        return check_support("MEMKIND_HBW_NODES", HBM);
    }
    bool is_DAX_KMEM_supported() const
    {
        return check_support("MEMKIND_DAX_KMEM_NODES", PMEM);
    }
};
