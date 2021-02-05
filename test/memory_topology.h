// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once

#include <memkind.h>

#include "TestPrereq.hpp"
#include <bitset>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

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
protected:
    virtual MapNodeSet HBW_nodes() const
    {
        return {};
    }

    virtual MapNodeSet HBW_all_nodes() const
    {
        return HBW_nodes();
    }

    virtual MapNodeSet Capacity_local_nodes() const
    {
        return {};
    }

    virtual MapNodeSet Latency_local_nodes() const
    {
        return {};
    }

    virtual MapNodeSet Bandwidth_local_nodes() const
    {
        return {};
    }

private:
    int get_kind_mem_policy_flag() const
    {
        if (m_memory_kind == MEMKIND_HBW || m_memory_kind == MEMKIND_HBW_ALL ||
            m_memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL ||
            m_memory_kind == MEMKIND_LOWEST_LATENCY_LOCAL ||
            m_memory_kind == MEMKIND_HIGHEST_BANDWIDTH_LOCAL)
            return MPOL_BIND;
        else if (m_memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED ||
                 m_memory_kind == MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED ||
                 m_memory_kind == MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED)
            return MPOL_PREFERRED;
        return -1;
    }

    MapNodeSet get_memory_kind_Nodes() const
    {
        if (m_memory_kind == MEMKIND_HBW)
            return HBW_nodes();
        else if (m_memory_kind == MEMKIND_HBW_ALL)
            return HBW_all_nodes();
        else if (m_memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL ||
                 m_memory_kind == MEMKIND_HIGHEST_CAPACITY_LOCAL_PREFERRED)
            return Capacity_local_nodes();
        else if (m_memory_kind == MEMKIND_LOWEST_LATENCY_LOCAL ||
                 m_memory_kind == MEMKIND_LOWEST_LATENCY_LOCAL_PREFERRED)
            return Latency_local_nodes();
        else if (m_memory_kind == MEMKIND_HIGHEST_BANDWIDTH_LOCAL ||
                 m_memory_kind == MEMKIND_HIGHEST_BANDWIDTH_LOCAL_PREFERRED)
            return Bandwidth_local_nodes();
        else
            return {};
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
        std::cout << "Failed for, Init: " << nodes.init
                  << " Target: " << nodes.target << std::endl;
        return false;
    }
    TestPrereq m_tp;

protected:
    memkind_t m_memory_kind;

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

    bool verify_nodes(const Nodes &nodes) const
    {
        auto memory_kind_nodes = get_memory_kind_Nodes();
        return test_node_set(nodes, memory_kind_nodes);
    }

public:
    bool is_kind_supported() const
    {
        if (!m_tp.is_HMAT_supported() &&
            m_tp.is_kind_required_HMAT(m_memory_kind)) {
            return false;
        }
        if (!m_tp.is_libhwloc_supported() &&
            m_tp.is_kind_required_libhwloc(m_memory_kind)) {
            return false;
        }
        return get_memory_kind_Nodes().size() > 0;
    }

    void *allocate(size_t size) const
    {
        return memkind_malloc(m_memory_kind, size);
    }

    void deallocate(void *ptr) const
    {
        memkind_free(m_memory_kind, ptr);
    }

    bool verify_kind(int init_node, void *ptr) const
    {
        int policy = -1;
        int target_node = -1;
        int status = -1;

        BitmaskPtr ptr_nodemask =
            BitmaskPtr(numa_allocate_nodemask(), numa_free_nodemask);
        BitmaskPtr target_nodemask =
            BitmaskPtr(numa_allocate_nodemask(), numa_free_nodemask);

        // verify Node from allocation
        status = get_mempolicy(&target_node, nullptr, 0, ptr,
                               MPOL_F_NODE | MPOL_F_ADDR);
        if (status) {
            return false;
        }
        if (!verify_nodes({init_node, target_node})) {
            return false;
        }

        // verify mbind mask from allocation
        status = get_mempolicy(&policy, ptr_nodemask->maskp, ptr_nodemask->size,
                               ptr, MPOL_F_ADDR);
        if (status) {
            return false;
        }

        for (auto const &node_id : get_target_nodes(init_node))
            numa_bitmask_setbit(target_nodemask.get(), node_id);

        if (numa_bitmask_equal(ptr_nodemask.get(), target_nodemask.get()) !=
            1) {
            std::bitset<64> ptr_bits(*ptr_nodemask.get()->maskp);
            std::bitset<64> target_bits(*target_nodemask.get()->maskp);
            std::cout << "Failed for, Init mask: " << ptr_bits
                      << " Target mask: " << target_bits << std::endl;
            return false;
        }

        if (policy != get_kind_mem_policy_flag()) {
            return false;
        }

        return true;
    }

    std::unordered_set<int> get_target_nodes(int init) const
    {
        auto memory_kind_nodes = get_memory_kind_Nodes();
        return get_target_nodes(init, memory_kind_nodes);
    }
    AbstractTopology(memkind_t kind) : m_memory_kind(kind)
    {}
    AbstractTopology() = delete;
    virtual ~AbstractTopology() = default;
};

