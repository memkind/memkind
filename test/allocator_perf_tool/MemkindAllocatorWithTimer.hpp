// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */

#pragma once

#include <memkind.h>

#include "Allocation_info.hpp"
#include "Allocator.hpp"
#include "Configuration.hpp"
#include "FunctionCalls.hpp"
#include "WrappersMacros.hpp"
#include <cerrno>

#include <stdlib.h>

class MemkindAllocatorWithTimer: public Allocator
{
public:
    MemkindAllocatorWithTimer() : kind(&MEMKIND_DEFAULT)
    {}

    MemkindAllocatorWithTimer(memkind_t &memory_kind, unsigned kind_type_id)
    {
        kind = &memory_kind;
        type_id = kind_type_id;
    }
    ~MemkindAllocatorWithTimer(void)
    {}

    memory_operation wrapped_malloc(size_t size)
    {
        START_TEST(type_id, FunctionCalls::MALLOC)
        data.ptr = memkind_malloc(*kind, size);
        END_TEST
    }

    memory_operation wrapped_calloc(size_t num, size_t size)
    {
        START_TEST(type_id, FunctionCalls::CALLOC)
        data.ptr = memkind_calloc(*kind, num, size);
        END_TEST
    }

    memory_operation wrapped_realloc(void *ptr, size_t size)
    {
        START_TEST(type_id, FunctionCalls::REALLOC)
        data.ptr = memkind_realloc(*kind, ptr, size);
        END_TEST
    }

    void wrapped_free(void *ptr)
    {
        memkind_free(*kind, ptr);
    }

    void change_kind(memkind_t &memory_kind, unsigned kind_type_id)
    {
        kind = &memory_kind;
        type_id = kind_type_id;
    }

    unsigned type()
    {
        return type_id;
    }
    memkind_t get_kind()
    {
        return *kind;
    }

private:
    memkind_t *kind;
    unsigned type_id;
};
