// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/fast_pool_allocator.h>
#include <memkind/internal/memkind_memtier.h>
#include <memkind/internal/pool_allocator.h>

#include <mtt_allocator.h>

#include <argp.h>
#include <assert.h>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <random>
#include <thread>

#define ACCESS_STRIDE 10
#define USE_DAX_KMEM  1

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

enum class RawAllocator
{
    /// PoolAllocator
    POOL,
    FAST_POOL,
    MTT_ALLOCATOR,
};

struct GeneralPolicy {
    enum class GeneralType
    {
        /// Use memtier_policy_t
        Memtier,
        /// Use RawAllocator
        RawAllocator,
        /// Use standard library allocator: std::malloc and std::free
        StdAllocator
    } generalType;
    union {
        memtier_policy_t policy;
        RawAllocator allocator;
    };
};

enum NonAsciiKeys
{
    KEY_NOF_ALLOCATIONS = 0x100,
    KEY_ALLOCATION_SIZE = 0x101,
    KEY_LOW_LIMIT = 0x102,
    KEY_SOFT_LIMIT = 0x103,
    KEY_HARD_LIMIT = 0x104,
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

class StdAllocator: public Allocator
{
public:
    void *malloc(size_t size_bytes)
    {
        return std::malloc(size_bytes);
    }