class KNM_All2All: public AbstractTopology
{
public:
    KNM_All2All(memkind_t kind) : AbstractTopology(kind){};

protected:
    MapNodeSet HBW_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {1}));
        return nodeset_map;
    }

private:
    MapNodeSet Bandwidth_local_nodes() const final
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

class KNM_SNC2: public AbstractTopology
{
public:
    KNM_SNC2(memkind_t kind) : AbstractTopology(kind){};

protected:
    MapNodeSet HBW_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {3}));
        return nodeset_map;
    }

    MapNodeSet HBW_all_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2, 3}));
        nodeset_map.emplace(NodeSet(1, {2, 3}));
        return nodeset_map;
    }

private:
    MapNodeSet Bandwidth_local_nodes() const final
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

class KNM_SNC4: public AbstractTopology
{
public:
    KNM_SNC4(memkind_t kind) : AbstractTopology(kind){};

protected:
    MapNodeSet HBW_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {5}));
        nodeset_map.emplace(NodeSet(2, {6}));
        nodeset_map.emplace(NodeSet(3, {7}));
        return nodeset_map;
    }

    MapNodeSet HBW_all_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4, 5, 6, 7}));
        nodeset_map.emplace(NodeSet(1, {4, 5, 6, 7}));
        nodeset_map.emplace(NodeSet(2, {4, 5, 6, 7}));
        nodeset_map.emplace(NodeSet(3, {4, 5, 6, 7}));
        return nodeset_map;
    }

private:
    MapNodeSet Bandwidth_local_nodes() const final
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

class CLX_2_var1: public AbstractTopology
{
public:
    CLX_2_var1(memkind_t kind) : AbstractTopology(kind){};

private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class CLX_2_var1_HMAT: public CLX_2_var1
{
public:
    CLX_2_var1_HMAT(memkind_t kind) : CLX_2_var1(kind){};

private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class CLX_2_var1_HBW: public AbstractTopology
{
public:
    CLX_2_var1_HBW(memkind_t kind) : AbstractTopology(kind){};

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class CLX_2_var2: public AbstractTopology
{
public:
    CLX_2_var2(memkind_t kind) : AbstractTopology(kind){};

private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {2}));
        return nodeset_map;
    }
};

class CLX_2_var2_HMAT: public CLX_2_var2
{
public:
    CLX_2_var2_HMAT(memkind_t kind) : CLX_2_var2(kind){};

private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class CLX_2_var2_HBW: public AbstractTopology
{
public:
    CLX_2_var2_HBW(memkind_t kind) : AbstractTopology(kind){};

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {2}));
        return nodeset_map;
    }
};

