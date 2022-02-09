// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/memkind_memtier.h>
#include <memkind/internal/pool_allocator.h>

#include <argp.h>
#include <assert.h>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <random>
#include <thread>

#define ACCESS_STRIDE 10

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

/// dependency inversion for memkind and other allocators
class Allocator
{
public:
    virtual void *malloc(size_t size_bytes) = 0;
    virtual void free(void *ptr) = 0;
};

class StaticRatioAllocator: public Allocator
{
    std::shared_ptr<memtier_memory> memory;

public:
    StaticRatioAllocator(memtier_policy_t policy,
                         std::pair<size_t, size_t> dram_pmem_ratio)
    {
        struct memtier_builder *m_tier_builder = memtier_builder_new(policy);
        memtier_builder_add_tier(m_tier_builder, MEMKIND_DEFAULT,
                                 dram_pmem_ratio.first);
        //     memtier_builder_add_tier(m_tier_builder, MEMKIND_DAX_KMEM,
        memtier_builder_add_tier(m_tier_builder, MEMKIND_REGULAR,
                                 dram_pmem_ratio.second);
        memory = std::shared_ptr<memtier_memory>(
            memtier_builder_construct_memtier_memory(m_tier_builder),
            [](struct memtier_memory *m) { memtier_delete_memtier_memory(m); });
        memtier_builder_delete(m_tier_builder);
    }
    void *malloc(size_t size_bytes)
    {
        return memtier_malloc(memory.get(), size_bytes);
    }

    void free(void *ptr)
    {
        memtier_free(ptr);
    }
};

class PoolAllocatorWrapper: public Allocator
{
    PoolAllocator allocator;

public:
    PoolAllocatorWrapper()
    {
        pool_allocator_create(&allocator);
    }

    ~PoolAllocatorWrapper()
    {
        pool_allocator_destroy(&allocator);
    }

    void *malloc(size_t size_bytes)
    {
        return pool_allocator_malloc(&allocator, size_bytes);
    }

    void free(void *ptr)
    {
        pool_allocator_free(ptr);
    }
};

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
    size_t nof_allocations;
    size_t allocation_size;
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
    std::condition_variable clientToFenceReady;
    size_t readyClients = 0;
    size_t nofClients;
    std::condition_variable fenceToClientsStart;
    ;

public:
    Fence(size_t nof_clients = 0) : nofClients(nof_clients)
    {}
    void Start()
    {
        std::unique_lock<std::mutex> lock(m);
        // await until all threads are prepared
        clientToFenceReady.wait(lock,
                                [&] { return readyClients >= nofClients; });
        ready = true;
        fenceToClientsStart.notify_all();
    }

    void Await()
    {
        std::unique_lock<std::mutex> lock(m);
        if (++readyClients >= nofClients)
            clientToFenceReady.notify_one();

        fenceToClientsStart.wait(lock, [&] { return ready; });
    }
};

/// @warning: API is not thread safe, only one function per instance
/// should be called at once
class Loader
{
    static std::random_device dev;
    std::mt19937 generator;
    TestCase test_case;
    /// 2D array: uint64_t data[nof_allocations][allocation_size]
    volatile uint64_t **data;
    size_t allocation_size;
    size_t nof_done_allocations = 0;
    size_t nof_allocations;
    size_t iterations;
    size_t additional_stack;
    std::shared_ptr<std::thread> this_thread;
    uint64_t execution_time_millis = 0;
    std::shared_ptr<Allocator> allocator;

