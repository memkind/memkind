// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>

namespace csv
{

class Row
{
public:
    Row()
    {
        row << std::fixed;
        row.precision(6);
    }

    template <class T>
    void append(const T &e)
    {
        row << "," << e;
    }

    std::string export_row() const
    {
        std::stringstream ss(row.str());
        ss << std::endl;
        return ss.str();
    }

private:
    std::stringstream row;
};

} // namespace csv