class CLX_2_var3: public AbstractTopology
{
public:
    CLX_2_var3(memkind_t kind) : AbstractTopology(kind){};

private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {3}));
        return nodeset_map;
    }
};

class CLX_2_var3_HMAT: public CLX_2_var3
{
public:
    CLX_2_var3_HMAT(memkind_t kind) : CLX_2_var3(kind){};

private:
    MapNodeSet Latency_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }
};

class CLX_2_var3_HBW: public AbstractTopology
{
public:
    CLX_2_var3_HBW(memkind_t kind) : AbstractTopology(kind){};

protected:
    MapNodeSet HBW_nodes() const
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {3}));
        return nodeset_map;
    }

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {3}));
        return nodeset_map;
    }
};

class CLX_2_var4_HBW: public AbstractTopology
{
public:
    CLX_2_var4_HBW(memkind_t kind) : AbstractTopology(kind){};

protected:
    MapNodeSet HBW_nodes() const
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }

    MapNodeSet HBW_all_nodes() const
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0, 2}));
        nodeset_map.emplace(NodeSet(1, {1, 3}));
        return nodeset_map;
    }

private:
    MapNodeSet Capacity_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        return nodeset_map;
    }

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {2}));
        nodeset_map.emplace(NodeSet(1, {3}));
        return nodeset_map;
    }
};

class CLX_4_var1: public AbstractTopology
{
public:
    CLX_4_var1(memkind_t kind) : AbstractTopology(kind){};

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

class CLX_4_var1_HMAT: public CLX_4_var1
{
public:
    CLX_4_var1_HMAT(memkind_t kind) : CLX_4_var1(kind){};

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var1_HBW: public AbstractTopology
{
public:
    CLX_4_var1_HBW(memkind_t kind) : AbstractTopology(kind){};

protected:
    MapNodeSet HBW_nodes() const
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {5}));
        nodeset_map.emplace(NodeSet(2, {6}));
        nodeset_map.emplace(NodeSet(3, {7}));
        return nodeset_map;
    }

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {5}));
        nodeset_map.emplace(NodeSet(2, {6}));
        nodeset_map.emplace(NodeSet(3, {7}));
        return nodeset_map;
    }
};

class CLX_4_var2: public AbstractTopology
{
public:
    CLX_4_var2(memkind_t kind) : AbstractTopology(kind){};

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

class CLX_4_var2_HMAT: public CLX_4_var2
{
public:
    CLX_4_var2_HMAT(memkind_t kind) : CLX_4_var2(kind){};

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var2_HBW: public AbstractTopology
{
public:
    CLX_4_var2_HBW(memkind_t kind) : AbstractTopology(kind){};

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {4}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var3: public AbstractTopology
{
public:
    CLX_4_var3(memkind_t kind) : AbstractTopology(kind){};

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

class CLX_4_var3_HMAT: public CLX_4_var3
{
public:
    CLX_4_var3_HMAT(memkind_t kind) : CLX_4_var3(kind){};

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var3_HBW: public AbstractTopology
{
public:
    CLX_4_var3_HBW(memkind_t kind) : AbstractTopology(kind){};

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {5}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {6}));
        return nodeset_map;
    }
};

class CLX_4_var4: public AbstractTopology
{
public:
    CLX_4_var4(memkind_t kind) : AbstractTopology(kind){};

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

class CLX_4_var4_HMAT: public CLX_4_var4
{
public:
    CLX_4_var4_HMAT(memkind_t kind) : CLX_4_var4(kind){};

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {0}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {3}));
        return nodeset_map;
    }
};

class CLX_4_var4_HBW: public AbstractTopology
{
public:
    CLX_4_var4_HBW(memkind_t kind) : AbstractTopology(kind){};

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

    MapNodeSet Bandwidth_local_nodes() const final
    {
        MapNodeSet nodeset_map;
        nodeset_map.emplace(NodeSet(0, {4}));
        nodeset_map.emplace(NodeSet(1, {1}));
        nodeset_map.emplace(NodeSet(2, {2}));
        nodeset_map.emplace(NodeSet(3, {5}));
        return nodeset_map;
    }
};

