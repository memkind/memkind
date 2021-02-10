// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <memkind.h>

#include <cstdlib>
#include <new>

template <class deriving_class>
class memkind_allocated
{
public:
    static memkind_t getClassKind()
    {
        return MEMKIND_DEFAULT;
    }

    void *operator new(std::size_t size)
    {
        return deriving_class::operator new(size,
                                            deriving_class::getClassKind());
    }

    void *operator new[](std::size_t size)
    {
        return deriving_class::operator new(size,
                                            deriving_class::getClassKind());
    }

    void *operator new(std::size_t size, memkind_t memory_kind)
    {
        void *result_ptr = NULL;
        int allocation_result = 0;

        // This check if deriving_class has specified alignment, which is
        // suitable to be used with posix_memalign()
        if (alignof(deriving_class) < sizeof(void *)) {
            result_ptr = memkind_malloc(memory_kind, size);
            allocation_result = result_ptr ? 1 : 0;
        } else {
            allocation_result = memkind_posix_memalign(
                memory_kind, &result_ptr, alignof(deriving_class), size);
        }

        if (allocation_result) {
            throw std::bad_alloc();
        }

        return result_ptr;
    }

    void *operator new[](std::size_t size, memkind_t memory_kind)
    {
        return deriving_class::operator new(size, memory_kind);
    }

    void operator delete(void *ptr, memkind_t memory_kind)
    {
        memkind_free(memory_kind, ptr);
    }

    void operator delete(void *ptr)
    {
        memkind_free(0, ptr);
    }

    void operator delete[](void *ptr)
    {
        deriving_class::operator delete(ptr);
    }

    void operator delete[](void *ptr, memkind_t memory_kind)
    {
        deriving_class::operator delete(ptr, memory_kind);
    }

protected:
    memkind_allocated()
    {}

    ~memkind_allocated()
    {}
};
