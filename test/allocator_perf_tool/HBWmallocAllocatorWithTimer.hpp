// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2018 Intel Corporation. */

#pragma once

#include <hbwmalloc.h>

#include "Allocator.hpp"
#include "Allocation_info.hpp"
#include "Configuration.hpp"
#include "WrappersMacros.hpp"
#include "FunctionCalls.hpp"
#include <cerrno>

#include <stdlib.h>


class HBWmallocAllocatorWithTimer
    : public Allocator
{
public:
    memory_operation wrapped_malloc(size_t size)
    {
        START_TEST(AllocatorTypes::HBWMALLOC_ALLOCATOR, FunctionCalls::MALLOC)
        data.ptr = hbw_malloc(size);
        END_TEST
    }

    memory_operation wrapped_calloc(size_t num, size_t size)
    {
        START_TEST(AllocatorTypes::HBWMALLOC_ALLOCATOR, FunctionCalls::CALLOC)
        data.ptr = hbw_calloc(num, size);
        END_TEST
    }

    memory_operation wrapped_realloc(void *ptr, size_t size)
    {
        START_TEST(AllocatorTypes::HBWMALLOC_ALLOCATOR, FunctionCalls::REALLOC)
        data.ptr = hbw_realloc(ptr, size);
        END_TEST
    }

    void wrapped_free(void *ptr)
    {
        hbw_free(ptr);
    }

    unsigned type()
    {
        return AllocatorTypes::HBWMALLOC_ALLOCATOR;
    }

};
