// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#pragma once

#include "framework.hpp"
#include "operations.hpp"

using std::string;
using std::vector;

using performance_tests::ExecutionMode;
using performance_tests::Metrics;
using performance_tests::Operation;
using performance_tests::OperationName;

template <class T>
class PerfTestCase
{

private:
    vector<vector<Operation *>> m_testOperations;
    Operation *m_freeOperation;
    performance_tests::PerformanceTest *m_test;
    const unsigned m_seed = 1297654;
    const unsigned m_repeats = 50;
    const unsigned m_threads = 64;
    const unsigned m_iterations = 100;

public:
    PerfTestCase()
    {
        m_freeOperation = new T(OperationName::Free);
        srand(m_seed);
        m_test = nullptr;
    }

    ~PerfTestCase()
    {
        delete m_test;
        for (vector<Operation *> ops : m_testOperations) {
            for (Operation *op : ops) {
                delete op;
            }
        }
    }

    // Perform common actions for all test cases
    Metrics runTest(vector<memkind_t> kinds = {MEMKIND_DEFAULT})
    {
        m_test->setKind(kinds);
        m_test->showInfo();
        m_test->run();
        return m_test->getMetrics();
    }

    // malloc only
    // 128 bytes

    void setupTest_singleOpSingleIter()
    {

        m_testOperations = {{new T(OperationName::Malloc)}};

        m_test = new performance_tests::PerformanceTest(m_repeats, m_threads,
                                                        m_iterations * 10);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({128});
    }

    // malloc, calloc, realloc and align (equal probability)
    // 120, 521, 1200 and 4099 bytes
    void setupTest_manyOpsSingleIter()
    {
        m_testOperations = {{new T(OperationName::Malloc, 25),
                             new T(OperationName::Calloc, 50),
                             new T(OperationName::Realloc, 75),
                             new T(OperationName::Align, 100)}};

        m_test = new performance_tests::PerformanceTest(m_repeats, m_threads,
                                                        m_iterations * 10);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({120, 521, 1200, 4099});
    }

    // malloc, calloc, realloc and align (equal probability)
    // 500000, 1000000, 2000000 and 4000000 bytes
    void setupTest_manyOpsSingleIterHugeAlloc()
    {
        m_testOperations = {{new T(OperationName::Malloc, 25),
                             new T(OperationName::Calloc, 50),
                             new T(OperationName::Realloc, 75),
                             new T(OperationName::Align, 100)}};

        m_test = new performance_tests::PerformanceTest(m_repeats, m_threads,
                                                        m_iterations);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({500000, 1000000, 2000000, 4000000});
    }

    // 4 iterations of each thread (first malloc, then calloc, realloc and
    // align) 120, 521, 1200 and 4099 bytes
    void setupTest_singleOpManyIters()
    {
        m_testOperations = {{new T(OperationName::Malloc, 100)},
                            {new T(OperationName::Calloc, 100)},
                            {new T(OperationName::Realloc, 100)},
                            {new T(OperationName::Align, 100)}};

        m_test = new performance_tests::PerformanceTest(m_repeats, m_threads,
                                                        m_iterations * 10);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({120, 521, 1200, 4099});
        m_test->setExecutionMode(ExecutionMode::ManyIterations);
    }

    // 4 iterations of each thread, all with same set of operations (malloc,
    // then calloc, realloc and align), but different operation probability 120,
    // 521, 1200 and 4099 bytes
    void setupTest_manyOpsManyIters()
    {
        m_testOperations = {
            {new T(OperationName::Malloc, 25), new T(OperationName::Calloc, 50),
             new T(OperationName::Realloc, 75),
             new T(OperationName::Align, 100)},
            {new T(OperationName::Malloc, 50), new T(OperationName::Calloc, 70),
             new T(OperationName::Realloc, 80),
             new T(OperationName::Align, 100)},
            {new T(OperationName::Calloc, 50), new T(OperationName::Malloc, 60),
             new T(OperationName::Realloc, 75),
             new T(OperationName::Align, 100)},
            {new T(OperationName::Realloc, 60),
             new T(OperationName::Malloc, 80), new T(OperationName::Calloc, 90),
             new T(OperationName::Align, 100)},
            {new T(OperationName::Realloc, 40),
             new T(OperationName::Malloc, 55), new T(OperationName::Calloc, 70),
             new T(OperationName::Align, 100)}};

        m_test = new performance_tests::PerformanceTest(m_repeats, m_threads,
                                                        m_iterations * 10);
        m_test->setOperations(m_testOperations, m_freeOperation);
        m_test->setAllocationSizes({120, 521, 1200, 4099});
        m_test->setExecutionMode(ExecutionMode::ManyIterations);
    }
};
