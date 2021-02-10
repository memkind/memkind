// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include "Iterator.hpp"

#include <vector>

template <class T>
class VectorIterator: public Iterator<T>
{
public:
    static VectorIterator create(int init_size)
    {
        return VectorIterator(init_size);
    }
    static VectorIterator create(std::vector<T> v)
    {
        return VectorIterator(v);
    }

    bool has_next() const
    {
        return !vec.empty();
    }

    T next()
    {
        T val = vec.back();
        vec.pop_back();
        return val;
    }

    size_t size() const
    {
        return vec.size();
    }

private:
    VectorIterator(std::vector<T> v)
    {
        vec = v;
    }

    std::vector<T> vec;
};
