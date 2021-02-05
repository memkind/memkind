// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */

#include <vector>

#include "Task.hpp"

class HugePageUnmap: public Task
{

public:
    HugePageUnmap(int operations, bool touch_memory, size_t alignment_size,
                  size_t alloc_size, hbw_pagesize_t page_size)
        : mem_operations_num(operations),
          touch_memory(touch_memory),
          alignment_size(alignment_size),
          alloc_size(alloc_size),
          page_size(page_size)
    {}

    ~HugePageUnmap()
    {
        for (int i = 0; i < mem_operations_num; i++) {
            hbw_free(results[i].ptr);
        }
    };

    void run()
    {
        void *ptr = NULL;

        for (int i = 0; i < mem_operations_num; i++) {
            int ret = hbw_posix_memalign_psize(&ptr, alignment_size, alloc_size,
                                               page_size);

            ASSERT_EQ(ret, 0);
            ASSERT_FALSE(ptr == NULL);

            if (touch_memory) {
                memset(ptr, alignment_size, alignment_size);
            }

            memory_operation data;
            data.ptr = ptr;
            results.push_back(data);
        }
    }

    std::vector<memory_operation> get_results()
    {
        return results;
    }

private:
    int mem_operations_num;
    std::vector<memory_operation> results;

    bool touch_memory;
    size_t alignment_size;
    size_t alloc_size;
    hbw_pagesize_t page_size;
};
