// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include <memkind.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <random>
#include <vector>

#define MB (1024 * 1024)
#define PRINT_FREQ 100000
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define BENCHMARK_LOG "bench_single_thread_%Y%m%d-%H%M"

static size_t block_size[] = {8519680, 4325376, 8519680, 4325376, 8519680,
                              4325376, 8519680, 4325376, 432517,  608478};

static struct memkind *pmem_kind;
static FILE *log_file;

static const char *const log_tag[] = {"_mem_default.log",
                                      "_mem_conservative.log"};

static void usage(char *name)
{
    fprintf(
        stderr,
        "Usage: %s pmem_kind_dir_path pmem_size pmem_policy test_time_limit_in_sec\n",
        name);
}

static int fragmentatation_test(size_t pmem_max_size, double test_time)
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
        size_t length = pmem_strs.size() / 2;
        std::uniform_int_distribution<size_t> m_index(0, length - 1);

        while ((pmem_str = static_cast<char *>(
                    memkind_malloc(pmem_kind, size))) == nullptr) {
            size_t to_evict = m_index(mt);
            char *str_to_evict = pmem_strs[to_evict];
            size_t evict_size =
                memkind_malloc_usable_size(pmem_kind, str_to_evict);
            total_allocated -= evict_size;

            if (total_allocated < total_size * 0.1) {
                fprintf(stderr, "allocated less than 10 percent.\n");
                return 1;
            }
            memkind_free(pmem_kind, str_to_evict);
            pmem_strs.erase(pmem_strs.begin() + to_evict);
        }
        pmem_strs.push_back(pmem_str);
        total_allocated += memkind_malloc_usable_size(pmem_kind, pmem_str);

        if (print_iter % PRINT_FREQ == 0) {
            fprintf(log_file, "%f\n",
                    static_cast<double>(total_allocated) / total_size);
            fflush(stdout);
        }

        auto finish = std::chrono::steady_clock::now();
        elapsed_seconds =
            std::chrono::duration_cast<std::chrono::duration<double>>(finish -
                                                                      start)
                .count();
    } while (elapsed_seconds < test_time);
    return 0;
}

static int create_pmem(const char *pmem_dir, size_t pmem_size,
                       memkind_mem_usage_policy policy)
{
    int err = 0;
    if (pmem_size == 0) {
        fprintf(stderr, "Invalid size to pmem kind must be not equal zero.\n");
        return 1;
    }

    if (policy > MEMKIND_MEM_USAGE_POLICY_MAX_VALUE) {
        fprintf(stderr, "Invalid memory usage policy param %u.\n", policy);
        return 1;
    }

    memkind_config *pmem_cfg = memkind_config_new();
    if (!pmem_cfg) {
        fprintf(stderr, "Unable to create pmem configuration.\n");
        return 1;
    }

    memkind_config_set_path(pmem_cfg, pmem_dir);
    memkind_config_set_size(pmem_cfg, pmem_size);
    memkind_config_set_memory_usage_policy(pmem_cfg, policy);

    err = memkind_create_pmem_with_config(pmem_cfg, &pmem_kind);
    memkind_config_delete(pmem_cfg);
    if (err) {
        fprintf(stderr, "Unable to create pmem kind.\n");
        return 1;
    }

    return 0;
}

static int create_log_file(memkind_mem_usage_policy policy)
{
    char file_name[100] = {'\0'};
    auto result = std::time(nullptr);

    strftime(file_name, 100, BENCHMARK_LOG, std::localtime(&result));
    std::strcat(file_name, log_tag[policy]);

    if ((log_file = fopen(file_name, "w+")) == nullptr) {
        fprintf(stderr, "Cannot create output file %s.\n", file_name);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    char *pmem_dir;
    size_t pmem_size;
    memkind_mem_usage_policy pmem_policy;
    double test_time_limit_in_sec;
    int status = 0;
    int err = 0;

    if (argc != 5) {
        usage(argv[0]);
        return 1;
    } else {
        pmem_dir = argv[1];
        pmem_size = std::stoull(argv[2]) * MB;
        pmem_policy =
            static_cast<memkind_mem_usage_policy>(std::stoul(argv[3]));
        test_time_limit_in_sec = std::stod(argv[4]);
    }

    err = create_pmem(pmem_dir, pmem_size, pmem_policy);
    if (err) {
        fprintf(stderr, "Cannot create pmem.\n");
        return 1;
    }

    err = create_log_file(pmem_policy);
    if (err) {
        memkind_destroy_kind(pmem_kind);
        fprintf(stderr, "Cannot create log file.\n");
        return 1;
    }

    status = fragmentatation_test(pmem_size, test_time_limit_in_sec);

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        fprintf(stderr, "Unable to destroy pmem kind.\n");
    }

    fclose(log_file);
    return status;
}
