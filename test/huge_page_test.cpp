// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */

#include "common.h"

#include "allocator_perf_tool/HugePageOrganizer.hpp"
#include "allocator_perf_tool/HugePageUnmap.hpp"

#include "Thread.hpp"
#include "TimerSysTime.hpp"

/*
 * This test was created because of the munmap() fail in jemalloc.
 * There are two root causes of the error:
 * - kernel bug (munmap() fails when the size is not aligned)
 * - heap Manager doesn’t provide size aligned to 2MB pages for munmap()
 * Test allocates 2000MB using Huge Pages
 * (50threads*10operations*4MBalloc_size), but it needs extra Huge Pages due to
 * overhead caused by heap management.
 */
class HugePageTest: public ::testing::Test
{
protected:
    void run()
    {
        unsigned mem_operations_num = 10;
        size_t threads_number = 50;
        bool touch_memory = true;
        size_t size_1MB = 1024 * 1024;
        size_t alignment = 2 * size_1MB;
        size_t alloc_size = 4 * size_1MB;

        std::vector<Thread *> threads;
        std::vector<Task *> tasks;

        TimerSysTime timer;
        timer.start();

        // This bug occurs more frequently under stress of multithreaded
        // allocations.
        for (int i = 0; i < threads_number; i++) {
            Task *task =
                new HugePageUnmap(mem_operations_num, touch_memory, alignment,
                                  alloc_size, HBW_PAGESIZE_2MB);
            tasks.push_back(task);
            threads.push_back(new Thread(task));
        }

        float elapsed_time = timer.getElapsedTime();

        ThreadsManager threads_manager(threads);
        threads_manager.start();
        threads_manager.barrier();
        threads_manager.release();

        // task release
        for (int i = 0; i < tasks.size(); i++) {
            delete tasks[i];
        }

        RecordProperty("threads_number", threads_number);
        RecordProperty("memory_operations_per_thread", mem_operations_num);
        RecordProperty("elapsed_time", elapsed_time);
    }
};

// Test passes when there is no crash.
TEST_F(HugePageTest, test_TC_MEMKIND_ext_UNMAP_HUGE_PAGE)
{
    HugePageOrganizer huge_page_organizer(1024);
    run();
}
