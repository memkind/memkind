// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once

#include <memkind.h>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include <numa.h>
#include <numaif.h>

struct Nodes {
    int init;
    int target;
};
using NodeSet = std::pair<int, std::unordered_set<int>>;
using MapNodeSet = std::unordered_map<int, std::unordered_set<int>>;
using BitmaskPtr =
    std::unique_ptr<struct bitmask, decltype(&numa_free_nodemask)>;

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

    virtual MapNodeSet Latency_local_nodes() const
    {
        return {};
    }

    int get_kind_mem_policy_flag(memkind_t memory_kind) const
    {
        if (memory_kind == MEMKIND_HBW ||
            memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL ||
            memory_kind == MEMKIND_LOWEST_LATENCY_LOCAL)
            return MPOL_BIND;
        else if (memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED ||
                 memory_kind == MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED)
            return MPOL_PREFERRED;
        return -1;
    }

    MapNodeSet get_memory_kind_Nodes(memkind_t memory_kind) const
    {
        if (memory_kind == MEMKIND_HBW)
            return HBW_nodes();
        else if (memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL ||
                 memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED)
            return Capacity_local_nodes();
        else if (memory_kind == MEMKIND_LOWEST_LATENCY_LOCAL ||
                 memory_kind == MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED)
            return Latency_local_nodes();
        else return {};
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

    bool verify_nodes(memkind_t memory_kind, const Nodes &nodes) const
    {
        auto memory_kind_nodes = get_memory_kind_Nodes(memory_kind);
        return test_node_set(nodes, memory_kind_nodes);
    }

public:
    bool is_kind_supported(memkind_t memory_kind) const
    {
        return get_memory_kind_Nodes(memory_kind).size() > 0;
    }

    bool verify_kind(memkind_t memory_kind, int init_node, void *ptr) const
    {
        int policy = -1;
        int target_node = -1;
        int status = -1;

        BitmaskPtr ptr_nodemask = BitmaskPtr(numa_allocate_nodemask(),
                                             numa_free_nodemask);
        BitmaskPtr target_nodemask = BitmaskPtr(numa_allocate_nodemask(),
                                                numa_free_nodemask);

        //verify Node from allocation
        status = get_mempolicy(&target_node, nullptr, 0, ptr,
                               MPOL_F_NODE | MPOL_F_ADDR);
        if (status) {
            return false;
        }
        if (!verify_nodes(memory_kind, {init_node, target_node})) {
            return false;
        }

        //verify mbind mask from allocation
        status = get_mempolicy(&policy, ptr_nodemask->maskp, ptr_nodemask->size,
                               ptr, MPOL_F_ADDR);
        if (status) {
            return false;
        }

        for (auto const &node_id: get_target_nodes(memory_kind, init_node))
            numa_bitmask_setbit(target_nodemask.get(), node_id);

        if (numa_bitmask_equal(ptr_nodemask.get(), target_nodemask.get()) !=1 ) {
            return false;
        }

        if (policy != get_kind_mem_policy_flag(memory_kind)) {
            return false;
        }

        return true;
    }

    std::unordered_set<int> get_target_nodes(memkind_t memory_kind, int init) const
    {
        auto memory_kind_nodes = get_memory_kind_Nodes(memory_kind);
        return get_target_nodes(init, memory_kind_nodes);
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

class CLX_2_var1_HMAT : public CLX_2_var1
{
private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class CLX_2_var1_HBW : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }

    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
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

class CLX_2_var2_HMAT : public CLX_2_var2
{
private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class CLX_2_var2_HBW : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }

    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
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

class CLX_2_var3_HMAT : public CLX_2_var3
{
private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class CLX_2_var3_HBW : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }

    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
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

class CLX_4_var1_HMAT : public CLX_4_var1
{
private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var1_HBW : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }

    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
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

class CLX_4_var2_HMAT : public CLX_4_var2
{
private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var2_HBW : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }

    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
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

class CLX_4_var3_HMAT : public CLX_4_var3
{
private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var3_HBW : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }

    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
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

class CLX_4_var4_HMAT : public CLX_4_var4
{
private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var4_HBW : public AbstractTopology
{
private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }

    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};
