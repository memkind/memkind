/*
 * Copyright (C) 2014, 2015 Intel Corporation.
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

#include <vector>
#include <fstream>
#include <pthread.h>
#include <unistd.h>
#include <regex>
#include <cmath>
#include <numa.h>
#include <numaif.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "common.h"
#include "omp.h"
#include "memkind.h"

#define NUM_THREADS 4
#define NUM_ITERATIONS 2

// Maximum amount of memory a Bind thread will allocate
static size_t max_size_g;

using namespace std;

// Tread local random generator, always the same sequence for all threads
thread_local std::mt19937 rand_init(0);
thread_local std::mt19937 rand_mt(rand_init());

// To work around cout interleave issue.
typedef struct ss_printer {
    bool clear;
} ss_printer;

static const ss_printer ss_cout = {true};
static const ss_printer ss_cout_nc = {false};

typedef struct fulfill_and_free_args {
    size_t num_bandwidth;
    int *bandwidth;
    pthread_barrier_t *barr;
    int (*create_allocations)(vector<void *> *, long long int *, size_t, int*);
} fulfill_and_free_args;

// Prints a stringstream and also clears it if p.clear is true.
ostream &operator<<(ostream &os, const ss_printer &p)
{
    cout << dynamic_cast<stringstream &>(os).str();
    if (p.clear)
        dynamic_cast<stringstream &>(os).str("");
    return os;
}

// Get the NUMA node index where <ptr> points to.
int get_numa_node(void *ptr)
{
    int status[1] = {-1};
    void *page = (void *)((uintptr_t)ptr & ~(uintptr_t)(sysconf(_SC_PAGESIZE) - 1));

    move_pages(0, 1, &page, NULL, status, 0);
    return status[0];
}

// Returns true if memory pointed by <ptr> is in a high bandwidth node.
bool is_high_bandwith(void *ptr, size_t num_bandwidth, int *bandwidth)
{
    int max_bandwidth = *max_element(&bandwidth[0], &bandwidth[num_bandwidth]);
    return bandwidth[get_numa_node(ptr)] == max_bandwidth;
}

// Reads /proc/buddyinfo to compare the free space of the highest order of each
// node zone and returns the biggest one. Returns ~0 in error.
size_t biggest_chunks_size(bool hbw, size_t num_bandwidth, int *bandwidth)
{
    ifstream            buddyinfo;
    string              expr = "(Node )(";
    unsigned long int   fragmenst;
    unsigned long int   order;
    size_t              page_size = sysconf(_SC_PAGESIZE);
    size_t              biggest_free = 0;
    size_t              current_free = -1;
    char                line[256];
    char               *last_token = NULL;
    char               *curr_token = NULL;
    char               *saveptr;
    int                 num_tokens = 0;

    // Form a regex to match lines containing the HBW nodes. For example, if The
    // HBW nodes were 0 and 2 the regex will be "(Node )(0|2)(, )(.*)"
    int max_bandwidth = *max_element(&bandwidth[0], &bandwidth[num_bandwidth]);
    for (size_t n = 0; n < num_bandwidth; n++)
        if (hbw == (bandwidth[n] == max_bandwidth)) {
            expr += to_string(n) + "|";
        }
    expr.pop_back();
    expr += ")(, )(.*)";

    buddyinfo.open("/proc/buddyinfo");
    while (!buddyinfo.eof()) {
        buddyinfo.getline(line, 256);
        if (regex_match(line, regex(expr))) {
            curr_token = strtok_r(line, " \t", &saveptr);
            while (curr_token != NULL) {
                num_tokens++;
                last_token = curr_token;
                curr_token = strtok_r(NULL, " \t", &saveptr);
            }
            if (last_token != NULL) {
                fragmenst = strtoul(last_token, NULL, 10);
                order = pow(2, num_tokens - 5);

                current_free = fragmenst * order * page_size;
                if (current_free > biggest_free) {
                    biggest_free = current_free;
                }
            }
        }
    }
    buddyinfo.close();
    return current_free > biggest_free ? -1 : biggest_free;
}

int create_allocations_prefered(
    vector<void *>     *allocations,
    long long int      *total_size,
    size_t              num_bandwidth,
    int                *bandwidth)
{
    stringstream    msg;
    void           *thread_id = (void *)pthread_self();
    void           *mem;
    size_t          size;
    size_t          free_hbw_mem;
    size_t          free_def_mem;
    size_t          dummy_var;

    *total_size = 0;

    while (true) {
        // Try to allocate random sized memory
        size = 2 * MB + rand_mt() % (2 * GB - 2 * MB);
        mem = hbw_malloc(size);

        // Get free available memory
        memkind_get_size(MEMKIND_HBW,     &dummy_var, &free_hbw_mem);
        memkind_get_size(MEMKIND_DEFAULT, &dummy_var, &free_def_mem);

        if (mem == NULL) {
            // Free HBW memory should be lower than <size>
            if (free_hbw_mem >= size) {
                msg << "ThID: " << thread_id << endl
                    << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << endl
                    << "errno " << errno << " " << strerror(errno) << endl
                    << "hbw_malloc(" << size << ") failed!" << endl
                    << "Free high bandwidth: " << free_hbw_mem << " bytes." << endl
                    << ss_cout;
                return -1;
            }

            // Free Regular memory should be lower than <size>
            else if (free_hbw_mem - free_def_mem >= size) {
                msg << "ThID: " << thread_id << endl
                    << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << endl
                    << "errno " << errno << " " << strerror(errno) << endl
                    << "hbw_malloc(" << size << ") failed!" << endl
                    << "Regular free memory: " << free_hbw_mem - free_def_mem << " bytes." << endl
                    << ss_cout;
                return -1;
            }
            else
                break;
        }
        else {
            // Use the memory and save it for later deallocation
            memset(mem, 0x0A, size);
            allocations->push_back(mem);
            *total_size += size;

            // If memory was NOT allocated in a high bandwidth node
            if (!is_high_bandwith(mem, num_bandwidth, bandwidth)) {
                // If free available memory is enough to storage <size>
                if (free_hbw_mem >= size) {
                    // Check fragments of the high-bandwidth nodes. If the sum
                    // of the biggest fragments is enough for <size>. Note that
                    // this fragments are NOT NECESSARILY contiguous.
                    size_t total_biggest_chunks = biggest_chunks_size(true, num_bandwidth, bandwidth);
                    if (total_biggest_chunks >= size) {
                        msg << "ThID: " << thread_id << endl
                            << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << endl
                            << "hbw_malloc(" << size << ") No HBW node" << endl
                            << "High bandwidth free: " << free_hbw_mem << " bytes." << endl
                            << "Biggest free contiguous memory: " << total_biggest_chunks << endl
                            << ss_cout;
                        return -1;
                    }
                    else
                        break;
                }
                else
                    break;
            }
        }
    }

    return 0;
}

int create_allocations_bind(
    vector<void *>     *allocations,
    long long int      *total_size,
    size_t              num_bandwidth,
    int                *bandwidth)
{
    stringstream    msg;
    bool            done  = false;
    void           *thread_id = (void *)pthread_self();
    void           *mem;
    size_t          size;
    size_t          free_hbw_mem;
    size_t          dummy_var;

    *total_size = 0;

    while (!done) {
        // Try to allocate random sized memory
        size = 2 * MB + rand_mt() % (2 * GB - 2 * MB);
        if (*total_size + size > max_size_g) {
            size = max_size_g - *total_size;
            done = true;
        }

        mem = hbw_malloc(size);

        // Get free available memory
        memkind_get_size(MEMKIND_HBW, &dummy_var, &free_hbw_mem);

        if (mem == NULL) {
            // Free HBW memory should be lower than <size>
            if (free_hbw_mem >= size) {
                msg << "ThID: " << thread_id << endl
                    << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << endl
                    << "errno " << errno << " " << strerror(errno) << endl
                    << "hbw_malloc(" << size << ") failed!" << endl
                    << "High bandwidth free: " << free_hbw_mem << " bytes." << endl
                    << ss_cout;
                return -1;
            }
            else
                break;
        }
        else {
            // Use the memory and save it for later deallocation
            memset(mem, 0x0A, size);
            allocations->push_back(mem);
            *total_size += size;

            // If memory was NOT allocated in a high bandwidth node
            if (!is_high_bandwith(mem, num_bandwidth, bandwidth)) {
                msg << "ThID: " << thread_id << endl
                    << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << endl
                    << "hbw_malloc(" << size << ") No HBW node" << endl
                    << ss_cout;
                return -1;
            }
        }
    }

    return 0;
}

void free_allocations(vector<void *> *allocations)
{
    for (auto it = allocations->begin(); it != allocations->end(); it++) {
        hbw_free(*it);
    }
    allocations->clear();
}

void *fulfill_and_free_function(void *_args)
{
    fulfill_and_free_args *args = (fulfill_and_free_args *)_args;
    long long int allocated_size;
    void *thread_id = (void *)pthread_self();
    bool failed = false;
    int result;
    int i;
    vector<void *> allocations;
    stringstream msg;

    msg << "ThID: " << thread_id << " initiated." << endl << ss_cout;

    for (i = 0; i < NUM_ITERATIONS; i++) {
        pthread_barrier_wait(args->barr);
        result = args->create_allocations(&allocations, &allocated_size, args->num_bandwidth, args->bandwidth);

        if (0 != result) {
            msg << "ThID: " << thread_id << endl
                << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << endl
                << "Test failed at iteration " << i << "." << endl
                << "Allocations " << allocations.size() << ". "
                << "Allocated size: " << allocated_size << endl
                << "Terminating all threads..." << endl
                << ss_cout;
            failed = true;
        }

        pthread_barrier_wait(args->barr);

        free_allocations(&allocations);
    }

    return (void *)(failed ? -1 : (unsigned long int)i);
}

class MultithreadedPolicyTest : public :: testing :: Test
{
protected:
    std::string     test_name;
    size_t          num_bandwidth;
    int            *bandwidth;
    volatile bool   test_done;

    virtual void SetUp();
    virtual void TearDown();

    // Returns if <sub_name> is a sub-string of member <test_name>.
    bool test_name_contains(const char *sub_name);

    void fulfill_and_free_test();
};

void MultithreadedPolicyTest::SetUp()
{
    struct bitmask *hbw_nodes_bm;
    char           *env_var;

    // Get current test name
    test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();

    // Get high bandwidth nodes
    num_bandwidth = 0;
    bandwidth = NULL;
    env_var = getenv("MEMKIND_HBW_NODES");
    if (env_var) {
        hbw_nodes_bm = numa_parse_nodestring(env_var);
        if (!hbw_nodes_bm)
            FAIL() << "Cannot parse MEMKIND_HBW_NODES=" << env_var;
        else {
            num_bandwidth = numa_num_task_nodes();
            bandwidth = new int[num_bandwidth];
            for (size_t node = 0; node < num_bandwidth; node++)
                if (numa_bitmask_isbitset(hbw_nodes_bm, node))
                    bandwidth[node] = 2;
                else
                    bandwidth[node] = 1;
            numa_bitmask_free(hbw_nodes_bm);
        }
    }
    else {
        const char *node_bandwidth_path = "/etc/memkind/node-bandwidth";
        ifstream nbw_file;

        nbw_file.open(node_bandwidth_path, ifstream::binary);
        if (nbw_file.good()) {
            nbw_file.seekg(0, nbw_file.end);
            num_bandwidth = nbw_file.tellg() / sizeof(int);
            nbw_file.seekg(0, nbw_file.beg);
            bandwidth = new int[num_bandwidth];
            nbw_file.read((char *)bandwidth, num_bandwidth * sizeof(int));
            nbw_file.close();
        }
        else {
            FAIL() << "Error opening " << node_bandwidth_path << "." << endl
                   << "Has the service started?";
        }
    }
}

void MultithreadedPolicyTest::TearDown()
{
    delete[] bandwidth;
}

// Returns if <sub_name> is a sub-string of member <test_name>.
bool MultithreadedPolicyTest::test_name_contains(const char *sub_name)
{
    return string::npos != test_name.find(sub_name);
}

void MultithreadedPolicyTest::fulfill_and_free_test()
{
    // Fork to call hbw_set_policy() more than once in the same test-case.
    pid_t pid = fork();
    if (pid == -1) {
        FAIL() << "Could not fork()";
    }
    else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        ASSERT_EQ(EXIT_SUCCESS, status) << "Child process failed!";
    }
    else {
        fulfill_and_free_args args;
        pthread_t posix_threads[NUM_THREADS];
        pthread_barrier_t posix_barrier;
        long int ret_val = 0;
        stringstream msg;

        if (test_name_contains("preferred")) {
            args.create_allocations = create_allocations_prefered;
            hbw_set_policy(HBW_POLICY_PREFERRED);
        }
        else if (test_name_contains("bind")) {
            // Set maximum amount of memory a thread will allocate to:
            // (total free memory + 5%) / NUM_THREADS. Swap partition is needed.
            size_t free_size;
            size_t dummy_var;
            memkind_get_size(MEMKIND_HBW, &dummy_var, &free_size);
            max_size_g = (free_size + (0.05L * free_size)) / NUM_THREADS;
            args.create_allocations = create_allocations_bind;
            hbw_set_policy(HBW_POLICY_BIND);
        }
        else
            EXPECT_TRUE(false) << "Could not find test to run!";

        args.barr = &posix_barrier;
        args.num_bandwidth = num_bandwidth;
        args.bandwidth = bandwidth;

        pthread_barrier_init(&posix_barrier, NULL, NUM_THREADS);
        for (unsigned int t = 0; t < NUM_THREADS; t++)
            pthread_create(&posix_threads[t], NULL, fulfill_and_free_function, (void *)&args);

        msg << "Threads started." << endl << ss_cout;

        for (unsigned int t = 0; t < NUM_THREADS; t++) {
            pthread_join(posix_threads[t], (void **)&ret_val);

            if (-1 != ret_val)
                msg << "Thread " << t << " iterations : "
                    << ret_val << endl
                    << ss_cout;

            EXPECT_NE(-1, ret_val) << "Thread " << t << " failed!";
        }

        pthread_barrier_destroy(&posix_barrier);

        if (HasFailure())
            exit(EXIT_FAILURE);
        else
            exit(EXIT_SUCCESS);
    }
}

TEST_F(MultithreadedPolicyTest, fulfill_and_free_preferred)
{
    fulfill_and_free_test();
}

TEST_F(MultithreadedPolicyTest, fulfill_and_free_bind)
{
    fulfill_and_free_test();
}
