// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <stddef.h>
#include <stdint.h>

// This structure is responsible to store information about single memory
// operation.
struct memory_operation {
    void *ptr;
    double total_time;
    size_t size_of_allocation;
    unsigned allocator_type;
    unsigned allocation_method;
    bool is_allocated;
    int error_code;
};

double convert_bytes_to_mb(uint64_t bytes);

int get_numa_node_id(void *ptr);
