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
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class BaseOpsGenerator
{
public:

    virtual vector<vector<Operation*>> singleOpSingleIterPerfTest() = 0;

    virtual vector<vector<Operation*>> manyOpsSingleIterPerfTest() = 0;

    virtual vector<vector<Operation*>> singleOpManyItersPerfTest() = 0;

    virtual vector<vector<Operation*>> manyOpsManyItersPerfTest() = 0;

    virtual Operation* freeOperation() = 0;

    virtual ~BaseOpsGenerator()
    {
    }

};

template <class T>
class OpsGenerator: public BaseOpsGenerator
{
    public:

        Operation* freeOperation()
        {
            return new T(OperationName::Free);
        }

        // Use malloc operation only
        virtual vector<vector<Operation*>> singleOpSingleIterPerfTest() override
        {

            return { { new T(OperationName::Malloc) } };

        }

        // Use all allocation operations, same set for all threads
        virtual vector<vector<Operation*>> manyOpsSingleIterPerfTest() override
        {
            return {
                {
                new T(OperationName::Malloc, 20),
                new T(OperationName::Calloc, 40),
                new T(OperationName::Realloc, 60),
                new T(OperationName::Align, 80)
                }
            };

        }

        // Use one allocation operation, same for all threads, different op in each iteration
        virtual vector<vector<Operation*>> singleOpManyItersPerfTest() override
        {
            return {
                { new T(OperationName::Malloc, 20) },
                { new T(OperationName::Calloc, 40) },
                { new T(OperationName::Realloc, 60) },
                { new T(OperationName::Align, 80) }
            };

        }

            // Use set of all allocation operation, same for all threads, different set in each iteration
        virtual vector<vector<Operation*>> manyOpsManyItersPerfTest() override
        {
            return {
                {
                    new T(OperationName::Malloc, 20),
                    new T(OperationName::Calloc, 40),
                    new T(OperationName::Realloc, 60),
                    new T(OperationName::Align, 80)
                },
                {
                    new T(OperationName::Malloc, 50),
                    new T(OperationName::Calloc, 70),
                    new T(OperationName::Realloc, 80),
                    new T(OperationName::Align, 90)
                },
                {
                    new T(OperationName::Calloc, 50),
                    new T(OperationName::Malloc, 60),
                    new T(OperationName::Realloc, 75),
                    new T(OperationName::Align, 85)
                },
                {
                    new T(OperationName::Realloc, 60),
                    new T(OperationName::Malloc, 80),
                    new T(OperationName::Calloc, 90),
                    new T(OperationName::Align, 95)
                },
                {
                    new T(OperationName::Realloc, 40),
                    new T(OperationName::Malloc, 55),
                    new T(OperationName::Calloc, 70),
                    new T(OperationName::Align, 90)
                }
            };
        }
    };

int main(int argc, char* argv[])
{
    unsigned seed = 1297654;
    int repeats;
    int threads;
    long operations;
    vector<size_t> sizes = { 128 };
    string library;
    int suite;
    PerformanceTest *test;
    BaseOpsGenerator *opsGenerator;
    vector<vector<Operation*>> testOperations;
    Operation *freeOperation;
    string filename = "";
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("repeats,r", po::value<int>(&repeats)->default_value(5), "number of repeats (default=5)")
        ("threads,t", po::value<int>(&threads)->default_value(8), "number of threads (default=8)")
        ("operations,o", po::value<long>(&operations)->default_value(1000000), "number of operations (default=1 000 000)")
        ("library,l", po::value<string>(&library)->default_value("malloc"), "memory allocation library (malloc|jemalloc|jemkmalloc|memkind) (default=malloc)")
        ("sizes,a", po::value< vector<size_t> >(&sizes), "allocation size (default = 128) - provide multiple to generate list")
        ("suite,s",
        po::value<int>(&suite)->default_value(-1),
        "test suite: \n0 - singleOpSingleIter (malloc() only in each thread, 1 iteration)\n1 - manyOpsSingleIter"
        "\n2 - manyOpsSingleIterHugeAlloc\n3 - singleOpManyIters\n4 - manyOpsManyItersPerf")
        ("file,f", po::value<string>(&filename), "ouput metrics file name");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << endl;
        return 1;
    }

    if (library == "malloc")
    {
        opsGenerator = new OpsGenerator<MallocOperation>();
    }
    else if (library == "jemalloc")
    {
        opsGenerator = new OpsGenerator<JemallocOperation>();
    }
    else if (library == "jemkmalloc")
    {
        opsGenerator = new OpsGenerator<JemkmallocOperation>();
    }
    else if (library == "memkind")
    {
        opsGenerator = new OpsGenerator<MemkindOperation>();
    }
    else
    {
        cout << "ERROR: unknown memory allocator library: " << library << endl;
        return 1;
    }
    cout << library << " memory allocator selected." << endl;
    cout << "Results will be stored in ";
    if (filename != "")
    {
        cout << "\"" << filename << "\"";
    }
    else
    {
        cout << "default";

    }
    cout << " file" << endl;
    test = new PerformanceTest(repeats, threads, operations / threads);
    string suiteName;
    switch (suite)
    {
    case 0:
        testOperations = opsGenerator->singleOpSingleIterPerfTest();
        suiteName = "singleOpSingleIterPerfTest";
        break;
    case 1:
        testOperations = opsGenerator->manyOpsSingleIterPerfTest();
        suiteName = "manyOpsSingleIterPerfTest";
        break;
    case 2:
        testOperations = opsGenerator->singleOpManyItersPerfTest();
        suiteName = "singleOpManyItersPerfTest";
        test->setExecutionMode(ExecutionMode::ManyIterations);
        break;
    case 3:
        testOperations = opsGenerator->manyOpsManyItersPerfTest();
        suiteName = "manyOpsManyItersPerfTest";
        test->setExecutionMode(ExecutionMode::ManyIterations);
         break;
    default:
        cout << "ERROR: test suite unknown / not provided: " << suite << endl;
        return -1;
        break;
    }

    freeOperation = opsGenerator->freeOperation();
    test->setOperations(testOperations, freeOperation);
    test->setAllocationSizes(sizes);
    srand(seed);
    test->setKind({ MEMKIND_DEFAULT });
    test->showInfo();
    test->run();
    test->writeMetrics(library.c_str(), suiteName.c_str(), filename);

    delete test;
    delete opsGenerator;

    for (vector<Operation *> ops : testOperations)
    {
        for (Operation * op : ops)
        {
            delete op;
        }
    }
    delete freeOperation;

    return 0;
}
