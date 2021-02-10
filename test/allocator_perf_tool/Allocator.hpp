// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2021 Intel Corporation. */
#pragma once

#include "Allocation_info.hpp"
#include <stdio.h>

class Allocator
{
public:
    virtual memory_operation wrapped_malloc(size_t size) = 0;
    virtual memory_operation wrapped_calloc(size_t num, size_t size) = 0;
    virtual memory_operation wrapped_realloc(void *ptr, size_t size) = 0;
    virtual void wrapped_free(void *ptr) = 0;
    virtual unsigned type() = 0;

    virtual ~Allocator(void)
    {}
};
