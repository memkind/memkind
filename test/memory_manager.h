// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2020 Intel Corporation. */

#pragma once
#include <memkind.h>
#include <new>

class MemoryManager
{
private:
    memkind_t kind;
    size_t memory_size;
    void *memory_pointer;

    void move(MemoryManager &&other)
    {
        kind = other.kind;
        memory_size = other.memory_size;
        if (memory_pointer)
            memkind_free(kind, memory_pointer);
        memory_pointer = std::move(other.memory_pointer);
        other.memory_pointer = nullptr;
    }

public:
    MemoryManager(memkind_t kind, size_t size)
        : kind(kind),
          memory_size(size),
          memory_pointer(memkind_malloc(kind, size))
    {
        if (!memory_pointer) {
            throw std::bad_alloc();
        }
    }

    size_t size()
    {
        return memory_size;
    }

    MemoryManager(const MemoryManager &) = delete;

    MemoryManager(MemoryManager &&other)
    {
        move(std::move(other));
    }

    MemoryManager &operator=(MemoryManager &&other)
    {
        move(std::move(other));
        return *this;
    }

    ~MemoryManager()
    {
        if (memory_pointer)
            memkind_free(kind, memory_pointer);
    }
};
