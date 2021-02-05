// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include "allocator_perf_tool/GTestAdapter.hpp"
#include "allocator_perf_tool/PmemMockup.hpp"
#include "allocator_perf_tool/Stats.hpp"
#include "allocator_perf_tool/TaskFactory.hpp"
#include "allocator_perf_tool/Thread.hpp"
#include "common.h"

static const size_t PMEM_PART_SIZE = 0;
extern const char *PMEM_DIR;

class PmemAllocPerformanceTest: public ::testing::Test
{
private:
    AllocatorFactory allocator_factory;

protected:
    void SetUp()
    {
        allocator_factory.initialize_allocator(
            AllocatorTypes::STANDARD_ALLOCATOR);

        int err =
            memkind_create_pmem(PMEM_DIR, PMEM_PART_SIZE, &MEMKIND_PMEM_MOCKUP);
        ASSERT_EQ(0, err);
        ASSERT_NE(nullptr, MEMKIND_PMEM_MOCKUP);
    }

    void TearDown()
    {
        int err = memkind_destroy_kind(MEMKIND_PMEM_MOCKUP);
        ASSERT_EQ(0, err);
    }

    float run(unsigned kind, unsigned call, size_t threads_number,
              size_t alloc_size, unsigned mem_operations_num)
    {
        TaskFactory task_factory;
        std::vector<Thread *> threads;
        std::vector<Task *> tasks;
        TypesConf func_calls;
        TypesConf allocator_types;

        func_calls.enable_type(FunctionCalls::FREE);
        func_calls.enable_type(call);
        allocator_types.enable_type(kind);

        TaskConf conf = {
            mem_operations_num, // number of memory operations
            {
                mem_operations_num, // number of memory operations
                alloc_size,         // min. size of single allocation
                alloc_size          // max. size of single allocatioion
            },
            func_calls,      // enable function calls
            allocator_types, // enable allocators
            11,              // random seed
            false, // do not log memory operations and statistics to csv file
        };

        for (int i = 0; i < threads_number; i++) {
            Task *task = task_factory.create(conf);
            tasks.push_back(task);
            threads.push_back(new Thread(task));
            conf.seed += 1;
        }

        ThreadsManager threads_manager(threads);
        threads_manager.start();
        threads_manager.barrier();

        TimeStats time_stats;
        for (int i = 0; i < tasks.size(); i++) {
            time_stats += tasks[i]->get_results();
        }

        return time_stats.stats[kind][call].total_time;
    }

    void run_test(unsigned kind, unsigned call, size_t threads_number,
                  size_t alloc_size, unsigned mem_operations_num)
    {
        allocator_factory.initialize_allocator(kind);
        float ref_time = run(AllocatorTypes::STANDARD_ALLOCATOR, call,
                             threads_number, alloc_size, mem_operations_num);
        float perf_time =
            run(kind, call, threads_number, alloc_size, mem_operations_num);
        float ref_delta_time_percent =
            allocator_factory.calc_ref_delta(ref_time, perf_time);

        GTestAdapter::RecordProperty("total_time_spend_on_alloc", perf_time);
        GTestAdapter::RecordProperty("alloc_operations_per_thread",
                                     mem_operations_num);
        GTestAdapter::RecordProperty("ref_delta_time_percent",
                                     ref_delta_time_percent);
    }
};

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_1_thread_100_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 1, 100,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_1_thread_4096_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 1, 4096,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_1_thread_1000_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 1, 1000,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_1_thread_1001_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 1, 1001,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_1_thread_1572864_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 1, 1572864,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_10_thread_100_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 10, 100,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_10_thread_4096_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 10, 4096,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_10_thread_1000_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 10, 1000,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_10_thread_1001_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 10, 1001,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_10_thread_1572864_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 10, 1572864,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_72_thread_100_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 72, 100,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_72_thread_4096_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 72, 4096,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_72_thread_1000_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 72, 1000,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_72_thread_1001_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 72, 1001,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_malloc_72_thread_1572864_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::MALLOC, 72, 1572864,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_1_thread_100_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 1, 100,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_1_thread_4096_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 1, 4096,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_1_thread_1000_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 1, 1000,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_1_thread_1001_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 1, 1001,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_1_thread_1572864_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 1, 1572864,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_10_thread_100_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 10, 100,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_10_thread_4096_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 10, 4096,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_10_thread_1000_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 10, 1000,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_10_thread_1001_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 10, 1001,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_10_thread_1572864_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 10, 1572864,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_72_thread_100_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 72, 100,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_72_thread_4096_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 72, 4096,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_72_thread_1000_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 72, 1000,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_72_thread_1001_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 72, 1001,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_calloc_72_thread_1572864_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::CALLOC, 72, 1572864,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_1_thread_100_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 1, 100,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_1_thread_4096_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 1, 4096,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_1_thread_1000_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 1, 1000,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_1_thread_1001_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 1, 1001,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_1_thread_1572864_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 1, 1572864,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_10_thread_100_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 10, 100,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_10_thread_4096_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 10, 4096,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_10_thread_1000_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 10, 1000,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_10_thread_1001_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 10, 1001,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_10_thread_1572864_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 10, 1572864,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_72_thread_100_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 72, 100,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_72_thread_4096_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 72, 4096,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_72_thread_1000_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 72, 1000,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_72_thread_1001_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 72, 1001,
             10000);
}

TEST_F(PmemAllocPerformanceTest,
       test_TC_MEMKIND_MEMKIND_PMEM_realloc_72_thread_1572864_bytes)
{
    run_test(AllocatorTypes::MEMKIND_PMEM, FunctionCalls::REALLOC, 72, 1572864,
             10000);
}
