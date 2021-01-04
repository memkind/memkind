// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once

#include <memkind.h>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>

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
    bool test_node_set(struct Nodes nodes, MapNodeSet map_nodes ) const
    {
        auto init_node_tpg = map_nodes.find(nodes.init);

        if (init_node_tpg != map_nodes.end()) {
            auto target_nodes_tpg = init_node_tpg->second;
            auto founded_node = target_nodes_tpg.find(nodes.target);

            if (founded_node != target_nodes_tpg.end()) {
                return true;
            }
        }
        std::cout << "Failed for, Init:" << nodes.init << "Target: " << nodes.target <<
                  std::endl;
        return false;
    }
public:
    bool is_kind_supported(memkind_t memory_kind) const
    {
        if (memory_kind == MEMKIND_HBW)
            return (HBW_nodes().size() > 0);
        return false;
    }

    bool verify_kind(memkind_t memory_kind, struct Nodes nodes) const
    {
        if (memory_kind == MEMKIND_HBW)
            return test_node_set(nodes, std::move(HBW_nodes()));
        return false;
    }
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
};

class KNM_SNC4 : public AbstractTopology
{
    MapNodeSet HBW_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {5}));
        nodeset_map.emplace(NodeSet(2, {6}));
        nodeset_map.emplace(NodeSet(3, {7}));
        return nodeset_map;
    }
};

class CLX_2_var1 : public AbstractTopology
{};

class CLX_4_var1 : public AbstractTopology
{};
