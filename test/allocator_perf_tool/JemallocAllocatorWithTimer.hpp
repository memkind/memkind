// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2018 Intel Corporation. */
#pragma once

#include "Allocator.hpp"
#include "Allocation_info.hpp"
#include "Configuration.hpp"
#include "WrappersMacros.hpp"
#include "FunctionCalls.hpp"

#include <jemalloc/jemalloc.h>

class JemallocAllocatorWithTimer
    : public Allocator
{
public:

    memory_operation wrapped_malloc(size_t size)
    {
        START_TEST(AllocatorTypes::JEMALLOC, FunctionCalls::MALLOC)
        data.ptr = malloc(size);
        END_TEST
    }

    memory_operation wrapped_calloc(size_t num, size_t size)
    {
        START_TEST(AllocatorTypes::JEMALLOC, FunctionCalls::CALLOC)
        data.ptr = calloc(num, size);
        END_TEST
    }
    memory_operation wrapped_realloc(void *ptr, size_t size)
    {
        START_TEST(AllocatorTypes::JEMALLOC, FunctionCalls::REALLOC)
        data.ptr = realloc(ptr, size);
        END_TEST
    }

    void wrapped_free(void *ptr)
    {
        free(ptr);
    }

    unsigned type()
    {
        return AllocatorTypes::JEMALLOC;
    }
};
