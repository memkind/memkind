// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */

#pragma once

#include <cstdlib>
#include <vector>

#include "VectorIterator.hpp"

class AllocationSizes
{
public:
    static VectorIterator<size_t>
    generate_random_sizes(int sizes_num, size_t from, size_t to, int seed)
    {
        srand(seed);
        std::vector<size_t> sizes;
        size_t range = to - from;

        if (from == to) {
            for (int i = 0; i < sizes_num; i++)
                sizes.push_back(from);
        } else {
            for (int i = 0; i < sizes_num; i++)
                sizes.push_back(rand() % (range - 1) + from);
        }

        return VectorIterator<size_t>::create(sizes);
    }

    static VectorIterator<size_t>
    generate_random_sizes(AllocationSizesConf conf, int seed)
    {
        return generate_random_sizes(conf.n, conf.size_from, conf.size_to,
                                     seed);
    }
};
