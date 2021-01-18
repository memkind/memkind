// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2021 Intel Corporation. */


#include <pmem_allocator.h>
#include <memkind.h>

#include <tbb/concurrent_hash_map.h>
#include <tbb/spin_rw_mutex.h>

#include <chrono>
#include <memory>
#include <iostream>
#include <sys/stat.h>

struct dram_mutex {
    using scoped_lock = tbb::spin_rw_mutex::scoped_lock;

    dram_mutex()
    {
        mtx = std::unique_ptr<tbb::spin_rw_mutex>(new tbb::spin_rw_mutex);
    }

    operator tbb::spin_rw_mutex &()
    {
        return *mtx;
    }

    // Mutex traits
    static const bool is_rw_mutex = true;
    static const bool is_recursive_mutex = true;
    static const bool is_fair_mutex = true;

    std::unique_ptr<tbb::spin_rw_mutex> mtx;
};

using allocator_type = libmemkind::pmem::allocator<std::pair<const int, int>>;
using map_type =
    tbb::concurrent_hash_map<int, int, tbb::tbb_hash_compare<int>, allocator_type, dram_mutex>;

static constexpr int NUM_ELEMENTS = 100000;
static constexpr size_t max_pmem_size = 1024*1024*1024;

void example(std::string pmem_directory)
{
    const size_t max_pmem_size = 1024*1024*1024;

    allocator_type alloc(pmem_directory, max_pmem_size);
    map_type map(alloc);

    std::chrono::steady_clock::time_point start, end;
    start = std::chrono::steady_clock::now();

    for (int i = 0; i < NUM_ELEMENTS; i++) {
        map.insert(typename map_type::value_type(i, i));
    }

    end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                    (end - start).count();

    std::cout << "Inserts took: " << duration << " ms" << std::endl;
}

int main(int argc, char *argv[])
{
    const char *pmem_directory = "/tmp/";

    if (argc > 2) {
        std::cerr << "Usage: memkind_cpp_concurrent_hash_map [directory path]\n"
                  << "\t[directory path] - directory where temporary file is created (default = \"/tmp/\")"
                  << std::endl;
        return 0;
    } else if (argc == 2) {
        struct stat st;
        if (stat(argv[1], &st) != 0 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr,"%s : Invalid path to pmem kind directory\n", argv[1]);
            return 1;
        }
        int status = memkind_check_dax_path(argv[1]);
        if (!status) {
            std::cout << "PMEM kind %s is on DAX-enabled File system.\n" << std::endl;
        } else {
            std::cout << "PMEM kind %s is not on DAX-enabled File system.\n" << std::endl;
        }
        pmem_directory = argv[1];
    }

    example(pmem_directory);

    return 0;
}
