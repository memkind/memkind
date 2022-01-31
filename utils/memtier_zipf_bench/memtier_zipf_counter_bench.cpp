// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/memkind_memtier.h>

#include <argp.h>
#include <assert.h>
#include <condition_variable>
#include <iostream>
#include <random>
#include <thread>

enum class TestCase
{
    /**
     * @brief read-modify-write test, random cacheline from malloc
     */
    RANDOM_INC,

    /**
     * @brief read-modify-write test, sequential cachelines from malloc
     */
    SEQ_INC,

    /**
     * @brief read-write test, random cacheline from malloc
     */
    RANDOM_COPY,

    /**
     * @brief read-write test, sequential cachelines from malloc
     */
    SEQ_COPY
};

static uint64_t clock_bench_time_ms()
{
    struct timespec tv;
    int ret = clock_gettime(CLOCK_MONOTONIC, &tv);
    assert(ret == 0);
    return tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
}

struct RunInfo {
    /* total operations: types_count * iterations_minor * iterations_major
     *
     * for iterations_major:
     *      for types_count:
     *          allocate()
     *      for iterations_minor:
     *          access()
     */
    TestCase test_case;
    size_t min_size;
    size_t types_count;
    size_t iterations_major;
    size_t iterations_minor;
    memtier_policy_t policy;
    std::pair<size_t, size_t> dram_pmem_ratio;
};

struct BenchResults {
    uint64_t malloc_time_ms;
    uint64_t access_time_ms;
    std::vector<uint64_t> malloc_times_ms;
    std::vector<uint64_t> access_times_ms;
};

class Fence
{
    bool ready = false;
    std::mutex m;
    std::condition_variable cv;

public:
    void Start()
    {
        std::lock_guard<std::mutex> lock(m);
        ready = true;
        cv.notify_all();
    }

    void Await()
    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&] { return ready; });
    }
};

/// @warning: API is not thread safe, only one function per instance
/// should be called at once
class Loader
{
    static std::random_device dev;
    std::mt19937 generator;
    TestCase test_case;
    std::shared_ptr<void> data;
    size_t data_size;
    size_t iterations;
    std::shared_ptr<std::thread> this_thread;
    uint64_t execution_time_millis = 0;

    // clang-format off
    /// @return time in milliseconds per whole data access
    uint64_t GenerateAccessOnce()
    {
        const int CACHE_LINE_LEAP = 5;
        uint64_t timestamp_start = clock_bench_time_ms();
        const size_t data_size_u64 = data_size / sizeof(uint64_t);
        std::uniform_int_distribution<uint64_t> distr(0, data_size_u64);

        switch (test_case) {
            case (TestCase::RANDOM_INC):
                for (size_t it = 0; it < iterations; ++it) {
                    for (size_t i = 0; i < data_size_u64; i += CACHE_LINE_SIZE_U64) {
                        size_t index = distr(generator);
                        static_cast<uint64_t *>(data.get())[index]++;
                    }
                }
                break;
            case (TestCase::SEQ_INC):
                for (size_t it = 0; it < iterations; ++it) {
                    for (size_t i = 0; i < data_size_u64; i += CACHE_LINE_SIZE_U64) {
                        static_cast<uint64_t *>(data.get())[i]++;
                    }
                }
                break;
            case (TestCase::RANDOM_COPY):
                for (size_t it = 0; it < iterations; ++it) {
                    for (size_t i = 0; i < data_size_u64; i += CACHE_LINE_SIZE_U64) {
                        // TODO: consider using a table with prepared random
                        // values in case of a CPU-bottleneck
                        size_t index_from = distr(generator);
                        size_t index_to = distr(generator);
                        static_cast<volatile uint64_t *>(data.get())[index_to] =
                            static_cast<volatile uint64_t *>(data.get())[index_from];
                    }
                }
                break;
            case (TestCase::SEQ_COPY):
                for (size_t it = 0; it < iterations; ++it) {
                    for (size_t i = 0; i < data_size_u64 - CACHE_LINE_LEAP * CACHE_LINE_SIZE_U64;
                            i += CACHE_LINE_SIZE_U64) {
                        static_cast<volatile uint64_t *>(data.get())[i] =
                            static_cast<volatile uint64_t *>(data.get())[i + 5 * CACHE_LINE_SIZE_U64];
                    }
                }
                break;
        }
        return clock_bench_time_ms() - timestamp_start;
    }
    // clang-format on

public:
    Loader(TestCase test_case, std::shared_ptr<void> data, size_t data_size,
           size_t iterations)
        : generator(dev()),
          test_case(test_case),
          data(data),
          data_size(data_size),
          iterations(iterations)
    {}

    void PrepareOnce(std::shared_ptr<Fence> fence)
    {
        assert(!this_thread);
        this_thread = std::make_shared<std::thread>([this, fence]() {
            fence->Await();
            this->execution_time_millis = this->GenerateAccessOnce();
        });
    }

    uint64_t CollectResults()
    {
        assert(this_thread &&
               "Collecting results of thread that was not started");
        this_thread->join();
        return this->execution_time_millis;
    }
};

