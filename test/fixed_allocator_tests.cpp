// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2018 - 2021 Intel Corporation. */
#include "common.h"
#include "memkind.h"
#include "fixed_allocator.h"
#include <algorithm>
#include <array>
#include <list>
#include <map>
#include <memory>
#include <scoped_allocator>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <sys/mman.h>

// Tests for fixed::allocator class.
class FixedAllocatorTests: public ::testing::Test
{
public:
    const size_t fixed_max_size = 1024 * 1024 * 1024;
    // pointer: deafault constructor is deleted,
    // initializer requires mapped area - cannot be provided here
    using int_alloc_ptr = std::shared_ptr<libmemkind::fixed::allocator<int>>;
    int_alloc_ptr alloc_source;
    int_alloc_ptr alloc_conservative;

    int_alloc_ptr alloc_source_f1;
    int_alloc_ptr alloc_source_f2;

protected:
    void SetUp()
    {
        void *addrs[4];
        for (size_t i=0; i<sizeof(addrs)/sizeof(addrs[0]); ++i) {
            addrs[i] = mmap(NULL, fixed_max_size, PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
            assert(addrs[i] != MAP_FAILED);
        }

        alloc_source =
            std::make_shared<libmemkind::fixed::allocator<int>>(addrs[0],
                                                                fixed_max_size);
        alloc_conservative =
            std::make_shared<libmemkind::fixed::allocator<int>>(addrs[1],
                                                                fixed_max_size);

        alloc_source_f1 =
            std::make_shared<libmemkind::fixed::allocator<int>>(addrs[2],
                                                                fixed_max_size);
        alloc_source_f2 =
            std::make_shared<libmemkind::fixed::allocator<int>>(addrs[3],
                                                                fixed_max_size);
    }

    void TearDown()
    {}
};

// Compare two same kind different type allocators
TEST_F(FixedAllocatorTests,
       test_TC_MEMKIND_AllocatorCompare_SameKindDifferentType_Test)
{
    libmemkind::fixed::allocator<char> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<char> alc2{*alloc_source_f1};
    ASSERT_TRUE(alc1 == alc2);
    ASSERT_FALSE(alc1 != alc2);
}

// Compare two same kind same type
TEST_F(FixedAllocatorTests,
       test_TC_MEMKIND_AllocatorCompare_SameKindSameType_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f1};
    ASSERT_TRUE(alc1 == alc2);
    ASSERT_FALSE(alc1 != alc2);
}

// Compare two different kind different type
TEST_F(FixedAllocatorTests,
       test_TC_MEMKIND_AllocatorCompare_DifferentKindDifferentType_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<char> alc2{*alloc_source_f2};
    ASSERT_FALSE(alc1 == alc2);
    ASSERT_TRUE(alc1 != alc2);
}

// Compare two different kind same type allocators
TEST_F(FixedAllocatorTests,
       test_TC_MEMKIND_AllocatorCompare_DifferentKindSameType_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f2};
    ASSERT_FALSE(alc1 == alc2);
    ASSERT_TRUE(alc1 != alc2);
}

// Copy assignment test
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_Allocator_CopyAssignment_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f2};
    ASSERT_TRUE(alc1 != alc2);
    alc1 = alc2;
    ASSERT_TRUE(alc1 == alc2);
}

// Move constructor test
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_Allocator_MoveConstructor_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f2};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f2};
    ASSERT_TRUE(alc2 == alc1);
    libmemkind::fixed::allocator<int> alc3 = std::move(alc1);
    ASSERT_TRUE(alc2 == alc3);
}

// Move assignment test
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_Allocator_MoveAssignment_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f2};
    ASSERT_TRUE(alc1 != alc2);

    {
        libmemkind::fixed::allocator<int> alc3{alc2};
        alc1 = std::move(alc3);
    }
    ASSERT_TRUE(alc1 == alc2);
}

// Single allocation-deallocation test
TEST_F(FixedAllocatorTests,
       test_TC_MEMKIND_Allocator_SingleAllocationDeallocation_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    int *created_object = alc1.allocate(1);
    alc1.deallocate(created_object, 1);
}

// Single allocation-deallocation test conservative policy
TEST_F(FixedAllocatorTests,
       test_TC_MEMKIND_Allocator_SingleAllocationDeallocation_TestConservative)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_conservative};
    int *created_object = alc1.allocate(1);
    alc1.deallocate(created_object, 1);
}

