/*
 * Copyright (C) 2019 Intel Corporation.
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

#include <memkind.h>

#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

#define MB (1024 * 1024)
#define PRINT_FREQ 1000000
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static size_t block_size [] =
{8519680, 4325376, 8519680, 4325376, 8519680, 4325376, 8519680, 4325376, 432517, 608478};

static struct memkind *pmem_kind = nullptr;
static FILE *output_file = nullptr;

static void usage(char *name)
{
    fprintf(stderr,
            "Usage: %s pmem_kind_dir_path pmem_size test_time_limit_in_sec output_file_name\n",
            name);
}

static void fragmentatation_test (size_t pmem_max_size, double test_time)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> m_size(0, ARRAY_SIZE(block_size) - 1);
    char *pmem_str = nullptr;
    std::vector<char *> pmem_strs;
    size_t total_size = pmem_max_size;
    size_t print_iter = 0;
    size_t total_allocated = 0;
    double elapsed_seconds;
    auto start = std::chrono::steady_clock::now();

    do {
        print_iter++;
        int index = m_size(mt);
        size_t size = block_size[index];
        int length = pmem_strs.size() / 2;
        std::uniform_int_distribution<> m_index(0, length - 1);

        while ((pmem_str = static_cast<char *>(memkind_malloc(pmem_kind,
                                                              size))) == nullptr) {
            int to_evict = m_index(mt);
            char *str_to_evict = pmem_strs[to_evict];
            size_t evict_size = memkind_malloc_usable_size(pmem_kind, str_to_evict);
            total_allocated -= evict_size;

            if (total_allocated < total_size * 0.1) {
                fprintf(stderr,"allocated less than 10 percent\n");
                exit(0);
            }
            memkind_free(pmem_kind, str_to_evict);
            pmem_strs.erase(pmem_strs.begin() + to_evict);
        }
        pmem_strs.push_back(pmem_str);
        total_allocated += memkind_malloc_usable_size(pmem_kind, pmem_str);

        if (print_iter % PRINT_FREQ == 0) {
            fprintf(output_file,"%f\n", static_cast<double>(total_allocated) / total_size);
            fflush(stdout);
        }
        auto finish = std::chrono::steady_clock::now();
        elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double> >
                          (finish - start).count();
    } while (elapsed_seconds < test_time);
}

int main(int argc, char *argv[])
{
    char *pmem_dir;
    char *output_log_path;
    size_t pmem_size;
    unsigned long pmem_policy;
    double test_time_limit_in_sec;
    int err = 0;

    if (argc != 6) {
        usage(argv[0]);
        return 1;
    } else {
        pmem_dir = argv[1];
        pmem_size = std::stoull(argv[2]) * MB;
        pmem_policy = std::stoul(argv[3]);
        test_time_limit_in_sec = std::stod(argv[4]);
        output_log_path = argv[5];
    }

    if (pmem_size == 0 ) {
        fprintf(stderr, "Invalid size to pmem kind must be not equal zero\n");
        return 1;
    }

    if ((output_file = fopen(output_log_path, "w+")) == nullptr) {
        fprintf(stderr, "Cannot create output file %s\n", output_log_path);
        return 1;
    }

    if (pmem_policy > MEMKIND_MEM_USAGE_POLICY_MAX_VALUE) {
        fprintf(stderr, "Invalid memory usage policy param %lu.\n", pmem_policy);
        return 1;
    }

    memkind_config *pmem_cfg = memkind_config_new();
    if (!pmem_cfg) {
        fprintf(stderr, "Unable to create pmem configuration.\n");
        return 1;
    }
    memkind_config_set_path(pmem_cfg, pmem_dir);
    memkind_config_set_size(pmem_cfg, pmem_size);
    memkind_config_set_memory_usage_policy(pmem_cfg,
                                           static_cast<memkind_mem_usage_policy>(pmem_policy));

    err = memkind_create_pmem_with_config(pmem_cfg, &pmem_kind);
    if (err) {
        fprintf(stderr, "Unable to create pmem kind.\n");
        return 1;
    }

    fragmentatation_test(pmem_size, test_time_limit_in_sec);

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        fprintf(stdout, "Unable to destroy pmem kind.\n");
        fclose(output_file);
        return 1;
    }
    memkind_config_delete(pmem_cfg);

    fclose(output_file);
    return 0;
}
