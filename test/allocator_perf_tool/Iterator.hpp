// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

template <class T>
class Iterator
{
public:
    virtual bool has_next() const = 0;
    virtual T next() = 0;
    virtual size_t size() const = 0;
};