// Shared cross-allocation-deallocation test
TEST_F(FixedAllocatorTests,
       test_TC_MEMKIND_Allocator_SharedAllocationDeallocation_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f1};
    int *created_object = nullptr;
    int *created_object_2 = nullptr;
    created_object = alc1.allocate(1);
    ASSERT_TRUE(alc1 == alc2);
    alc2.deallocate(created_object, 1);
    created_object = alc2.allocate(1);
    alc1.deallocate(created_object, 1);

    created_object = alc1.allocate(1);
    created_object_2 = alc2.allocate(1);
    alc2.deallocate(created_object, 1);
    alc1.deallocate(created_object_2, 1);
}

// Construct-destroy test
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_Allocator_ConstructDestroy_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    int *created_object = alc1.allocate(1);
    alc1.construct(created_object);
    alc1.destroy(created_object);
}

// Testing thread-safety support
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_Allocator_MultithreadingSupport_Test)
{
    const size_t num_threads = 1000;
    const size_t iteration_count = 5000;

    std::vector<std::thread> thds;
    for (size_t i = 0; i < num_threads; ++i) {

        thds.push_back(std::thread([&]() {
            std::vector<libmemkind::fixed::allocator<int>> allocators_local;
            for (size_t j = 0; j < iteration_count; j++) {
                allocators_local.push_back(
                    libmemkind::fixed::allocator<int>(*alloc_source));
            }
            for (size_t j = 0; j < iteration_count; j++) {
                allocators_local.pop_back();
            }
        }));
    }

    for (size_t i = 0; i < num_threads; ++i) {
        thds[i].join();
    }
}

// Test multiple memkind allocator usage
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_MultipleAllocatorUsage_Test)
{

    void *addr = mmap(NULL, fixed_max_size, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    {
        libmemkind::fixed::allocator<int> alloc{addr, fixed_max_size};
    }

    {
        libmemkind::fixed::allocator<int> alloc{addr, fixed_max_size};
    }
    munmap(addr, fixed_max_size);
}

// Test vector
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_AllocatorUsage_Vector_Test)
{
    libmemkind::fixed::allocator<int> alc{*alloc_source};
    std::vector<int, libmemkind::fixed::allocator<int>> vector{alc};

    const int num = 20;

    for (int i = 0; i < num; ++i) {
        vector.push_back(0xDEAD + i);
    }

    for (int i = 0; i < num; ++i) {
        ASSERT_TRUE(vector[i] == 0xDEAD + i);
    }
}

// Test list
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_AllocatorUsage_List_Test)
{
    libmemkind::fixed::allocator<int> alc{*alloc_source};

    std::list<int, libmemkind::fixed::allocator<int>> list{alc};

    const int num = 4;

    for (int i = 0; i < num; ++i) {
        list.emplace_back(0xBEAC011 + i);
        ASSERT_TRUE(list.back() == 0xBEAC011 + i);
    }

    for (int i = 0; i < num; ++i) {
        list.pop_back();
    }
}

// Test map
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_AllocatorUsage_Map_Test)
{
    libmemkind::fixed::allocator<std::pair<const std::string, std::string>> alc{
        *alloc_source};

    std::map<
        std::string, std::string, std::less<std::string>,
        libmemkind::fixed::allocator<std::pair<const std::string, std::string>>>
        map{std::less<std::string>(), alc};

    const int num = 10;

    for (int i = 0; i < num; ++i) {
        map[std::to_string(i)] = std::to_string(0x0CEA11 + i);
        ASSERT_TRUE(map[std::to_string(i)] == std::to_string(0x0CEA11 + i));
        map[std::to_string((i * 997 + 83) % 113)] =
            std::to_string(0x0CEA11 + i);
        ASSERT_TRUE(map[std::to_string((i * 997 + 83) % 113)] ==
                    std::to_string(0x0CEA11 + i));
    }
}

#if _GLIBCXX_USE_CXX11_ABI
// Test vector of strings
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_AllocatorUsage_VectorOfString_Test)
{
    typedef std::basic_string<char, std::char_traits<char>,
                              libmemkind::fixed::allocator<char>>
        pmem_string;
    typedef libmemkind::fixed::allocator<pmem_string> pmem_alloc;

    pmem_alloc alc{*alloc_source};
    libmemkind::fixed::allocator<char> st_alc{alc};

    std::vector<pmem_string, std::scoped_allocator_adaptor<pmem_alloc>> vec{
        alc};
    pmem_string arg{"Very very loooong striiiing", st_alc};

    vec.push_back(arg);
    ASSERT_TRUE(vec.back() == arg);
}

