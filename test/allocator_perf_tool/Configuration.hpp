// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <assert.h>
#include <map>
#include <string>

// AllocatorTypes class represent allocator types and names related to this
// types.
class AllocatorTypes
{
public:
    enum
    {
        STANDARD_ALLOCATOR,
        JEMALLOC,
        MEMKIND_DEFAULT,
        MEMKIND_REGULAR,
        MEMKIND_HBW,
        MEMKIND_INTERLEAVE,
        MEMKIND_HBW_INTERLEAVE,
        MEMKIND_HBW_PREFERRED,
        MEMKIND_HUGETLB,
        MEMKIND_GBTLB,
        MEMKIND_HBW_HUGETLB,
        MEMKIND_HBW_PREFERRED_HUGETLB,
        MEMKIND_HBW_GBTLB,
        MEMKIND_HBW_PREFERRED_GBTLB,
        HBWMALLOC_ALLOCATOR,
        MEMKIND_PMEM,
        MEMKIND_DAX_KMEM,
        NUM_OF_ALLOCATOR_TYPES
    };

    static const std::string &allocator_name(unsigned type)
    {
        static const std::string names[] = {"STANDARD_ALLOCATOR",
                                            "JEMALLOC",
                                            "MEMKIND_DEFAULT",
                                            "MEMKIND_REGULAR",
                                            "MEMKIND_HBW",
                                            "MEMKIND_INTERLEAVE",
                                            "MEMKIND_HBW_INTERLEAVE",
                                            "MEMKIND_HBW_PREFERRED",
                                            "MEMKIND_HUGETLB",
                                            "MEMKIND_GBTLB",
                                            "MEMKIND_HBW_HUGETLB",
                                            "MEMKIND_HBW_PREFERRED_HUGETLB",
                                            "MEMKIND_HBW_GBTLB",
                                            "MEMKIND_HBW_PREFERRED_GBTLB",
                                            "HBWMALLOC_ALLOCATOR",
                                            "MEMKIND_PMEM",
                                            "DAX_KMEM"};

        if (type >= NUM_OF_ALLOCATOR_TYPES)
            assert(!"Invalid input argument!");

        return names[type];
    }

    static unsigned allocator_type(const std::string &name)
    {
        for (unsigned i = 0; i < NUM_OF_ALLOCATOR_TYPES; i++) {
            if (allocator_name(i) == name)
                return i;
        }

        assert(!"Invalid input argument!");
    }

    static bool is_valid_memkind(unsigned type)
    {
        return (type >= MEMKIND_DEFAULT) && (type < NUM_OF_ALLOCATOR_TYPES);
    }
};

// Enable or disable enum values (types).
class TypesConf
{
public:
    TypesConf()
    {}

    TypesConf(unsigned type)
    {
        enable_type(type);
    }

    void enable_type(unsigned type)
    {
        types[type] = true;
    }

    void disable_type(unsigned type)
    {
        if (types.count(type))
            types[type] = false;
    }

    bool is_enabled(unsigned type) const
    {
        return (types.count(type) ? types.find(type)->second : false);
    }

private:
    std::map<unsigned, bool> types;
};

// AllocationSizesConf class represents allocation sizes configuration.
// This data is needed to generate "n" sizes in range from "size_from" to
// "size_to".
class AllocationSizesConf
{
public:
    unsigned n;
    size_t size_from;
    size_t size_to;
};

// TaskConf class contain configuration data for task,
// where:
// - "n" - number of iterations,
// - "allocation_sizes_conf" - allocation sizes configuration,
// - "func_calls" - enabled or disabled function calls,
// - "allocators_types" - enable allocators,
// - "seed" - random seed.
// - "touch_memory" - enable or disable touching memory
class TaskConf
{
public:
    unsigned n;
    AllocationSizesConf allocation_sizes_conf;
    TypesConf func_calls;
    TypesConf allocators_types;
    unsigned seed;
    bool touch_memory;
    bool is_csv_log_enabled;
};