    // clang-format off
    /// @return time in milliseconds per whole data access
    uint64_t GenerateAccessOnce()
    {
        const int CACHE_LINE_LEAP = 5;
        uint64_t timestamp_start = clock_bench_time_ms();
//         const size_t data_size_u64 = data_size / sizeof(uint64_t);
        std::uniform_int_distribution<uint64_t> distr_alloc(0, nof_allocations);
        std::uniform_int_distribution<uint64_t> distr_internal(0, allocation_size);

        switch (test_case) {
            case (TestCase::RANDOM_INC):
                for (size_t i=0; i<iterations; ++i) {
                    for (size_t intra_allocation_it=0; intra_allocation_it<allocation_size; intra_allocation_it += ACCESS_STRIDE) {
                        for (size_t allocation_it=0; allocation_it<nof_allocations; ++allocation_it) {
                            size_t index_alloc = distr_alloc(generator);
                            size_t index_internal = distr_internal(generator);
                            data[index_alloc][index_internal]++;
                        }
                    }
                }
                break;
            case (TestCase::SEQ_INC):
                for (size_t i=0; i<iterations; ++i) {
                    for (size_t intra_allocation_it=0; intra_allocation_it<allocation_size; intra_allocation_it += ACCESS_STRIDE) {
                        for (size_t allocation_it=0; allocation_it<nof_allocations; ++allocation_it) {
                            data[allocation_it][intra_allocation_it]++;
                        }
                    }
                }
                break;
            case (TestCase::RANDOM_COPY):
                for (size_t i=0; i<iterations; ++i) {
                    for (size_t intra_allocation_it=0; intra_allocation_it<allocation_size; intra_allocation_it += ACCESS_STRIDE) {
                        for (size_t allocation_it=0; allocation_it<nof_allocations; ++allocation_it) {
                            size_t index_alloc_from = distr_alloc(generator);
                            size_t index_internal_from = distr_internal(generator);
                            size_t index_alloc_to = distr_alloc(generator);
                            size_t index_internal_to = distr_internal(generator);
                            data[index_alloc_to][index_internal_to] =
                            data[index_alloc_from][index_internal_from];
                        }
                    }
                }
                break;
            case (TestCase::SEQ_COPY):
                for (size_t i=0; i<iterations; ++i) {
                    for (size_t intra_allocation_it=0; intra_allocation_it<allocation_size-5; intra_allocation_it += ACCESS_STRIDE) {
                        for (size_t allocation_it=0; allocation_it<nof_allocations; ++allocation_it) {
                            size_t index_alloc_to = allocation_it;
                            size_t index_internal_to = intra_allocation_it+5;
                            size_t index_alloc_from = allocation_it;
                            size_t index_internal_from = intra_allocation_it;
                            data[index_alloc_to][index_internal_to] =
                            data[index_alloc_from][index_internal_from];
                        }
                    }
                }
                break;
        }
        return clock_bench_time_ms() - timestamp_start;
    }
    // clang-format on

public:
    Loader(TestCase test_case, std::shared_ptr<Allocator> allocator,
           size_t nof_allocations, size_t allocation_size, size_t iterations,
           size_t additional_stack)
        : generator(dev()),
          test_case(test_case),
          nof_allocations(nof_allocations),
          allocation_size(allocation_size),
          iterations(iterations),
          allocator(allocator),
          additional_stack(additional_stack)
    {
        data = (volatile uint64_t **)allocator->malloc(sizeof(void *) *
                                                       nof_allocations);
        assert(data && "allocation failed!");
    }

    ~Loader()
    {
        for (size_t i = 0; i < nof_done_allocations; ++i)
            allocator->free((void *)data[i]);
        allocator->free(data);
    }

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

    /// @return true if all allocated
    bool AllocateNext()
    {
        // modify stack size - type control
        volatile uint8_t DATA[16 * additional_stack];
        DATA[0] = 0;
        // stride 16 was necessary - lower changes were not always reflected in
        // stack size TODO investigate and modify stack size hash to avoid
        // unused types
        DATA[16 * additional_stack - 1] = 0;
        // avoid moving reads and writes before and after this fence
        std::atomic_thread_fence(std::memory_order_seq_cst);

        if (nof_done_allocations < nof_allocations) {
            data[nof_done_allocations] = (volatile uint64_t *)allocator->malloc(
                sizeof(uint64_t) * allocation_size);
            assert(data[nof_done_allocations] && "allocation failed!");
            ++nof_done_allocations;
        }

        return nof_done_allocations >= nof_allocations;
    }
};

std::random_device Loader::dev;

class LoaderCreator
{
    TestCase test_case;
    size_t nof_allocations;
    size_t allocation_size;
    size_t additional_stack;
    size_t iterations;
    //     std::shared_ptr<memtier_memory> memory;
    std::shared_ptr<Allocator> allocator;

    std::shared_ptr<Loader> CreateLoader_()
    {
        return std::make_shared<Loader>(test_case, allocator, nof_allocations,
                                        allocation_size, iterations,
                                        additional_stack);
    }

public:
    LoaderCreator(TestCase test_case, size_t nof_allocations,
                  size_t allocation_size, size_t iterations,
                  std::shared_ptr<Allocator> allocator, size_t additional_stack)
        : test_case(test_case),
          nof_allocations(nof_allocations),
          allocation_size(allocation_size),
          iterations(iterations),
          allocator(allocator),
          additional_stack(additional_stack)
    {}

