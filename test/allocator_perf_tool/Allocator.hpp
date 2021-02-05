// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <stdio.h>

#include "Allocation_info.hpp"

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