std::random_device Loader::dev;

static std::shared_ptr<memtier_memory>
create_memory(memtier_policy_t policy,
              std::pair<size_t, size_t> dram_pmem_ratio)
{
    struct memtier_builder *m_tier_builder = memtier_builder_new(policy);
    memtier_builder_add_tier(m_tier_builder, MEMKIND_DEFAULT,
                             dram_pmem_ratio.first);
    memtier_builder_add_tier(m_tier_builder, MEMKIND_DAX_KMEM,
                             dram_pmem_ratio.second);
    auto memory = std::shared_ptr<memtier_memory>(
        memtier_builder_construct_memtier_memory(m_tier_builder),
        [](struct memtier_memory *m) { memtier_delete_memtier_memory(m); });
    memtier_builder_delete(m_tier_builder);

    return memory;
}

class LoaderCreator
{
    TestCase test_case;
    size_t size, iterations;
    std::shared_ptr<memtier_memory> memory;

public:
    LoaderCreator(TestCase test_case, size_t size, size_t iterations,
                  std::shared_ptr<memtier_memory> memory)
        : test_case(test_case),
          size(size),
          iterations(iterations),
          memory(memory)
    {}

    std::shared_ptr<Loader> CreateLoader()
    {
        void *allocated_memory = memtier_malloc(memory.get(), size);
        assert(allocated_memory && "memtier malloc failed");
        std::shared_ptr<void> allocated_memory_ptr(allocated_memory,
                                                   memtier_free);

        return std::make_shared<Loader>(test_case, allocated_memory_ptr, size,
                                        iterations);
    }
};

/// @return malloc time, access time
static std::pair<uint64_t, uint64_t>
create_and_run_loaders(std::vector<LoaderCreator> &creators)
{
    std::vector<std::shared_ptr<Loader>> loaders;

    auto fence = std::make_shared<Fence>();
    loaders.reserve(creators.size());

    uint64_t start_mallocs_timestamp = clock_bench_time_ms();
    for (auto &creator : creators) {
        loaders.push_back(creator.CreateLoader());
    }
    uint64_t malloc_time = clock_bench_time_ms() - start_mallocs_timestamp;

    for (auto &loader : loaders) {
        loader->PrepareOnce(fence);
    }
    fence->Start();
    uint64_t summed_access_time_ms = 0u;
    for (auto &loader : loaders) {
        summed_access_time_ms += loader->CollectResults();
    }

    return std::make_pair(malloc_time, summed_access_time_ms);
}

/// @param N number of elements
/// @param s distribution parameter
/// @return \sum_{n=1}^{N}(1/n^s)
static double calculate_zipf_denominator(int N, double s)
{
    double ret = 0;
    for (int n = 1; n <= N; ++n) {
        ret += 1.0 / pow(n, s);
    }

    return ret;
}

/// Creates loader creators - each one holds parameters for a loader.
/// The number of iterations for each loader is described by zipf distribution.
/// Instead of generating random numbers that follow zipf distribution,
/// we create a population that strictly follows zipf distribution by hand.
///
/// The zipf distribution is described by:
///     f(k, s, N) = \frac{1/k^s}{\sum_{n=1}^{N}(1/n^s)}
///
/// where:
///     - k: rank of element, starts with 1,
///     - s: distribution parameter,
///     - N: total number of elements
///
static std::vector<LoaderCreator>
create_loader_creators(TestCase test_case, size_t min_size, size_t types_count,
                       size_t total_iterations, memtier_policy_t policy,
                       std::pair<size_t, size_t> pmem_dram_ratio)
{
    const double s = 2.0;
    std::vector<LoaderCreator> creators;

    auto memory = create_memory(policy, pmem_dram_ratio);
    creators.reserve(types_count);
    double zipf_denominator = calculate_zipf_denominator(types_count, s);

    // create loader creators that share memory
    double probability_sum = 0;
    for (size_t size = min_size, k = 1u; size < min_size + types_count;
         ++size, ++k) {
        double probability = 1 / pow(k, s) / zipf_denominator;
        probability_sum += probability;
        size_t iterations = (size_t)(total_iterations * probability);
        std::cout << "Type " << k << " iterations count: " << iterations
                  << std::endl;
        creators.push_back(LoaderCreator(test_case, size, iterations, memory));
    }
    assert(abs(probability_sum - 1.0) < 0.0001 &&
           "probabilities do not sum to 100% !");

    return creators;
}

/// @return duration of allocations and accesses in milliseconds
BenchResults run_test(const RunInfo &info)
{

    std::vector<LoaderCreator> loader_creators = create_loader_creators(
        info.test_case, info.min_size, info.types_count, info.iterations_minor,
        info.policy, info.dram_pmem_ratio);

    std::vector<uint64_t> malloc_times;
    std::vector<uint64_t> access_times;
    malloc_times.reserve(info.iterations_major);
    access_times.reserve(info.iterations_major);

    BenchResults ret = {0};

    ret.malloc_times_ms.reserve(info.iterations_major);
    ret.access_times_ms.reserve(info.iterations_major);

    for (size_t major_it = 0; major_it < info.iterations_major; ++major_it) {
        std::pair<uint64_t, uint64_t> times =
            create_and_run_loaders(loader_creators);
        ret.malloc_time_ms += times.first;
        ret.access_time_ms += times.second;
        ret.malloc_times_ms.push_back(times.first);
        ret.access_times_ms.push_back(times.second);
    }

    return ret;
}