// Test map of int strings
TEST_F(FixedAllocatorTests,
       test_TC_MEMKIND_AllocatorScopedUsage_MapOfIntString_Test)
{
    typedef std::basic_string<char, std::char_traits<char>,
                              libmemkind::fixed::allocator<char>>
        pmem_string;
    typedef int key_t;
    typedef pmem_string value_t;
    typedef std::pair<const key_t, value_t> target_pair;
    typedef libmemkind::fixed::allocator<target_pair> pmem_alloc;
    typedef libmemkind::fixed::allocator<char> str_allocator_t;
    typedef std::map<key_t, value_t, std::less<key_t>,
                     std::scoped_allocator_adaptor<pmem_alloc>>
        map_t;

    pmem_alloc map_allocator(*alloc_source);

    str_allocator_t str_allocator(map_allocator);

    value_t source_str1("Lorem ipsum dolor ", str_allocator);
    value_t source_str2("sit amet consectetuer adipiscing elit", str_allocator);

    map_t target_map{std::scoped_allocator_adaptor<pmem_alloc>(map_allocator)};

    target_map[key_t(165)] = source_str1;
    ASSERT_TRUE(target_map[key_t(165)] == source_str1);
    target_map[key_t(165)] = source_str2;
    ASSERT_TRUE(target_map[key_t(165)] == source_str2);
}
#endif // _GLIBCXX_USE_CXX11_ABI




// Test multiple memkind allocator usage - vector of int
TEST_F(FixedAllocatorTests, test_TC_MEMKIND_MultipleAllocatorUsageVector_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source};
    libmemkind::fixed::allocator<int> alc2{*alloc_source};
    std::vector<int, libmemkind::fixed::allocator<int>> vector1{alc1};
    std::vector<int, libmemkind::fixed::allocator<int>> vector2{alc2};

    const int num = 20;

    for (int i = 0; i < num; ++i) {
        vector1.push_back(0xDEAD + i);
        vector2.push_back(0xFEED + i);
    }

    for (int i = 0; i < num; ++i) {
        ASSERT_TRUE(vector1[i] == 0xDEAD + i);
        ASSERT_TRUE(vector2[i] == 0xFEED + i);
    }
}

// Tests with static arrays

// Tests for fixed::allocator class.
class FixedAllocatorTestsStatic: public ::testing::Test
{
public:
    const static size_t fixed_max_size = 126 * 1024 * 1024;
    // pointer: deafault constructor is deleted,
    // initializer requires mapped area - cannot be provided here
    static uint8_t addrs[fixed_max_size][4];
    using int_alloc_ptr = std::shared_ptr<libmemkind::fixed::allocator<int>>;
    int_alloc_ptr alloc_source;
    int_alloc_ptr alloc_conservative;

    int_alloc_ptr alloc_source_f1;
    int_alloc_ptr alloc_source_f2;

protected:
    void SetUp()
    {
        alloc_source =
            std::make_shared<libmemkind::fixed::allocator<int>>(addrs[0],
                                                                fixed_max_size);
        alloc_conservative =
            std::make_shared<libmemkind::fixed::allocator<int>>(addrs[1],
                                                                fixed_max_size);

        alloc_source_f1 =
            std::make_shared<libmemkind::fixed::allocator<int>>(addrs[2],
                                                                fixed_max_size);
        alloc_source_f2 =
            std::make_shared<libmemkind::fixed::allocator<int>>(addrs[3],
                                                                fixed_max_size);
    }

    void TearDown()
    {}
};

uint8_t FixedAllocatorTestsStatic::addrs[fixed_max_size][4]={0};


// Compare two same kind different type allocators
TEST_F(FixedAllocatorTestsStatic,
       test_TC_MEMKIND_AllocatorCompare_SameKindDifferentType_Test)
{
    libmemkind::fixed::allocator<char> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<char> alc2{*alloc_source_f1};
    ASSERT_TRUE(alc1 == alc2);
    ASSERT_FALSE(alc1 != alc2);
}

// Compare two same kind same type
TEST_F(FixedAllocatorTestsStatic,
       test_TC_MEMKIND_AllocatorCompare_SameKindSameType_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f1};
    ASSERT_TRUE(alc1 == alc2);
    ASSERT_FALSE(alc1 != alc2);
}

