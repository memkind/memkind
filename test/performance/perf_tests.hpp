/*
* Copyright (C) 2014, 2015 Intel Corporation.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright notice(s),
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice(s),
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
* EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef  __PERF_TESTS_HPP
#define  __PERF_TESTS_HPP

#include "framework.hpp"
#include "operations.hpp"
#include <gtest/gtest.h>

// produce name with expected prefix
#define PERF_TEST(LIB, NAME) \
TEST_F(LIB##PerfTest, TC_##LIB##_Performance_##NAME)

template <class T>
class PerfTest : public testing::Test
{
protected:
    vector<vector<Operation*>> m_testOperations;
    Operation *m_freeOperation;
    PerformanceTest *m_test;
    unsigned m_seed = 1297654;
    const::testing::TestInfo* const m_testInfo =
      ::testing::UnitTest::GetInstance()->current_test_info();

    void SetUp()
    {
        m_freeOperation = new T(OperationName::Free);
        m_test = nullptr;
    }

    void TearDown()
    {
        delete m_test;
        for (vector<Operation *> ops : m_testOperations)
        {
            for (Operation * op : ops)
            {
                delete op;
            }
        }
    }

    // Perform common actions for all test cases
    void runTest(vector<memkind_t> kinds = { MEMKIND_DEFAULT })
    {
        m_test->setKind(kinds);
        m_test->showInfo();
        m_test->run();
        m_test->writeMetrics(m_testInfo->test_case_name(), m_testInfo->name());
    }

    // 5 repeats, 8 threads, 1000000 operations/thread
    // malloc only
    // 128 bytes
    void singleOpSingleIterPerfTest()
    {

        m_testOperations = { { new T(OperationName::Malloc) } };

        srand(m_seed);
        m_test = new PerformanceTest(5, 8, 1000000);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({ 128 });
    }

    // 5 repeats, 8 threads, 1000000 operations/thread
    // malloc, calloc, realloc and align (equal probability)
    // 120, 521, 1200 and 4099 bytes
    void manyOpsSingleIterPerfTest()
    {
        m_testOperations = { {
                new T(OperationName::Malloc, 25),
                new T(OperationName::Calloc, 50),
                new T(OperationName::Realloc, 75),
                new T(OperationName::Align, 100)
            } };

        srand(m_seed);
        m_test = new PerformanceTest(5, 8, 1000000);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({ 120, 521, 1200, 4099 });
    }

    // 5 repeats, 8 threads, 10000 operations/thread
    // malloc, calloc, realloc and align (equal probability)
    // 1000000, 2000000, 4000000 and 8000000 bytes
    void manyOpsSingleIterHugeAllocPerfTest()
    {
        m_testOperations = { {
                new T(OperationName::Malloc, 25),
                new T(OperationName::Calloc, 50),
                new T(OperationName::Realloc, 75),
                new T(OperationName::Align, 100)
            }};

        srand(m_seed);
        m_test = new PerformanceTest(5, 8, 1000);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({ 1000000, 2000000, 4000000, 8000000 });
    }

    // 5 repeats, 8 threads, 1000000 operations/thread
    // 4 iterations of each thread (first malloc, then calloc, realloc and align)
    // 120, 521, 1200 and 4099 bytes
    void singleOpManyItersPerfTest()
    {
        m_testOperations = {
            { new T(OperationName::Malloc, 100) },
            { new T(OperationName::Calloc, 100) },
            { new T(OperationName::Realloc, 100) },
            { new T(OperationName::Align, 100) }
        };

        srand(m_seed);
        m_test = new PerformanceTest (5, 8, 1000000);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({ 120, 521, 1200, 4099 });
        m_test->setExecutionMode(ExecutionMode::ManyIterations);
    }

    // 5 repeats, 8 threads, 1000000 operations/thread
    // 4 iterations of each thread, all with same set of operations (malloc, then calloc, realloc and align),
    // but different operation probability
    // 120, 521, 1200 and 4099 bytes
    void manyOpsManyItersPerfTest()
    {
        m_testOperations = {
            {
                new T(OperationName::Malloc, 25),
                new T(OperationName::Calloc, 50),
                new T(OperationName::Realloc, 75),
                new T(OperationName::Align, 100)
            },
            {
                new T(OperationName::Malloc, 50),
                new T(OperationName::Calloc, 70),
                new T(OperationName::Realloc, 80),
                new T(OperationName::Align, 100)
            },
            {
                new T(OperationName::Calloc, 50),
                new T(OperationName::Malloc, 60),
                new T(OperationName::Realloc, 75),
                new T(OperationName::Align, 100)
            },
            {
                new T(OperationName::Realloc, 60),
                new T(OperationName::Malloc, 80),
                new T(OperationName::Calloc, 90),
                new T(OperationName::Align, 100)
            },
            {
                new T(OperationName::Realloc, 40),
                new T(OperationName::Malloc, 55),
                new T(OperationName::Calloc, 70),
                new T(OperationName::Align, 100)
            }
        };

        srand(m_seed);
        m_test = new PerformanceTest (5, 8, 10000);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({ 120, 521, 1200, 4099 });
        m_test->setExecutionMode(ExecutionMode::ManyIterations);
    }
};

#endif // __PERF_TESTS_HPP