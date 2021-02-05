// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include "Configuration.hpp"
#include "VectorIterator.hpp"
#include <cstdlib>
#include <vector>

class FunctionCalls
{
public:
    enum
    {
        FREE,
        MALLOC,
        CALLOC,
        REALLOC,
        NUM_OF_FUNCTIONS
    };

    static const std::string function_name(unsigned type)
    {
        static std::string names[] = {"free", "malloc", "calloc", "realloc"};

        if (type >= NUM_OF_FUNCTIONS)
            assert(!"Invalidate input argument!");

        return names[type];
    }

    static unsigned function_type(const std::string &name)
    {
        for (unsigned i = 0; i < NUM_OF_FUNCTIONS; i++) {
            if (function_name(i) == name)
                return i;
        }

        assert(!"Invalid input argument!");
    }

    static VectorIterator<int>
    generate_random_allocator_func_calls(int call_num, int seed,
                                         TypesConf func_calls)
    {
        std::vector<unsigned> avail_types;

        std::srand(seed);
        std::vector<int> calls;

        for (int i = 0; i < call_num; i++) {
            int index;
            do {
                index = rand() % (NUM_OF_FUNCTIONS);
            } while (!func_calls.is_enabled(index));

            calls.push_back(index);
        }

        return VectorIterator<int>::create(calls);
    }
};