// Compare two different kind different type
TEST_F(FixedAllocatorTestsStatic,
       test_TC_MEMKIND_AllocatorCompare_DifferentKindDifferentType_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<char> alc2{*alloc_source_f2};
    ASSERT_FALSE(alc1 == alc2);
    ASSERT_TRUE(alc1 != alc2);
}

// Compare two different kind same type allocators
TEST_F(FixedAllocatorTestsStatic,
       test_TC_MEMKIND_AllocatorCompare_DifferentKindSameType_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f2};
    ASSERT_FALSE(alc1 == alc2);
    ASSERT_TRUE(alc1 != alc2);
}

// Copy assignment test
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_Allocator_CopyAssignment_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f2};
    ASSERT_TRUE(alc1 != alc2);
    alc1 = alc2;
    ASSERT_TRUE(alc1 == alc2);
}

// Move constructor test
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_Allocator_MoveConstructor_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f2};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f2};
    ASSERT_TRUE(alc2 == alc1);
    libmemkind::fixed::allocator<int> alc3 = std::move(alc1);
    ASSERT_TRUE(alc2 == alc3);
}

// Move assignment test
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_Allocator_MoveAssignment_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f2};
    ASSERT_TRUE(alc1 != alc2);

    {
        libmemkind::fixed::allocator<int> alc3{alc2};
        alc1 = std::move(alc3);
    }
    ASSERT_TRUE(alc1 == alc2);
}

// Single allocation-deallocation test
TEST_F(FixedAllocatorTestsStatic,
       test_TC_MEMKIND_Allocator_SingleAllocationDeallocation_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    int *created_object = alc1.allocate(1);
    alc1.deallocate(created_object, 1);
}

// Single allocation-deallocation test conservative policy
TEST_F(FixedAllocatorTestsStatic,
       test_TC_MEMKIND_Allocator_SingleAllocationDeallocation_TestConservative)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_conservative};
    int *created_object = alc1.allocate(1);
    alc1.deallocate(created_object, 1);
}

// Shared cross-allocation-deallocation test
TEST_F(FixedAllocatorTestsStatic,
       test_TC_MEMKIND_Allocator_SharedAllocationDeallocation_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    libmemkind::fixed::allocator<int> alc2{*alloc_source_f1};
    int *created_object = nullptr;
    int *created_object_2 = nullptr;
    created_object = alc1.allocate(1);
    ASSERT_TRUE(alc1 == alc2);
    alc2.deallocate(created_object, 1);
    created_object = alc2.allocate(1);
    alc1.deallocate(created_object, 1);

    created_object = alc1.allocate(1);
    created_object_2 = alc2.allocate(1);
    alc2.deallocate(created_object, 1);
    alc1.deallocate(created_object_2, 1);
}

// Construct-destroy test
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_Allocator_ConstructDestroy_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source_f1};
    int *created_object = alc1.allocate(1);
    alc1.construct(created_object);
    alc1.destroy(created_object);
}

// Testing thread-safety support
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_Allocator_MultithreadingSupport_Test)
{
    const size_t num_threads = 1000;
    const size_t iteration_count = 5000;

    std::vector<std::thread> thds;
    for (size_t i = 0; i < num_threads; ++i) {

        thds.push_back(std::thread([&]() {
            std::vector<libmemkind::fixed::allocator<int>> allocators_local;
            for (size_t j = 0; j < iteration_count; j++) {
                allocators_local.push_back(
                    libmemkind::fixed::allocator<int>(*alloc_source));
            }
            for (size_t j = 0; j < iteration_count; j++) {
                allocators_local.pop_back();
            }
        }));
    }

    for (size_t i = 0; i < num_threads; ++i) {
        thds[i].join();
    }
}

// Test multiple memkind allocator usage
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_MultipleAllocatorUsage_Test)
{

    void *addr = mmap(NULL, fixed_max_size, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    {
        libmemkind::fixed::allocator<int> alloc{addr, fixed_max_size};
    }

    {
        libmemkind::fixed::allocator<int> alloc{addr, fixed_max_size};
    }
    munmap(addr, fixed_max_size);
}

// Test vector
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_AllocatorUsage_Vector_Test)
{
    libmemkind::fixed::allocator<int> alc{*alloc_source};
    std::vector<int, libmemkind::fixed::allocator<int>> vector{alc};

    const int num = 20;

    for (int i = 0; i < num; ++i) {
        vector.push_back(0xDEAD + i);
    }

    for (int i = 0; i < num; ++i) {
        ASSERT_TRUE(vector[i] == 0xDEAD + i);
    }
}

