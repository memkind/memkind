// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind/internal/memkind_memtier.h>

#include <argp.h>
#include <assert.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <stdint.h>
#include <thread>
#include <vector>

class counter_bench_alloc
{
public:
    double run(size_t nruns, size_t nthreads) const
    {
        std::chrono::time_point<std::chrono::system_clock> start, end;
        start = std::chrono::system_clock::now();

        if (nthreads == 1) {
            for (size_t r = 0; r < nruns; ++r) {
                single_run();
            }
        } else {
            std::vector<std::thread> vthread(nthreads);
            for (size_t r = 0; r < nruns; ++r) {
                for (size_t k = 0; k < nthreads; ++k) {
                    vthread[k] = std::thread([&]() { single_run(); });
                }
                for (auto &t : vthread) {
                    t.join();
                }
            }
        }
        end = std::chrono::system_clock::now();
        auto time_per_op =
            static_cast<double>((end - start).count()) / m_iteration;
        return time_per_op / (nruns * nthreads);
    }
    virtual ~counter_bench_alloc() = default;

protected:
    const size_t m_size = 512;
    virtual void *bench_alloc(size_t) const = 0;
    virtual void bench_free(void *) const = 0;

private:
    const size_t m_iteration = 10000000;
    void single_run() const
    {
        std::vector<void *> v;
        v.reserve(m_iteration);
        for (size_t i = 0; i < m_iteration; i++) {
            v.emplace_back(bench_alloc(m_size));
        }
        for (size_t i = 0; i < m_iteration; i++) {
            bench_free(v[i]);
        }
        v.clear();
    }
};

class memkind_bench_alloc: public counter_bench_alloc
{
protected:
    void *bench_alloc(size_t size) const final
    {
        return memkind_malloc(MEMKIND_DEFAULT, size);
    }

    void bench_free(void *ptr) const final
    {
        memkind_free(MEMKIND_DEFAULT, ptr);
    }
};

class memtier_kind_bench_alloc: public counter_bench_alloc
{
protected:
    void *bench_alloc(size_t size) const final
    {
        return memtier_kind_malloc(MEMKIND_DEFAULT, size);
    }

    void bench_free(void *ptr) const final
    {
        memtier_kind_free(MEMKIND_DEFAULT, ptr);
    }
};

class memtier_bench_alloc: public counter_bench_alloc
{
public:
    memtier_bench_alloc()
    {
        m_tier_builder = memtier_builder_new(MEMTIER_POLICY_STATIC_RATIO);
        memtier_builder_add_tier(m_tier_builder, MEMKIND_DEFAULT, 1);
        if (m_tier_builder) {
            m_tier_memory =
                memtier_builder_construct_memtier_memory(m_tier_builder);
        } else {
            m_tier_memory = NULL;
        }
    }

    ~memtier_bench_alloc()
    {
        memtier_builder_delete(m_tier_builder);
        memtier_delete_memtier_memory(m_tier_memory);
    }

protected:
    void *bench_alloc(size_t size) const final
    {
        return memtier_malloc(m_tier_memory, size);
    }

    void bench_free(void *ptr) const final
    {
        memtier_realloc(m_tier_memory, ptr, 0);
    }

private:
    struct memtier_builder *m_tier_builder;
    struct memtier_memory *m_tier_memory;
};

class memtier_multiple_bench_alloc: public counter_bench_alloc
{
public:
    memtier_multiple_bench_alloc(memtier_policy_t policy)
    {
        m_tier_builder = memtier_builder_new(policy);
        memtier_builder_add_tier(m_tier_builder, MEMKIND_DEFAULT, 1);
        memtier_builder_add_tier(m_tier_builder, MEMKIND_REGULAR, 1);
        if (m_tier_builder) {
            m_tier_memory =
                memtier_builder_construct_memtier_memory(m_tier_builder);
        } else {
            m_tier_memory = NULL;
        }
    }

    ~memtier_multiple_bench_alloc()
    {
        memtier_builder_delete(m_tier_builder);
        memtier_delete_memtier_memory(m_tier_memory);
    }

protected:
    void *bench_alloc(size_t size) const final
    {
        return memtier_malloc(m_tier_memory, size);
    }

    void bench_free(void *ptr) const final
    {
        memtier_realloc(m_tier_memory, ptr, 0);
    }

private:
    struct memtier_builder *m_tier_builder;
    struct memtier_memory *m_tier_memory;
};

using Benchptr = std::unique_ptr<counter_bench_alloc>;
struct BenchArgs {
    Benchptr bench;
    int thread_no;
    int run_no;
};

// clang-format off
static int parse_opt(int key, char *arg, struct argp_state *state)
{
    auto args = (BenchArgs *)state->input;
    switch (key) {
        case 'm':
            args->bench = Benchptr(new memkind_bench_alloc());
            break;
        case 'k':
            args->bench = Benchptr(new memtier_kind_bench_alloc());
            break;
        case 'x':
            args->bench = Benchptr(new memtier_bench_alloc());
            break;
        case 's':
            args->bench = Benchptr(new memtier_multiple_bench_alloc(MEMTIER_POLICY_STATIC_RATIO));
            break;
        case 'd':
            args->bench = Benchptr(new memtier_multiple_bench_alloc(MEMTIER_POLICY_DYNAMIC_THRESHOLD));
            break;
        case 't':
            args->thread_no = std::strtol(arg, nullptr, 10);
            break;
        case 'r':
            args->run_no = std::strtol(arg, nullptr, 10);
            break;
    }
    return 0;
}

static struct argp_option options[] = {
    {"memkind", 'm', 0, 0, "Benchmark memkind."},
    {"memtier_kind", 'k', 0, 0, "Benchmark memtier_memkind."},
    {"memtier", 'x', 0, 0, "Benchmark memtier_memory - single tier."},
    {"memtier_multiple", 's', 0, 0, "Benchmark memtier_memory - two tiers, static ratio."},
    {"memtier_multiple", 'd', 0, 0, "Benchmark memtier_memory - two tiers, dynamic threshold."},
    {"thread", 't', "int", 0, "Threads numbers."},
    {"runs", 'r', "int", 0, "Benchmark run numbers."},
    {0}};
// clang-format on

static struct argp argp = {options, parse_opt, nullptr, nullptr};

int main(int argc, char *argv[])
{
    struct BenchArgs arguments = {
        .bench = nullptr, .thread_no = 0, .run_no = 1};

    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    double time_per_op =
        arguments.bench->run(arguments.run_no, arguments.thread_no);
    std::cout << "Mean second per operation:" << time_per_op << std::endl;
    return 0;
}