using TpgPtr = std::unique_ptr<AbstractTopology>;

TpgPtr TopologyFactory(std::string tpg_name, memkind_t kind)
{
    if (tpg_name.compare("KnightsMillAll2All") == 0)
        return TpgPtr(new KNM_All2All(kind));
    else if (tpg_name.compare("KnightsMillSNC2") == 0)
        return TpgPtr(new KNM_SNC2(kind));
    else if (tpg_name.compare("KnightsMillSNC4") == 0)
        return TpgPtr(new KNM_SNC4(kind));
    else if (tpg_name.compare("CascadeLake2Var1") == 0)
        return TpgPtr(new CLX_2_var1(kind));
    else if (tpg_name.compare("CascadeLake2Var1HMAT") == 0)
        return TpgPtr(new CLX_2_var1_HMAT(kind));
    else if (tpg_name.compare("CascadeLake2Var1HBW") == 0)
        return TpgPtr(new CLX_2_var1_HBW(kind));
    else if (tpg_name.compare("CascadeLake2Var2") == 0)
        return TpgPtr(new CLX_2_var2(kind));
    else if (tpg_name.compare("CascadeLake2Var2HMAT") == 0)
        return TpgPtr(new CLX_2_var2_HMAT(kind));
    else if (tpg_name.compare("CascadeLake2Var2HBW") == 0)
        return TpgPtr(new CLX_2_var2_HBW(kind));
    else if (tpg_name.compare("CascadeLake2Var3") == 0)
        return TpgPtr(new CLX_2_var3(kind));
    else if (tpg_name.compare("CascadeLake2Var3HMAT") == 0)
        return TpgPtr(new CLX_2_var3_HMAT(kind));
    else if (tpg_name.compare("CascadeLake2Var3HBW") == 0)
        return TpgPtr(new CLX_2_var3_HBW(kind));
    else if (tpg_name.compare("CascadeLake2Var4HBW") == 0)
        return TpgPtr(new CLX_2_var4_HBW(kind));
    else if (tpg_name.compare("CascadeLake4Var1") == 0)
        return TpgPtr(new CLX_4_var1(kind));
    else if (tpg_name.compare("CascadeLake4Var1HMAT") == 0)
        return TpgPtr(new CLX_4_var1_HMAT(kind));
    else if (tpg_name.compare("CascadeLake4Var1HBW") == 0)
        return TpgPtr(new CLX_4_var1_HBW(kind));
    else if (tpg_name.compare("CascadeLake4Var2") == 0)
        return TpgPtr(new CLX_4_var2(kind));
    else if (tpg_name.compare("CascadeLake4Var2HMAT") == 0)
        return TpgPtr(new CLX_4_var2_HMAT(kind));
    else if (tpg_name.compare("CascadeLake4Var2HBW") == 0)
        return TpgPtr(new CLX_4_var2_HBW(kind));
    else if (tpg_name.compare("CascadeLake4Var3") == 0)
        return TpgPtr(new CLX_4_var3(kind));
    else if (tpg_name.compare("CascadeLake4Var3HMAT") == 0)
        return TpgPtr(new CLX_4_var3_HMAT(kind));
    else if (tpg_name.compare("CascadeLake4Var3HBW") == 0)
        return TpgPtr(new CLX_4_var3_HBW(kind));
    else if (tpg_name.compare("CascadeLake4Var4") == 0)
        return TpgPtr(new CLX_4_var4(kind));
    else if (tpg_name.compare("CascadeLake4Var4HMAT") == 0)
        return TpgPtr(new CLX_4_var4_HMAT(kind));
    else if (tpg_name.compare("CascadeLake4Var4HBW") == 0)
        return TpgPtr(new CLX_4_var4_HBW(kind));
    else
        throw std::runtime_error("Unknown topology");
}
