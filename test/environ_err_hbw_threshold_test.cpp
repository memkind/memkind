// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <hbwmalloc.h>
#include <memkind.h>
#include <memory>
#include <numa.h>
#include <numaif.h>
#include <omp.h>
#include <sys/sysinfo.h>

#include "common.h"
#include "memkind/internal/memkind_hbw.h"
#include "memory_topology.h"
#include "TestPrereq.hpp"

/* TODO This test is run with overridden MEMKIND_HBW_NODES environment variable
 * and tries to perform allocation from DRAM using hbw_malloc() using
 * default HBW_POLICY_PREFERRED policy.
 */
int main(int argc, char *argv[])
{
    const int retcode_success = 0;
    const int retcode_not_available = 1;
    const int retcode_error = 2;

    if (argc == 1) {
        return -1;
    }

    memkind_t memory_kind;
    if (argc == 2) {
        if (std::string(argv[1]) == "MEMKIND_HBW") {
            memory_kind = MEMKIND_HBW;
        } else if (std::string(argv[1]) == "MEMKIND_HBW_ALL") {
            memory_kind = MEMKIND_HBW_ALL;
        } else {
            return -1;
        }
    }

    const char *memory_tpg = std::getenv("MEMKIND_TEST_TOPOLOGY");
    std::cout << "MEMKIND_TEST_TOPOLOGY is: " << memory_tpg << std::endl;

    TestPrereq tp;
    bool any_fail = false;
    bool any_alloc = false;

    if (tp.is_libhwloc_supported()) {
        int status = numa_available();
        if (status != 0) {
            return -1;
        }

        // use big size to ensure that we call jemalloc extent
        const size_t size = 11*MB-5;
        int threads_num = get_nprocs();
        auto topology = TopologyFactory(memory_tpg);

        #pragma omp parallel for num_threads(threads_num)
        for (int thread_id=0; thread_id<threads_num; ++thread_id) {
            cpu_set_t cpu_set;
            CPU_ZERO(&cpu_set);
            CPU_SET(thread_id, &cpu_set);

            status = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
            int cpu = sched_getcpu();
            if (thread_id != cpu) {
                any_fail = true;
                continue;
            }

            void *ptr = memkind_malloc(memory_kind, size);
            if (topology->is_kind_supported(memory_kind)) {
                if (ptr == nullptr) {
                    std::cout << "Kind supported but allocation failed" << std::endl;
                    any_fail = true;
                    continue;
                }

                memset(ptr, 0, size);
                int init_node = numa_node_of_cpu(cpu);
                auto res = topology->verify_kind(memory_kind, init_node, ptr);
                memkind_free(memory_kind, ptr);

                if (res != true) {
                    std::cout << "Verify kind failed failed" << std::endl;
                    any_fail = true;
                } else {
                    any_alloc = true;
                }
            } else {
                if (ptr != nullptr) {
                    std::cout << "Kind not supported but allocation succed" << std::endl;
                    any_fail = true;
                }
            }
        }
    } else {
        std::cout << "libhwloc is required." << std::endl;
    }

    return any_fail ? retcode_error :
           (any_alloc ? retcode_success : retcode_not_available);
}
