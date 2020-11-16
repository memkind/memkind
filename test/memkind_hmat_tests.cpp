// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <memkind.h>

#include "common.h"

class MemkindHMATFunctionalTests: public ::testing::Test
{
protected:

    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindHMATFunctionalTests, test_TC_HMAT_example)
{
    if(const char *env_p = std::getenv("MEMKIND_TEST_TOPOLOGY")) {
        std::cout << "MEMKIND_TEST_TOPOLOGY is: " << env_p << '\n';
    } else {
        std::cout << "MEMKIND_TEST_TOPOLOGY was not presetnt:" << '\n';
    }
}