    std::shared_ptr<Loader> CreateLoader()
    {
        // create array on stack and avoid optimising it out
        volatile uint8_t DATA[16 * additional_stack];
        DATA[0] = 0;
        DATA[16 * additional_stack - 1] = 0;
        // avoid moving reads and writes before and after this fence
        std::atomic_thread_fence(std::memory_order_seq_cst);
        return this->CreateLoader_();
    }
};

/// @return malloc time, access time
static std::pair<uint64_t, uint64_t>
create_and_run_loaders(std::vector<LoaderCreator> &creators)
{
    std::vector<std::shared_ptr<Loader>> loaders;

    auto fence = std::make_shared<Fence>(creators.size());
    loaders.reserve(creators.size());

    for (auto &creator : creators) {
        loaders.push_back(creator.CreateLoader());
    }
    uint64_t start_mallocs_timestamp = clock_bench_time_ms();
    bool all_finished = false;
    // allocate so that fields interleave on common allocators
    while (!all_finished) {
        all_finished = true;
        // use creators like
        size_t first = 0;
        size_t last = loaders.size() - 1;
        // make one allocation per each loader
        // order of allocations:
        // hottest -> coldest -> second hottest -> second coldest -> ...
        // made to assure that each pair of allocations has ~ same average place
        // in ranking
        while (first <= last) {
            all_finished &= loaders[first++]->AllocateNext();
            if (first <= last)
                all_finished &= loaders[last--]->AllocateNext();
        }
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
create_loader_creators(TestCase test_case, size_t nof_allocations,
                       size_t allocation_size, size_t types_count,
                       size_t total_iterations, memtier_policy_t policy,
                       std::pair<size_t, size_t> pmem_dram_ratio)
{
    const double s = 2.0;
    std::vector<LoaderCreator> creators;

    std::shared_ptr<Allocator> allocator;
    switch (policy) {
        case MEMTIER_POLICY_STATIC_RATIO:
            allocator =
                std::make_shared<StaticRatioAllocator>(policy, pmem_dram_ratio);
            break;
        default:
            assert(false && "case not handled");
            break;
    }
    creators.reserve(types_count);
    double zipf_denominator = calculate_zipf_denominator(types_count, s);

    // create loader creators that share memory
    double probability_sum = 0;
    // TODO
    for (size_t type_idx = 0, k = 1u; type_idx < types_count; ++type_idx, ++k) {
        double probability = 1 / pow(k, s) / zipf_denominator;
        probability_sum += probability;
        size_t iterations = (size_t)(total_iterations * probability);
        std::cout << "Type " << k << " iterations count: " << iterations
                  << std::endl;
        creators.push_back(LoaderCreator(test_case, nof_allocations,
                                         allocation_size, iterations, allocator,
                                         type_idx));
    }
    assert(abs(probability_sum - 1.0) < 0.0001 &&
           "probabilities do not sum to 100% !");

    return creators;
}

/// @return duration of allocations and accesses in milliseconds
BenchResults run_test(const RunInfo &info)
{

    std::vector<LoaderCreator> loader_creators = create_loader_creators(
        info.test_case, info.nof_allocations, info.allocation_size,
        info.types_count, info.iterations_minor, info.policy,
        info.dram_pmem_ratio);

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
            std::string dram_ratio_part_str =
                ratio_string.substr(0, char_count);
            args->dram_ratio_part =
                std::strtoul(dram_ratio_part_str.c_str(), nullptr, 10);
            ratio_string.erase(0, char_count + delimiter.length());
            std::string pmem_ratio_part_str =
                ratio_string.substr(0, ratio_string.find(delimiter));
            args->pmem_ratio_part =
                std::strtoul(pmem_ratio_part_str.c_str(), nullptr, 10);
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
    size_t ALLOCATION_SIZE_U64 = 1024 / 8 * 3 / 2; // 1.5 kB
    size_t NOF_ALLOCATIONS = 512 * 1024;

    struct BenchArgs args = {.policy = MEMTIER_POLICY_STATIC_RATIO,
                             .test_case = TestCase::SEQ_INC,
                             .test_iterations = 1,
                             .access_iterations = 500,
                             .threads_count = 1,
                             .dram_ratio_part = 1,
                             .pmem_ratio_part = 8};
    argp_parse(&argp, argc, argv, 0, 0, &args);

    RunInfo info = {.test_case = args.test_case,
                    .nof_allocations = NOF_ALLOCATIONS,
                    .allocation_size = ALLOCATION_SIZE_U64,
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
