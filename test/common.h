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

#define MB 1048576ULL
#define GB 1073741824ULL
#define KB 1024ULL

#define HBW_SUCCESS 0
#define HBW_ERROR -1

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static const char *kind_name(memkind_t kind)
{
    if (kind == MEMKIND_DEFAULT)
        return "MEMKIND_DEFAULT";
    else if (kind == MEMKIND_HUGETLB)
        return "MEMKIND_HUGETLB";
    else if (kind == MEMKIND_INTERLEAVE)
        return "MEMKIND_INTERLEAVE";
    else if (kind == MEMKIND_HBW)
        return "MEMKIND_HBW";
    else if (kind == MEMKIND_HBW_ALL)
        return "MEMKIND_HBW_ALL";
    else if (kind == MEMKIND_HBW_PREFERRED)
        return "MEMKIND_HBW_PREFERRED";
    else if (kind == MEMKIND_HBW_HUGETLB)
        return "MEMKIND_HBW_HUGETLB";
    else if (kind == MEMKIND_HBW_ALL_HUGETLB)
        return "MEMKIND_HBW_ALL_HUGETLB";
    else if (kind == MEMKIND_HBW_PREFERRED_HUGETLB)
        return "MEMKIND_HBW_PREFERRED_HUGETLB";
    else if (kind == MEMKIND_HBW_GBTLB)
        return "MEMKIND_HBW_GBTLB";
    else if (kind == MEMKIND_HBW_PREFERRED_GBTLB)
        return "MEMKIND_HBW_PREFERRED_GBTLB";
    else if (kind == MEMKIND_HBW_INTERLEAVE)
        return "MEMKIND_HBW_INTERLEAVE";
    else if (kind == MEMKIND_REGULAR)
        return "MEMKIND_REGULAR";
    else if (kind == MEMKIND_GBTLB)
        return "MEMKIND_GBTLB";
    else if (kind == MEMKIND_DAX_KMEM)
        return "MEMKIND_DAX_KMEM";
    else if (kind == MEMKIND_DAX_KMEM_ALL)
        return "MEMKIND_DAX_KMEM_ALL";
    else if (kind == MEMKIND_DAX_KMEM_PREFERRED)
        return "MEMKIND_DAX_KMEM_PREFERRED";
    else if (kind == MEMKIND_DAX_KMEM_INTERLEAVE)
        return "MEMKIND_DAX_KMEM_INTERLEAVE";
    else return "Unknown memory kind";
}

class Memkind_Param_Test: public ::testing::Test,
    public ::testing::WithParamInterface<memkind_t>
{
protected:
    memkind_t memory_kind;
    void SetUp()
    {
        memory_kind = GetParam();
        std::cout << "Testing memory kind: " << kind_name(memory_kind) << std::endl;
    }
    void TearDown()
    {
        std::cout << "Finished Testing memory kind: " << kind_name(
                      memory_kind) << std::endl;
    }
};

#endif
