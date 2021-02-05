// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "memkind_allocated.hpp"
#include <memkind.h>

// This code is example usage of C++11 features with custom allocator,
// support for C++11 is required.
#include <iostream>

#if __cplusplus > 199711L

#include <cstdlib>
#include <new>
#include <string>

// example class definition, which derive from  memkind_allocated template to
// have objects allocated with memkind, and have alignment specified by
// alignas()
class alignas(128) memkind_allocated_example
    : public memkind_allocated<memkind_allocated_example>
{
    std::string message;

public:
    // Override method for returning class default kind to make objects be by
    // default allocated on High-Bandwidth Memory
    static memkind_t getClassKind()
    {
        return MEMKIND_HBW;
    }

    memkind_allocated_example(std::string my_message)
    {
        this->message = my_message;
    }

    memkind_allocated_example()
    {}

    void print_message()
    {
        std::cout << message << std::endl;
        std::cout << "Memory address of this object is: " << (void *)this
                  << std::endl
                  << std::endl;
    }
};

int main()
{
    memkind_t specified_kind = MEMKIND_HBW_HUGETLB;

    memkind_allocated_example *default_kind_example =
        new memkind_allocated_example(std::string(
            "This object has been allocated using class default kind, which is: MEMKIND_DEFAULT"));
    default_kind_example->print_message();
    delete default_kind_example;

    memkind_allocated_example *specified_kind_example =
        new (specified_kind) memkind_allocated_example(std::string(
            "This object has been allocated using specified kind, which is: MEMKIND_HBW_HUGETLB"));
    specified_kind_example->print_message();
    delete specified_kind_example;

    // examples for using same approach for allocating arrays of objects, note
    // that objects created that way can be initialized only with default
    // (unparameterized) constructor
    memkind_allocated_example *default_kind_array_example =
        new memkind_allocated_example[5]();
    delete[] default_kind_array_example;

    memkind_allocated_example *specified_kind_array_example =
        new (specified_kind) memkind_allocated_example[5]();
    delete[] specified_kind_array_example;

    return 0;
}
#else // If C++11 is not available - do nothing.
int main()
{
    std::cout
        << "WARNING: because your compiler does not support C++11 standard,"
        << std::endl;
    std::cout << "this example is only as a dummy placeholder." << std::endl;
    return 0;
}
#endif
