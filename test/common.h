// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2021 Intel Corporation. */

#ifndef COMMON_H
#define COMMON_H

#include <hbwmalloc.h>
#include <memkind.h>

#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include "TestPrereq.hpp"

#define MB 1048576ULL
#define GB 1073741824ULL
#define KB 1024ULL

#define HBW_SUCCESS 0
#define HBW_ERROR -1

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

class Memkind_Test: public ::testing::Test
{
protected:
    TestPrereq prereq;
    memkind_t kind;
    void SetUp()
    {
        std::cout << "Testing: " << prereq.kind_name(kind) << std::endl;
    }
    void TearDown()
    {
        std::cout << "Finish testing: " << prereq.kind_name(kind) << std::endl;
    }
};

class Memkind_Param_Test: public ::testing::Test,
    public ::testing::WithParamInterface<memkind_t>
{
protected:
    TestPrereq prereq;
    memkind_t kind;
    void SetUp()
    {
        kind = GetParam();
        std::cout << "Testing: " << prereq.kind_name(kind) << std::endl;
    }
    void TearDown()
    {
        std::cout << "Finish testing: " << prereq.kind_name(kind) << std::endl;
    }
};

#endif
