// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <ctime>
#include <sys/time.h>

#include <chrono>
#define START_TEST(ALLOCATOR, METHOD)                                          \
    memory_operation data;                                                     \
    data.allocator_type = ALLOCATOR;                                           \
    data.allocation_method = METHOD;                                           \
    data.error_code = 0;                                                       \
    std::chrono::system_clock::time_point last =                               \
        std::chrono::high_resolution_clock::now();
#define END_TEST                                                               \
    std::chrono::system_clock::time_point now =                                \
        std::chrono::high_resolution_clock::now();                             \
    std::chrono::duration<double, std::milli> elapsedTime(now - last);         \
    data.total_time = elapsedTime.count() / 1000.0;                            \
    data.size_of_allocation = size;                                            \
    data.error_code = errno;                                                   \
    data.is_allocated = true;                                                  \
    return data;
