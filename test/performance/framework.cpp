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

#include "framework.hpp"

using namespace std;

#ifdef __DEBUG
mutex g_coutMutex;
int g_msgLevel = 1;
#endif

const unsigned Operation::MaxTreshold = 100;
const unsigned Operation::MemalignMaxMultiplier = 4;

void Barrier::wait()
{
    unique_lock<mutex> lock(m_barrierMutex);
    // Decrement number of threads awaited at the barrier
    m_waiting--;
    if (m_waiting == 0)
    {
        // Called by the last expected thread - notify all waiting threads and exit
        m_cVar.notify_all();
        // Store the time when barrier was released
        if (m_releasedAt.tv_sec == 0 && m_releasedAt.tv_nsec == 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &m_releasedAt);
        }
        return;
    }
    // Wait unitl the last expected thread calls wait() on Barrier instance, or timeout occurs
    m_cVar.wait_until(lock, chrono::system_clock::now() + chrono::seconds(10), [](){ return GetInstance().m_waiting == 0;});
}

// Operation class
Operation::
    Operation(
        OperationName name,
    unsigned treshold)
    : m_name(name)
    , m_treshold(treshold)
{
    assert(treshold <= MaxTreshold);
}

Operation::~Operation()
{
}

bool Operation::checkCondition(unsigned treshold) const
{
    return (treshold < m_treshold);
}

const OperationName Operation::getName() const
{
    return m_name;
}

string Operation::getNameStr() const
{
    switch (m_name)
    {
    case OperationName::Malloc:
        return "malloc";

    case OperationName::Calloc:
        return "calloc";

    case OperationName::Realloc:
        return "realloc";

    case OperationName::Align:
        return "align";

    case OperationName::Free:
        return "free";

    default:
        return "<unknown>";
    }
}

unsigned Operation::getTreshold() const
{
    return m_treshold;
}

// Worker class
Worker::Worker(
    uint32_t operationsCount,
    const vector<size_t> &allocationSizes,
    Operation *freeOperation,
    memkind_t kind)
    : m_operationsCount(operationsCount)
    , m_allocationSizes(allocationSizes)
    , m_freeOperation(freeOperation)
    , m_kind(kind)
    , m_allocations(vector<void*>(operationsCount, nullptr))
{
    assert(freeOperation->getName() == OperationName::Free);
}

void Worker::setOperations(const vector<Operation*> &testOperations)
{
    m_testOperations = testOperations;
}

void Worker::run()
{
    m_thread = new thread(&Worker::work, this);
}

#ifdef __DEBUG
uint16_t Worker::getId()
{
    return m_threadId;
}
void Worker::setId(uint16_t threadId)
{
    m_threadId = threadId;
}
#endif

void Worker::finish()
{
    if (m_thread != nullptr)
    {
        m_thread->join();
        delete m_thread;
    }
}

inline size_t Worker::pickAllocationSize(int i)
{
    // get random allocation size
    return (m_allocationSizes.size() > 1 ?
        m_allocationSizes[rand() % m_allocationSizes.size()] :
        m_allocationSizes[0]);
}

inline void * &Worker::pickAllocation(int i)
{
    // get random entry in allocations table
    return m_allocations[rand() % m_operationsCount];
}

void Worker::work()
{
    EMIT(1, "Entering barrier " << m_threadId)
    Barrier::GetInstance().wait();
    EMIT(1, "Starting thread " << m_threadId)
    for(uint32_t i = 0 ; i < m_operationsCount ; i++)
    {
        int treshold = rand() % Operation::MaxTreshold;

        for (const Operation *operation : m_testOperations) //each operation
        {
            if (operation->checkCondition(treshold))
            {
                operation->perform(m_kind, pickAllocation(i), pickAllocationSize(i));
                break;
            }
        }
    }
}

void Worker::clean()
{
    EMIT(2, "Cleaning thread " << m_threadId)
    for(void * &alloc : m_allocations)
    {
        m_freeOperation->perform(m_kind, alloc);
    }
    EMIT(1, "Thread " << m_threadId << " finished")
}

// PerformanceTest class
const string PerformanceTest::m_defaultMetricsFile = "perf_tests.txt";

PerformanceTest::PerformanceTest(
    size_t repeatsCount,
    size_t threadsCount,
    size_t operationsCount)
    : m_repeatsCount(repeatsCount)
    , m_threadsCount(threadsCount)
    , m_operationsCount(operationsCount)
    , m_executionMode(ExecutionMode::SingleInteration)
{
}

void PerformanceTest::setAllocationSizes(const vector<size_t> &allocationSizes)
{
    m_allocationSizes = allocationSizes;
}

void PerformanceTest::setOperations(const vector<vector<Operation*>> &testOperations, Operation *freeOperation)
{
    m_testOperations = testOperations;
    m_freeOperation = freeOperation;
}

void PerformanceTest::setExecutionMode(ExecutionMode executionMode)
{
    m_executionMode = executionMode;
}

void PerformanceTest::setKind(const vector<memkind_t> &kinds)
{
    m_kinds = kinds;
}