    void free(void *ptr)
    {
        return std::free(ptr);
    }
};

class MemtierAllocator: public Allocator
{
    std::shared_ptr<memtier_memory> memory;

public:
    MemtierAllocator(memtier_policy_t policy,
                     std::pair<size_t, size_t> dram_pmem_ratio)
    {
        struct memtier_builder *m_tier_builder = memtier_builder_new(policy);
        memtier_builder_add_tier(m_tier_builder, MEMKIND_DEFAULT,
                                 dram_pmem_ratio.first);

#if USE_DAX_KMEM
        memtier_builder_add_tier(m_tier_builder, MEMKIND_DAX_KMEM,
                                 dram_pmem_ratio.second);
#else
        std::cerr << "NOTE: second tier is defined as MEMKIND_REGULAR"
                  << std::endl;
        memtier_builder_add_tier(m_tier_builder, MEMKIND_REGULAR,
                                 dram_pmem_ratio.second);
#endif

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

class FastPoolAllocatorWrapper: public Allocator
{
    FastPoolAllocator allocator;

public:
    FastPoolAllocatorWrapper()
    {
        uintptr_t dummy_address = 0ul;
        size_t dummy_nof_pages = 0ul;
        fast_pool_allocator_create(&allocator, &dummy_address, &dummy_nof_pages,
                                   &gStandardMmapCallback);
    }

    ~FastPoolAllocatorWrapper()
    {
        fast_pool_allocator_destroy(&allocator);
    }

    void *malloc(size_t size_bytes)
    {
        return fast_pool_allocator_malloc(&allocator, size_bytes);
    }

    void free(void *ptr)
    {
        fast_pool_allocator_free(&allocator, ptr);
    }
};

class MTTAllocatorWrapper: public Allocator
{
    MTTAllocator allocator;

public:
    MTTAllocatorWrapper(const MTTInternalsLimits &limits)
    {
        mtt_allocator_create(&allocator, &limits);
    }

    ~MTTAllocatorWrapper()
    {
        mtt_allocator_destroy(&allocator);
    }

    void *malloc(size_t size_bytes)
    {
        return mtt_allocator_malloc(&allocator, size_bytes);
    }

    void free(void *ptr)
    {
        mtt_allocator_free(&allocator, ptr);
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
    GeneralPolicy policy;
    std::pair<size_t, size_t> dram_pmem_ratio;
    MTTInternalsLimits limits;
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
static std::vector<LoaderCreator> create_loader_creators(
    TestCase test_case, size_t nof_allocations, size_t allocation_size,
    size_t types_count, size_t total_iterations, GeneralPolicy policy,
    std::pair<size_t, size_t> pmem_dram_ratio, const MTTInternalsLimits *limits)
{
    const double s = 2.0;
    std::vector<LoaderCreator> creators;

    std::shared_ptr<Allocator> allocator;
    switch (policy.generalType) {
        case GeneralPolicy::GeneralType::Memtier:
        {
            allocator = std::make_shared<MemtierAllocator>(policy.policy,
                                                           pmem_dram_ratio);
            break;
        }
        case GeneralPolicy::GeneralType::StdAllocator:
        {
            allocator = std::make_shared<StdAllocator>();
            break;
        }
        case GeneralPolicy::GeneralType::RawAllocator:
        {
            switch (policy.allocator) {
                case RawAllocator::POOL:
                    allocator = std::make_shared<PoolAllocatorWrapper>();
                    break;
                case RawAllocator::FAST_POOL:
                    allocator = std::make_shared<FastPoolAllocatorWrapper>();
                    break;
                case RawAllocator::MTT_ALLOCATOR:
                    assert(limits && limits->lowLimit && limits->softLimit &&
                           limits->hardLimit && "limits not initialized!");
                    allocator = std::make_shared<MTTAllocatorWrapper>(*limits);
                    break;
            }
        }
    }
    creators.reserve(types_count);
    double zipf_denominator = calculate_zipf_denominator(types_count, s);

    // create loader creators that share memory
    double probability_sum = 0;
    // k reverses - coldest data is allocated first
    // the order of allocs and it's effect might be investigated in the future
    for (size_t type_idx = 0, k = types_count; type_idx < types_count;
         ++type_idx, --k) {
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
        info.dram_pmem_ratio, &info.limits);

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
    GeneralPolicy policy;
    TestCase test_case;
    size_t test_iterations;
    size_t access_iterations;
    size_t threads_count;
    size_t dram_ratio_part;
    size_t pmem_ratio_part;
    size_t nof_allocations;
    size_t allocation_size;
    MTTInternalsLimits limits;
};

static int parse_opt(int key, char *arg, struct argp_state *state)
{
    auto args = (BenchArgs *)state->input;
    switch (key) {
        case 's':
            args->policy.generalType = GeneralPolicy::GeneralType::Memtier;
            args->policy.policy = MEMTIER_POLICY_STATIC_RATIO;
            break;
        case 'm':
            args->policy.generalType = GeneralPolicy::GeneralType::RawAllocator;
            args->policy.allocator = RawAllocator::MTT_ALLOCATOR;
            break;
        case 'p':
            args->policy.generalType = GeneralPolicy::GeneralType::RawAllocator;
            args->policy.allocator = RawAllocator::POOL;
            break;
        case 'f':
            args->policy.generalType = GeneralPolicy::GeneralType::RawAllocator;
            args->policy.allocator = RawAllocator::FAST_POOL;
            break;
        case 'g':
            args->policy.generalType = GeneralPolicy::GeneralType::StdAllocator;
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
            assert(arg && "argument for test iterations not supplied");
            args->test_iterations = std::strtoul(arg, nullptr, 10);
            assert(args->test_iterations > 0 &&
                   "test iterations must be greater than zero");
            break;
        case 'a':
            assert(arg && "argument for access iterations not supplied");
            args->access_iterations = std::strtoul(arg, nullptr, 10);
            assert(args->access_iterations > 0 &&
                   "access iterations must be greater than zero");
            break;
        case 't':
            assert(arg && "argument for threads not supplied");
            args->threads_count = std::strtoul(arg, nullptr, 10);
            assert(args->threads_count > 0 &&
                   "at least one thread is required");
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
        case KEY_NOF_ALLOCATIONS:
        {
            assert(arg &&
                   "argument for the number of allocations not supplied");
            args->nof_allocations = std::strtoul(arg, nullptr, 10);
            assert(args->nof_allocations > 0 &&
                   "at least one allocation per type is required");
            break;
        }
        case KEY_ALLOCATION_SIZE:
        {
            assert(arg && "argument for allocation size not supplied");
            args->allocation_size = std::strtoul(arg, nullptr, 10);
            assert(args->allocation_size > 0 &&
                   "allocation size has to be > 0");
            break;
        }
        case KEY_LOW_LIMIT:
        {
            assert(arg && "argument for allocation size not supplied");
            args->limits.lowLimit = std::strtoul(arg, nullptr, 10);
            assert(args->limits.lowLimit > 0 && "limit has to be > 0");
            break;
        }
        case KEY_SOFT_LIMIT:
        {
            assert(arg && "argument for allocation size not supplied");
            args->limits.softLimit = std::strtoul(arg, nullptr, 10);
            assert(args->limits.softLimit > 0 && "limit has to be > 0");
            break;
        }
        case KEY_HARD_LIMIT:
        {
            assert(arg && "argument for allocation size not supplied");
            args->limits.hardLimit = std::strtoul(arg, nullptr, 10);
            assert(args->limits.hardLimit > 0 && "limit has to be > 0");
            break;
        }
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp_option options[] = {
    {"static", 's', 0, 0, "Benchmark static ratio policy.", 0},
    {"pool", 'p', 0, 0, "Benchmark pool allocator.", 0},
    {"fast_pool", 'f', 0, 0, "Benchmark fast pool allocator.", 0},
    {"movement", 'm', 0, 0, "Benchmark data movement policy.", 0},
    {"low-limit", KEY_LOW_LIMIT, "size_t", 0,
     "value below which movement from PMEM -> DRAM occurs, must be a multiple of TRACED_PAGESIZE",
     4},
    {"soft-limit", KEY_SOFT_LIMIT, "size_t", 0,
     "soft limit of used dram, data movement DRAM -> PMEM will occur when surpassed, must be a multiple of TRACED_PAGESIZE",
     4},
    {"hard-limit", KEY_HARD_LIMIT, "size_t", 0,
     "hard limit of used dram, this limit will not be surpassed by the allocator TODO @warning not implemented, must be a multiple of TRACED_PAGESIZE",
     4},
    {"std", 'g', 0, 0, "Benchmark std glibc allocator.", 0},
    {0, 'A', 0, 0, "Benchmark test case Random Incrementation.", 1},
    {0, 'B', 0, 0, "Benchmark test case Sequential Incrementation.", 1},
    {0, 'C', 0, 0, "Benchmark test case Random Copy.", 1},
    {0, 'D', 0, 0, "Benchmark test case Sequential Copy.", 1},
    {"iterations", 'i', "size_t", 0, "Benchmark iterations.", 2},
    {"access", 'a', "size_t", 0, "Number of accesses done for each thread.", 2},
    {"threads", 't', "size_t", 0, "Threads count.", 2},
    {"ratio", 'r', "dram:pmem", 0, "DRAM to PMEM ratio.", 2},
    {"nof_allocs", KEY_NOF_ALLOCATIONS, "size_t", 0,
     "Number of allocations per type", 3},
    {"alloc_size", KEY_ALLOCATION_SIZE, "size_t", 0,
     "Size of each allocation - size of uint64_t buffer", 3},
    {0}};

static struct argp argp = {options, parse_opt, nullptr, nullptr};

int main(int argc, char *argv[])
{
    // buffer with 1.5*128 uint64_t entries - > 1.5 kB
    size_t ALLOCATION_SIZE_U64 = 1024 / sizeof(uint64_t) * 3 / 2;
    size_t NOF_ALLOCATIONS = 512 * 1024;

    struct BenchArgs args;

    args.policy.generalType = GeneralPolicy::GeneralType::Memtier;
    args.policy.policy = MEMTIER_POLICY_STATIC_RATIO;
    args.test_case = TestCase::SEQ_INC;
    args.test_iterations = 1;
    args.access_iterations = 500;
    args.threads_count = 1;
    args.dram_ratio_part = 1;
    args.pmem_ratio_part = 8;
    args.nof_allocations = NOF_ALLOCATIONS;
    args.allocation_size = ALLOCATION_SIZE_U64;
    args.limits.lowLimit = 0ul;
    args.limits.softLimit = 0ul;
    args.limits.hardLimit = 0ul;

    argp_parse(&argp, argc, argv, 0, 0, &args);

    RunInfo info = {
        .test_case = args.test_case,
        .nof_allocations = args.nof_allocations,
        .allocation_size = args.allocation_size,
        .types_count = args.threads_count,
        .iterations_major = args.test_iterations,
        .iterations_minor = args.access_iterations,
        .policy = args.policy,
        .dram_pmem_ratio =
            std::make_pair(args.dram_ratio_part, args.pmem_ratio_part),
        .limits = args.limits,
    };

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
