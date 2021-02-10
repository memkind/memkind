// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once
#include "Configuration.hpp"
#include "FunctionCalls.hpp"
#include "Stats.hpp"

#include <stdio.h>

class ConsoleLog
{
public:
    static void print_stats(TimeStats &stats, unsigned allocator_type,
                            unsigned func_calls)
    {
        if (stats.stats.count(allocator_type)) {
            if (stats.stats[allocator_type].count(func_calls)) {
                MethodStats method_stats =
                    stats.stats[allocator_type][func_calls];
                printf(
                    " %20s (%u) | %7s | %10f.s | %10f.s  | %zu bytes/%f MB \n",
                    AllocatorTypes::allocator_name(allocator_type).c_str(),
                    allocator_type,
                    FunctionCalls::function_name(func_calls).c_str(),
                    method_stats.total_time, method_stats.average_time,
                    method_stats.allocation_size,
                    convert_bytes_to_mb(method_stats.allocation_size));
            }
        }
    }

    static void print_table(TimeStats &stats)
    {
        printf(
            "\n====== Allocators function calls performance =================================================\n");
        printf(
            " %20s Id:   Method:    Total time:    Average time:  Allocated memory bytes/MB: \n",
            "Allocator:");
        for (unsigned i = 0; i <= AllocatorTypes::MEMKIND_HBW_PREFERRED; i++) {
            for (unsigned func_call = FunctionCalls::FREE + 1;
                 func_call < FunctionCalls::NUM_OF_FUNCTIONS; func_call++) {
                print_stats(stats, i, func_call);
            }
        }
        printf(
            "==============================================================================================\n");
    }

    static void print_requested_memory(TimeStats &stats, std::string test_name)
    {
        printf("\n====== Requested memory stats for %s =================\n",
               test_name.c_str());
        printf("Total requested allocations: %zu bytes/%f MB. \n",
               stats.get_allocated(),
               convert_bytes_to_mb(stats.get_allocated()));
        printf("Total requested deallocations: %zu bytes/%f MB. \n",
               stats.get_deallocated(),
               convert_bytes_to_mb(stats.get_deallocated()));
        printf(
            "=====================================================================\n");
    }
};
