// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include "memkind.h"

#include "Allocator.hpp"
#include "Configuration.hpp"
#include "HBWmallocAllocatorWithTimer.hpp"
#include "JemallocAllocatorWithTimer.hpp"
#include "MemkindAllocatorWithTimer.hpp"
#include "Numastat.hpp"
#include "PmemMockup.hpp"
#include "StandardAllocatorWithTimer.hpp"
#include "VectorIterator.hpp"

#include <assert.h>
#include <map>
#include <string.h>
#include <vector>

class AllocatorFactory
{
public:
    // Allocator initialization statistics
    struct initialization_stat {
        float total_time;     // Total time of initialization.
        float ref_delta_time; // Delta Time.
        unsigned allocator_type;
        std::vector<float> memory_overhead; // Memory overhead per numa node.
    };

    AllocatorFactory()
    {
        memkind_allocators[AllocatorTypes::MEMKIND_DEFAULT] =
            MemkindAllocatorWithTimer(MEMKIND_DEFAULT,
                                      AllocatorTypes::MEMKIND_DEFAULT);
        memkind_allocators[AllocatorTypes::MEMKIND_REGULAR] =
            MemkindAllocatorWithTimer(MEMKIND_REGULAR,
                                      AllocatorTypes::MEMKIND_REGULAR);
        memkind_allocators[AllocatorTypes::MEMKIND_HBW] =
            MemkindAllocatorWithTimer(MEMKIND_HBW, AllocatorTypes::MEMKIND_HBW);
        memkind_allocators[AllocatorTypes::MEMKIND_INTERLEAVE] =
            MemkindAllocatorWithTimer(MEMKIND_INTERLEAVE,
                                      AllocatorTypes::MEMKIND_INTERLEAVE);
        memkind_allocators[AllocatorTypes::MEMKIND_HBW_INTERLEAVE] =
            MemkindAllocatorWithTimer(MEMKIND_HBW_INTERLEAVE,
                                      AllocatorTypes::MEMKIND_HBW_INTERLEAVE);
        memkind_allocators[AllocatorTypes::MEMKIND_HBW_PREFERRED] =
            MemkindAllocatorWithTimer(MEMKIND_HBW_PREFERRED,
                                      AllocatorTypes::MEMKIND_HBW_PREFERRED);
        memkind_allocators[AllocatorTypes::MEMKIND_HUGETLB] =
            MemkindAllocatorWithTimer(MEMKIND_HUGETLB,
                                      AllocatorTypes::MEMKIND_HUGETLB);
        memkind_allocators[AllocatorTypes::MEMKIND_GBTLB] =
            MemkindAllocatorWithTimer(MEMKIND_GBTLB,
                                      AllocatorTypes::MEMKIND_GBTLB);
        memkind_allocators[AllocatorTypes::MEMKIND_HBW_HUGETLB] =
            MemkindAllocatorWithTimer(MEMKIND_HBW_HUGETLB,
                                      AllocatorTypes::MEMKIND_HBW_HUGETLB);
        memkind_allocators[AllocatorTypes::MEMKIND_HBW_PREFERRED_HUGETLB] =
            MemkindAllocatorWithTimer(
                MEMKIND_HBW_PREFERRED_HUGETLB,
                AllocatorTypes::MEMKIND_HBW_PREFERRED_HUGETLB);
        memkind_allocators[AllocatorTypes::MEMKIND_HBW_GBTLB] =
            MemkindAllocatorWithTimer(MEMKIND_HBW_GBTLB,
                                      AllocatorTypes::MEMKIND_HBW_GBTLB);
        memkind_allocators[AllocatorTypes::MEMKIND_HBW_PREFERRED_GBTLB] =
            MemkindAllocatorWithTimer(
                MEMKIND_HBW_PREFERRED_GBTLB,
                AllocatorTypes::MEMKIND_HBW_PREFERRED_GBTLB);
        memkind_allocators[AllocatorTypes::MEMKIND_PMEM] =
            MemkindAllocatorWithTimer(MEMKIND_PMEM_MOCKUP,
                                      AllocatorTypes::MEMKIND_PMEM);
        memkind_allocators[AllocatorTypes::MEMKIND_DAX_KMEM] =
            MemkindAllocatorWithTimer(MEMKIND_DAX_KMEM,
                                      AllocatorTypes::MEMKIND_DAX_KMEM);
    }

