// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2020 Intel Corporation. */
#include "allocator_perf_tool/HugePageOrganizer.hpp"

#include <dlfcn.h>

#include "common.h"

class DlopenTest: public ::testing::Test
{
protected:
    DlopenTest()
    {
        const char *path = "libmemkind.so";

        dlerror();
        handle = dlopen(path, RTLD_LAZY);
        assert((handle != NULL && dlerror() == NULL) &&
               "Couldn't open libmemkind.so");
        memkind_malloc = (memkind_malloc_t)dlsym(handle, "memkind_malloc");
        assert(dlerror() == NULL &&
               "Couldn't get memkind_malloc from memkind library");
        memkind_free = (memkind_free_t)dlsym(handle, "memkind_free");
        assert(dlerror() == NULL &&
               "Couldn't get memkind_free from memkind library");
    }

    ~DlopenTest()
    {
        dlclose(handle);
    }

    void test(const char *kind_name, size_t alloc_size)
    {
        void **kind_ptr = (void **)dlsym(handle, kind_name);
        EXPECT_TRUE(dlerror() == NULL)
            << "Couldn't get kind from memkind library";
        EXPECT_TRUE(kind_ptr != NULL) << "Kind ptr to memkind library is NULL";

        void *allocation_ptr = memkind_malloc((*kind_ptr), alloc_size);
        EXPECT_TRUE(allocation_ptr != NULL)
            << "Allocation with memkind_malloc failed";

        memset(allocation_ptr, 0, alloc_size);

        memkind_free((*kind_ptr), allocation_ptr);
    }

    bool pathExists(const char *p)
    {
        struct stat info;
        if (0 != stat(p, &info)) {
            return false;
        }
        return true;
    }

private:
    void *handle;
    typedef void *(*memkind_malloc_t)(void *, size_t);
    typedef void (*memkind_free_t)(void *, void *);
    memkind_malloc_t memkind_malloc;
    memkind_free_t memkind_free;
};

TEST_F(DlopenTest, test_TC_MEMKIND_DEFAULT_4194305_bytes)
{
    test("MEMKIND_DEFAULT", 4194305);
}

TEST_F(DlopenTest, test_TC_MEMKIND_HBW_4194305_bytes)
{
    test("MEMKIND_HBW", 4194305);
}

TEST_F(DlopenTest, test_TC_MEMKIND_HBW_HUGETLB_4194305_bytes)
{
    HugePageOrganizer huge_page_organizer(8);
    test("MEMKIND_HBW_HUGETLB", 4194305);
}

TEST_F(DlopenTest, test_TC_MEMKIND_HBW_PREFERRED_4194305_bytes)
{
    test("MEMKIND_HBW_PREFERRED", 4194305);
}

TEST_F(DlopenTest, test_TC_MEMKIND_HBW_INTERLEAVE_4194305_bytes)
{
    test("MEMKIND_HBW_INTERLEAVE", 4194305);
}