inline void PerformanceTest::runIteration()
{
    timespec iterationStop, iterationStart;

    Barrier::GetInstance().reset(m_threadsCount);
    for (Worker * worker : m_workers)
    {
        worker->run();
    }
    for (Worker * worker : m_workers)
    {
        worker->finish();
    }
    EMIT(1, "Alloc completed");
    for (Worker * worker : m_workers)
    {
        worker->clean();
    }
    clock_gettime(CLOCK_MONOTONIC, &iterationStop);
    iterationStart = Barrier::GetInstance().releasedAt();
    m_durations.push_back(
        (iterationStop.tv_sec - iterationStart.tv_sec) * NanoSecInSec +
        (iterationStop.tv_nsec -iterationStart.tv_nsec)
    );
}

void PerformanceTest::prepareWorkers()
{
    for (size_t threadId = 0; threadId < m_threadsCount; threadId++)
    {
        m_workers.push_back(
            new Worker(
            m_operationsCount,
            m_allocationSizes,
            m_freeOperation,
            m_kinds.size() > 0 ? m_kinds[threadId % m_kinds.size()] : nullptr)
        );
#ifdef __DEBUG
        m_workers.back()->setId(threadId);
#endif
        if (m_executionMode == ExecutionMode::SingleInteration)
        {
            // In ManyIterations mode, operations will be set for each thread at the beginning of each iteration
            m_workers.back()->setOperations(m_testOperations[threadId % m_testOperations.size()]);
        }
    }
}

void PerformanceTest::writeMetrics(const string &suiteName, const string &caseName, const string& fileName)
{
    uint64_t totalDuration = 0;

    for (uint64_t& duration : m_durations)
    {
        totalDuration += duration;
    }

    uint64_t executedOperations = m_repeatsCount * m_threadsCount * m_operationsCount;
    double repeatDuration       = (double) totalDuration / ((uint64_t) m_repeatsCount * NanoSecInSec);
    double iterationDuration    = repeatDuration;
    if (m_executionMode == ExecutionMode::ManyIterations)
    {
        executedOperations *= m_testOperations.size();
        iterationDuration  /= m_testOperations.size();
    }
    double operationsPerSecond  = (double) executedOperations * NanoSecInSec / totalDuration;
    double avgOperationDuration = (double) totalDuration / executedOperations;
    assert(iterationDuration != 0.0);

    FILE* f = fopen(fileName != "" ? fileName.c_str() : m_defaultMetricsFile.c_str(), "a+");

    // For thousands separation
    setlocale(LC_ALL, "");
    fprintf(f,
        "%s;%s;%zu;%zu;%f;%f;%f;%f\n",
        suiteName.c_str(),
        caseName.c_str(),
        m_repeatsCount,
        m_threadsCount,
        operationsPerSecond,
        avgOperationDuration,
        iterationDuration,
        repeatDuration);
    fclose(f);
    printf("Operations/sec:\t\t\t%'f\n"
           "Avg. operation duration:\t%f nsec\n"
           "Iteration duration:\t\t%f sec\n"
           "Repeat duration:\t\t%f sec\n",
        operationsPerSecond,
        avgOperationDuration,
        iterationDuration,
        repeatDuration);
}

int PerformanceTest::run()
{
    if (m_testOperations.empty() ||
        m_allocationSizes.empty() ||
        m_freeOperation == nullptr)
    {
        cout << "ERROR: Test not initialized" << endl;
        return 1;
    }
    // Create threads
    prepareWorkers();
    for (size_t repeat = 0; repeat < m_repeatsCount; repeat++)
    {
        EMIT(1, "Test run #" << repeat)
        if (m_executionMode == ExecutionMode::SingleInteration)
        {
            runIteration();
        }
        else
        {
            // Perform each operations list in separate iteration, for each thread
            for (vector<Operation*> & ops : m_testOperations)
            {
                for (Worker * worker : m_workers)
                {
                    worker->setOperations(ops);
                }
                runIteration();
            }
        }
    }
    return 0;
}

void PerformanceTest::showInfo()
{
    printf("Test parameters: %lu repeats, %lu threads, %d operations per thread\n",
        m_repeatsCount,
        m_threadsCount,
        m_operationsCount);
    printf("Thread memory allocation operations:\n");
    for (unsigned long i = 0; i < m_testOperations.size(); i++)
    {
        if (m_executionMode == ExecutionMode::SingleInteration)
        {
            printf("\tThread %lu,%lu,...\n", i, i + (m_testOperations.size()));
        }
        else
        {
            printf("\tIteration %lu\n", i);
        }
        for (const Operation* op : m_testOperations[i])
        {
            printf("\t\t %s (treshold %d)\n", op->getNameStr().c_str(), op->getTreshold());
        }
    }
    printf("Memory free operation:\n\t\t%s\n", m_freeOperation->getNameStr().c_str());
    printf("Allocation sizes:\n");
    for (size_t size : m_allocationSizes)
    {
        printf("\t\t%lu bytes\n", size);
    }
}