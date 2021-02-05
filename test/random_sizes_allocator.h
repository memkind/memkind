// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2020 Intel Corporation. */

#pragma once
#include "memory_manager.h"
#include <random>
#include <vector>

class RandomSizesAllocator
{
private:
    std::vector<MemoryManager> allocated_memory;
    memkind_t kind;
    std::default_random_engine generator;
    std::uniform_int_distribution<int> memory_distribution;

    size_t get_random_size()
    {
        return memory_distribution(generator);
    }

public:
    RandomSizesAllocator(memkind_t kind, size_t min_size, size_t max_size,
                         int max_allocations_number)
        : kind(kind), memory_distribution(min_size, max_size)
    {
        allocated_memory.reserve(max_allocations_number);
    }

    size_t malloc_random_memory()
    {
        size_t size = get_random_size();
        allocated_memory.emplace_back(kind, size);
        return size;
    }

    size_t free_random_memory()
    {
        if (empty())
            return 0;
        std::uniform_int_distribution<int> distribution(
            0, allocated_memory.size() - 1);
        int random_index = distribution(generator);
        auto it = std::begin(allocated_memory) + random_index;
        size_t size = it->size();
        allocated_memory.erase(it);
        return size;
    }

    bool empty()
    {
        return allocated_memory.empty();
    }
};
