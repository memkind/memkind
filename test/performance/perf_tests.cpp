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

#include "perf_tests.hpp"
#include <iostream>
#include <gtest/gtest.h>

// Memkind performance tests
using std::cout;
using std::endl;

// Memkind tests
class PerformanceTest : public testing::Test
{
protected:
    const double Delta = 0.15;

    Metrics referenceMetrics;
    Metrics performanceMetrics;
    PerfTestCase<performance_tests::MallocOperation> referenceTest;
    PerfTestCase<performance_tests::MemkindOperation> performanceTest;

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    bool checkDelta(double value, double reference, const string &info, double delta, bool notLessThan = false)
    {
        // If notLessThan is true, current value should be >= (reference - delta); otherwise, it should be <= (reference + delta)
        double treshold;
        if (notLessThan)
        {
            treshold = reference / (1 + Delta);
        }
        else
        {
            treshold = reference * (1 + Delta);
        }
        cout << "Value of '" << info << "' expected to be at " << (notLessThan ? "least '" : "most '")
             << treshold << "'. Actual='" << value << "', reference='"
             << reference << "', delta='" << delta << "'." << endl;
        if (notLessThan ? (value >= treshold) : (value <= treshold))
        {
            return true;
        }
        else
        {
            cout << "ERROR: Value of '" << info << "' outside expected bounds!" << endl;
            return false;
        }
    }

    bool compareMetrics(Metrics metrics, Metrics reference, double delta)
    {
        return (
            checkDelta(metrics.operationsPerSecond, reference.operationsPerSecond, "operationsPerSecond", delta, true) &&
            checkDelta(metrics.avgOperationDuration, reference.avgOperationDuration, "avgOperationDuration", delta) &&
            checkDelta(metrics.iterationDuration, reference.iterationDuration, "iterationDuration", delta) &&
            checkDelta(metrics.repeatDuration, reference.repeatDuration, "repeatDuration", delta)
            );
    }
};

PERF_TEST(PerformanceTest, single_op_single_iter)
{
    referenceTest.setupTest_singleOpSingleIter();
    referenceMetrics = referenceTest.runTest();

    performanceTest.setupTest_singleOpSingleIter();
    performanceMetrics = performanceTest.runTest();
    EXPECT_TRUE(compareMetrics(performanceMetrics, referenceMetrics, Delta));
}

PERF_TEST(PerformanceTest, many_ops_single_iter)
{
    referenceTest.setupTest_manyOpsSingleIter();
    referenceMetrics = referenceTest.runTest();

    performanceTest.setupTest_manyOpsSingleIter();
    performanceMetrics = performanceTest.runTest();
    EXPECT_TRUE(compareMetrics(performanceMetrics, referenceMetrics, Delta));
}

PERF_TEST(PerformanceTest, many_ops_single_iter_huge_alloc)
{
    referenceTest.setupTest_manyOpsSingleIterHugeAlloc();
    referenceMetrics = referenceTest.runTest();

    performanceTest.setupTest_manyOpsSingleIterHugeAlloc();
    performanceMetrics = performanceTest.runTest();
    EXPECT_TRUE(compareMetrics(performanceMetrics, referenceMetrics, Delta));
}

PERF_TEST(PerformanceTest, single_op_many_iters)
{
    referenceTest.setupTest_singleOpManyIters();
    referenceMetrics = referenceTest.runTest();

    performanceTest.setupTest_singleOpManyIters();
    performanceMetrics = performanceTest.runTest();
    EXPECT_TRUE(compareMetrics(performanceMetrics, referenceMetrics, Delta));
}

PERF_TEST(PerformanceTest, many_ops_many_iters)
{
    referenceTest.setupTest_manyOpsManyIters();
    referenceMetrics = referenceTest.runTest();

    performanceTest.setupTest_manyOpsManyIters();
    performanceMetrics = performanceTest.runTest();
    EXPECT_TRUE(compareMetrics(performanceMetrics, referenceMetrics, Delta));
}

TEST_F(PerformanceTest, many_ops_many_iters_many_kinds)
{
    referenceTest.setupTest_manyOpsManyIters();
    referenceMetrics = referenceTest.runTest({ MEMKIND_DEFAULT, MEMKIND_HBW_PREFERRED });

    performanceTest.setupTest_manyOpsManyIters();
    performanceMetrics = performanceTest.runTest({ MEMKIND_DEFAULT, MEMKIND_HBW_PREFERRED });
    EXPECT_TRUE(compareMetrics(performanceMetrics, referenceMetrics, Delta));
}
