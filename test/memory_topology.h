// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once

#include <memkind.h>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include <numaif.h>

struct Nodes {
    int init;
    int target;
};
using NodeSet = std::pair<int, std::unordered_set<int>>;
using MapNodeSet = std::unordered_map<int, std::unordered_set<int>>;

class AbstractTopology
{
private:
    virtual MapNodeSet HBW_nodes() const
    {
        return {};
    }

    virtual MapNodeSet Capacity_local_nodes() const
    {
        return {};
    }

    int get_kind_mem_policy_flag(memkind_t memory_kind) const
    {
        if (memory_kind == MEMKIND_HBW ||
            memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL)
            return MPOL_BIND;
        return -1;
    }

    bool test_node_set(const Nodes &nodes, const MapNodeSet &map_nodes) const
    {
        auto init_node_tpg = map_nodes.find(nodes.init);

        if (init_node_tpg != map_nodes.end()) {
            auto target_nodes_tpg = init_node_tpg->second;
            auto found_node = target_nodes_tpg.find(nodes.target);

            if (found_node != target_nodes_tpg.end()) {
                return true;
            }
        }
        std::cout << "Failed for, Init: " << nodes.init << " Target: " << nodes.target
                  << std::endl;
        return false;
    }

    std::unordered_set<int> get_target_nodes(int init,
                                             const MapNodeSet &map_nodes) const
    {
        auto init_node_tpg = map_nodes.find(init);
        if (init_node_tpg != map_nodes.end()) {
            return init_node_tpg->second;
        }
        std::cout << "Failed for, Init: " << init << std::endl;
        return {};
    }

public:
    bool is_kind_supported(memkind_t memory_kind) const
    {
        if (memory_kind == MEMKIND_HBW)
            return (HBW_nodes().size() > 0);
        else if (memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL)
            return (Capacity_local_nodes().size() > 0);
        return false;
    }

    bool verify_nodes(memkind_t memory_kind, const Nodes &nodes) const
    {
        if (memory_kind == MEMKIND_HBW)
            return test_node_set(nodes, HBW_nodes());
        else if (memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL)
            return test_node_set(nodes, Capacity_local_nodes());
        return false;
    }

    bool verify_addr(memkind_t memory_kind, void *ptr) const
    {
        int policy = -1;
        int status = get_mempolicy(&policy, nullptr, 0, ptr, MPOL_F_ADDR);
        if (status) {
            return false;
        }
        return policy == get_kind_mem_policy_flag(memory_kind);
    }

    std::unordered_set<int> get_target_nodes(memkind_t memory_kind, int init) const
    {
        if (memory_kind == MEMKIND_HBW)
            return get_target_nodes(init, HBW_nodes());
        else if (memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL)
            return get_target_nodes(init, Capacity_local_nodes());
        return {};
    }

    virtual ~AbstractTopology() = default;
};

class KNM_All2All : public AbstractTopology
{
private:
    MapNodeSet HBW_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {1}));
        return nodeset_map;
    }

    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        return nodeset_map;
    }
};

class KNM_SNC2 : public AbstractTopology
{
private:
    MapNodeSet HBW_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {3}));
        return nodeset_map;
    }

    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class KNM_SNC4 : public AbstractTopology
{
private:
    MapNodeSet HBW_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {5}));
        nodeset_map.emplace(NodeSet(2, {6}));
        nodeset_map.emplace(NodeSet(3, {7}));
        return nodeset_map;
    }

    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_2_var1 : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class CLX_2_var2 : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {2}));
        return nodeset_map;
    }
};

class CLX_2_var3 : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {3}));
        return nodeset_map;
    }
};

class CLX_4_var1 : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {5}));
        nodeset_map.emplace(NodeSet(2, {6}));
        nodeset_map.emplace(NodeSet(3, {7}));
        return nodeset_map;
    }
};

class CLX_4_var2 : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {4}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var3 : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {5}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {6}));
        return nodeset_map;
    }
};

class CLX_4_var4 : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {5}));
        return nodeset_map;
    }
};