// Test list
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_AllocatorUsage_List_Test)
{
    libmemkind::fixed::allocator<int> alc{*alloc_source};

    std::list<int, libmemkind::fixed::allocator<int>> list{alc};

    const int num = 4;

    for (int i = 0; i < num; ++i) {
        list.emplace_back(0xBEAC011 + i);
        ASSERT_TRUE(list.back() == 0xBEAC011 + i);
    }

    for (int i = 0; i < num; ++i) {
        list.pop_back();
    }
}

// Test map
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_AllocatorUsage_Map_Test)
{
    libmemkind::fixed::allocator<std::pair<const std::string, std::string>> alc{
        *alloc_source};

    std::map<
        std::string, std::string, std::less<std::string>,
        libmemkind::fixed::allocator<std::pair<const std::string, std::string>>>
        map{std::less<std::string>(), alc};

    const int num = 10;

    for (int i = 0; i < num; ++i) {
        map[std::to_string(i)] = std::to_string(0x0CEA11 + i);
        ASSERT_TRUE(map[std::to_string(i)] == std::to_string(0x0CEA11 + i));
        map[std::to_string((i * 997 + 83) % 113)] =
            std::to_string(0x0CEA11 + i);
        ASSERT_TRUE(map[std::to_string((i * 997 + 83) % 113)] ==
                    std::to_string(0x0CEA11 + i));
    }
}

#if _GLIBCXX_USE_CXX11_ABI
// Test vector of strings
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_AllocatorUsage_VectorOfString_Test)
{
    typedef std::basic_string<char, std::char_traits<char>,
                              libmemkind::fixed::allocator<char>>
        pmem_string;
    typedef libmemkind::fixed::allocator<pmem_string> pmem_alloc;

    pmem_alloc alc{*alloc_source};
    libmemkind::fixed::allocator<char> st_alc{alc};

    std::vector<pmem_string, std::scoped_allocator_adaptor<pmem_alloc>> vec{
        alc};
    pmem_string arg{"Very very loooong striiiing", st_alc};

    vec.push_back(arg);
    ASSERT_TRUE(vec.back() == arg);
}

// Test map of int strings
TEST_F(FixedAllocatorTestsStatic,
       test_TC_MEMKIND_AllocatorScopedUsage_MapOfIntString_Test)
{
    typedef std::basic_string<char, std::char_traits<char>,
                              libmemkind::fixed::allocator<char>>
        pmem_string;
    typedef int key_t;
    typedef pmem_string value_t;
    typedef std::pair<const key_t, value_t> target_pair;
    typedef libmemkind::fixed::allocator<target_pair> pmem_alloc;
    typedef libmemkind::fixed::allocator<char> str_allocator_t;
    typedef std::map<key_t, value_t, std::less<key_t>,
                     std::scoped_allocator_adaptor<pmem_alloc>>
        map_t;

    pmem_alloc map_allocator(*alloc_source);

    str_allocator_t str_allocator(map_allocator);

    value_t source_str1("Lorem ipsum dolor ", str_allocator);
    value_t source_str2("sit amet consectetuer adipiscing elit", str_allocator);

    map_t target_map{std::scoped_allocator_adaptor<pmem_alloc>(map_allocator)};

    target_map[key_t(165)] = source_str1;
    ASSERT_TRUE(target_map[key_t(165)] == source_str1);
    target_map[key_t(165)] = source_str2;
    ASSERT_TRUE(target_map[key_t(165)] == source_str2);
}
#endif // _GLIBCXX_USE_CXX11_ABI




// Test multiple memkind allocator usage - vector of int
TEST_F(FixedAllocatorTestsStatic, test_TC_MEMKIND_MultipleAllocatorUsageVector_Test)
{
    libmemkind::fixed::allocator<int> alc1{*alloc_source};
    libmemkind::fixed::allocator<int> alc2{*alloc_source};
    std::vector<int, libmemkind::fixed::allocator<int>> vector1{alc1};
    std::vector<int, libmemkind::fixed::allocator<int>> vector2{alc2};

    const int num = 20;

    for (int i = 0; i < num; ++i) {
        vector1.push_back(0xDEAD + i);
        vector2.push_back(0xFEED + i);
    }

    for (int i = 0; i < num; ++i) {
        ASSERT_TRUE(vector1[i] == 0xDEAD + i);
        ASSERT_TRUE(vector2[i] == 0xFEED + i);
    }
}
