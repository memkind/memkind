// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */
#pragma once

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

/*
 * The GTestAdapter class is an adapter for GTest framework.
 * All methods from GTest framework that need to be adapted should be located
 * here.
 */

class GTestAdapter
{
public:
    template <class T>
    static void RecordProperty(const std::string &key, const T &value)
    {
        std::ostringstream tmp_value;
        tmp_value << value;
        testing::Test::RecordProperty(key, tmp_value.str());
        std::ios::fmtflags flags(std::cout.flags());
        std::cout << key << ": " << std::fixed << std::setprecision(5) << value
                  << std::endl;
        std::cout.flags(flags);
    }
};
