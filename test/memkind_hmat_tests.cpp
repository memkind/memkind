// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <memkind.h>
#include <numa.h>
#include <numaif.h>
#include <string>
#include <unistd.h>
#include <math.h>

#include "proc_stat.h"
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "TestPolicy.hpp"

#include "common.h"

typedef std::map<std::string, std::vector<int>> TopologyNumaMap;

class MemkindHMATFunctionalTests: 
    public ::testing::Test,
    public ::testing::WithParamInterface<std::string>
{
protected:
    TopologyNumaMap correctNumaPerTopology;
    std::string param;

    void SetUp()
    {
        // NOTE: this is hardcoded and should match topoligies in utils/qemu/topology
        correctNumaPerTopology = {
            { "test", {3} },
            { "test2", {3, 5} },
            { "jenkins", {4, 5, 6} },
        };
        
        param = GetParam();

        const char* envp = std::getenv("MEMKIND_TEST_TOPOLOGY");
        if (envp == nullptr)
            GTEST_SKIP();

        if (strcmp(envp, param.c_str()) != 0)
            GTEST_SKIP() << param;
    }

    void TearDown() {}
};

// NOTE: this is hardcoded and should match topoligies in utils/qemu/topology
INSTANTIATE_TEST_CASE_P(
    TopologyParam, MemkindHMATFunctionalTests,
    ::testing::Values("test", "test2", "jenkins"));

TEST_P(MemkindHMATFunctionalTests,
       test_TC_HMAT_HC_alloc_size_max)
{
    errno = 0;
    void *test1 = memkind_malloc(MEMKIND_HIGHEST_CAPACITY, SIZE_MAX);
    ASSERT_EQ(test1, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_P(MemkindHMATFunctionalTests, test_TC_HMAT_HC_correct_numa)
{
    memkind_t kind = MEMKIND_HIGHEST_CAPACITY;
    int size = 10;
    void *ptr = memkind_malloc(kind, size);
    ASSERT_NE(ptr, nullptr);
    memset(ptr, 0, 1);

    // get ID of NUMA node where the allocation is made
    int numa_id = -1;
    get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);

    // get list of expected NUMA nodes from test definitions
    // and check if current location is made on one of exepected NUMA nodes
    std::vector<int> correct_numas = correctNumaPerTopology[param];
    auto it = std::find(std::begin(correct_numas), std::end(correct_numas), numa_id);
    ASSERT_NE(it, std::end(correct_numas));

    memkind_free(MEMKIND_HIGHEST_CAPACITY, ptr);
}

TEST_P(MemkindHMATFunctionalTests, test_TC_HMAT_HC_alloc_until_full_numa)
{
    memkind_t kind = MEMKIND_HIGHEST_CAPACITY;

    ProcStat stat;
    void *ptr;
    const size_t alloc_size = 100 * MB;
    std::set<void *> allocations;
   
    std::vector<int> correct_numas = correctNumaPerTopology[param];
    size_t sum_of_free_space = 0;

    for (size_t i = 0; i < correct_numas.size(); i++) {
        int numa_id = correct_numas[i];
        long long numa_free = -1;

        numa_node_size64(numa_id, &numa_free);   

        // TODO DEBUG     
        printf("numa: %i, free: %lld\n", numa_id, numa_free);
        fflush(stdout);

        sum_of_free_space += numa_free;
    }
    
    int numa_id = -1;
    const int n_swap_alloc = 20;

    while (sum_of_free_space > alloc_size * allocations.size()) {
        ptr = memkind_malloc(kind, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);

        get_mempolicy(&numa_id, nullptr, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
        auto it = std::find(std::begin(correct_numas), std::end(correct_numas), numa_id);
        ASSERT_NE(it, std::end(correct_numas));
        
        // TODO DEBUG
        long long numa_free = -1;
        numa_node_size64(numa_id, &numa_free);
        printf("numa: %i, free: %lld\n", numa_id, numa_free);
        fflush(stdout);
    }


    size_t init_swap = stat.get_used_swap_space_size_bytes();
    for(int i = 0; i < n_swap_alloc; ++i) {
        ptr = memkind_malloc(kind, alloc_size);
        ASSERT_NE(nullptr, ptr);
        memset(ptr, 'a', alloc_size);
        allocations.insert(ptr);
        
        // TODO DEBUG
        printf("do alloc on swap\n");
        fflush(stdout);
    }

    ASSERT_GE(stat.get_used_swap_space_size_bytes(), init_swap);

    // TODO DEBUG
    printf("free\n");
    fflush(stdout);
    
    for (auto const &ptr: allocations) {
        memkind_free(kind, ptr);
    }
}