struct BenchArgs {
    memtier_policy_t policy;
    TestCase test_case;
    size_t test_iterations;
    size_t access_iterations;
    size_t threads_count;
    size_t dram_ratio_part;
    size_t pmem_ratio_part;
};

static int parse_opt(int key, char *arg, struct argp_state *state)
{
    auto args = (BenchArgs *)state->input;
    switch (key) {
        case 's':
            args->policy = MEMTIER_POLICY_STATIC_RATIO;
            break;
        case 'm':
            args->policy = MEMTIER_POLICY_DATA_MOVEMENT;
            break;
        case 'A':
            args->test_case = TestCase::RANDOM_INC;
            break;
        case 'B':
            args->test_case = TestCase::SEQ_INC;
            break;
        case 'C':
            args->test_case = TestCase::RANDOM_COPY;
            break;
        case 'D':
            args->test_case = TestCase::SEQ_COPY;
            break;
        case 'i':
            assert((size_t)arg > 0 &&
                   "test iterations must be greater than zero");
            args->test_iterations = std::strtoul(arg, nullptr, 10);
            break;
        case 'a':
            assert((size_t)arg > 0 &&
                   "access iterations must be greater than zero");
            args->access_iterations = std::strtoul(arg, nullptr, 10);
            break;
        case 't':
            assert((size_t)arg > 0 && "at least one thread is required");
            args->threads_count = std::strtoul(arg, nullptr, 10);
            break;
        case 'r':
        {
            std::string delimiter = ":";
            std::string ratio_string = arg;
            size_t char_count = ratio_string.find(delimiter);
            const char *dram_ratio_part_str =
                ratio_string.substr(0, char_count).c_str();
            args->dram_ratio_part =
                std::strtoul(dram_ratio_part_str, nullptr, 10);
            ratio_string.erase(0, char_count + delimiter.length());
            const char *pmem_ratio_part_str =
                ratio_string.substr(0, ratio_string.find(delimiter)).c_str();
            args->pmem_ratio_part =
                std::strtoul(pmem_ratio_part_str, nullptr, 10);
            break;
        }
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp_option options[] = {
    {"static", 's', 0, 0, "Benchmark static ratio policy.", 0},
    {"movement", 'm', 0, 0, "Benchmark data movement policy.", 0},
    {0, 'A', 0, 0, "Benchmark test case Random Incrementation.", 1},
    {0, 'B', 0, 0, "Benchmark test case Sequential Incrementation.", 1},
    {0, 'C', 0, 0, "Benchmark test case Random Copy.", 1},
    {0, 'D', 0, 0, "Benchmark test case Sequential Copy.", 1},
    {"iterations", 'i', "size_t", 0, "Benchmark iterations.", 2},
    {"access", 'a', "size_t", 0, "Number of accesses done for each thread.", 2},
    {"threads", 't', "size_t", 0, "Threads count.", 2},
    {"ratio", 'r', "dram:pmem", 0, "DRAM to PMEM ratio.", 2},
    {0}};

static struct argp argp = {options, parse_opt, nullptr, nullptr};

int main(int argc, char *argv[])
{
    size_t LOADER_SIZE = 1024 * 1024 * 512; // 512 MB

    struct BenchArgs args = {.policy = MEMTIER_POLICY_STATIC_RATIO,
                             .test_case = TestCase::SEQ_INC,
                             .test_iterations = 1,
                             .access_iterations = 500,
                             .threads_count = 1,
                             .dram_ratio_part = 1,
                             .pmem_ratio_part = 8};
    argp_parse(&argp, argc, argv, 0, 0, &args);

    RunInfo info = {.test_case = args.test_case,
                    .min_size = LOADER_SIZE,
                    .types_count = args.threads_count,
                    .iterations_major = args.test_iterations,
                    .iterations_minor = args.access_iterations,
                    .policy = args.policy,
                    .dram_pmem_ratio = std::make_pair(args.dram_ratio_part,
                                                      args.pmem_ratio_part)};

    BenchResults stats = run_test(info);

    std::cout << "Measured execution time [malloc | access]: ["
              << stats.malloc_time_ms << "|" << stats.access_time_ms << "]"
              << std::endl;

    std::cout << "Malloc times: ";
    for (uint64_t malloc_time_ms : stats.malloc_times_ms)
        std::cout << malloc_time_ms << ",";
    std::cout << std::endl;

    std::cout << "Access times: ";
    for (uint64_t access_time_ms : stats.access_times_ms)
        std::cout << access_time_ms << ",";
    std::cout << std::endl;

    return 0;
}