    // Get existing allocator without creating new.
    // The owner of existing allocator is AllocatorFactory object.
    Allocator *get_existing(unsigned type)
    {
        switch (type) {
            case AllocatorTypes::STANDARD_ALLOCATOR:
                return &standard_allocator;

            case AllocatorTypes::JEMALLOC:
                return &jemalloc;

            case AllocatorTypes::HBWMALLOC_ALLOCATOR:
                return &hbwmalloc_allocator;

            default: {
                if (memkind_allocators.count(type))
                    return &memkind_allocators[type];

                assert(!"'type' out of range!");
            }
        }
    }

    initialization_stat initialize_allocator(Allocator &allocator)
    {
        size_t initial_size = 512;
        float before_node1 = Numastat::get_total_memory(0);
        float before_node2 = Numastat::get_total_memory(1);
        initialization_stat stat = {0};

        // malloc
        memory_operation malloc_data = allocator.wrapped_malloc(initial_size);
        stat.total_time += malloc_data.total_time;

        // realloc
        memory_operation realloc_data =
            allocator.wrapped_realloc(malloc_data.ptr, 256);
        allocator.wrapped_free(realloc_data.ptr);

        stat.total_time += realloc_data.total_time;

        // calloc
        memory_operation calloc_data =
            allocator.wrapped_calloc(initial_size, 1);
        allocator.wrapped_free(calloc_data.ptr);

        stat.total_time += calloc_data.total_time;

        stat.allocator_type = allocator.type();

        // calc memory overhead
        stat.memory_overhead.push_back(Numastat::get_total_memory(0) -
                                       before_node1);
        stat.memory_overhead.push_back(Numastat::get_total_memory(1) -
                                       before_node2);

        return stat;
    }

    initialization_stat initialize_allocator(unsigned type)
    {
        return initialize_allocator(*get_existing(type));
    }

    // Calc percent delta between reference value and current value.
    float calc_ref_delta(float ref_value, float value)
    {
        return ((value / ref_value) - 1.0) * 100.0;
    }

    // Test initialization performance over available allocators.
    // Return statistics.
    std::vector<initialization_stat> initialization_test()
    {
        std::vector<initialization_stat> stats;
        initialization_stat stat;

        stat = initialize_allocator(standard_allocator);
        float ref_time = stat.total_time;
        stats.push_back(stat);

        // Loop over available allocators to call initializer and compute stats.
        for (unsigned i = 1; i < AllocatorTypes::NUM_OF_ALLOCATOR_TYPES; i++) {
            stat = initialize_allocator(*get_existing(i));
            stat.ref_delta_time = calc_ref_delta(ref_time, stat.total_time);
            stats.push_back(stat);
        }

        return stats;
    }

    VectorIterator<Allocator *>
    generate_random_allocator_calls(int num, int seed,
                                    TypesConf allocator_calls)
    {
        srand(seed);
        std::vector<Allocator *> allocators_calls;

        for (int i = 0; i < num; i++) {
            int index;

            do {
                index = (rand() % (AllocatorTypes::NUM_OF_ALLOCATOR_TYPES));
            } while (!allocator_calls.is_enabled(index));

            allocators_calls.push_back(get_existing(index));
        }

        return VectorIterator<Allocator *>::create(allocators_calls);
    }

    // Return kind to the corresponding AllocatorTypes enum specified in
    // argument.
    memkind_t get_kind_by_type(unsigned type)
    {
        if (memkind_allocators.count(type))
            return memkind_allocators[type].get_kind();

        assert(!"'type' out of range!");
    }

private:
    StandardAllocatorWithTimer standard_allocator;
    JemallocAllocatorWithTimer jemalloc;
    HBWmallocAllocatorWithTimer hbwmalloc_allocator;

    std::map<unsigned, MemkindAllocatorWithTimer> memkind_allocators;
};
