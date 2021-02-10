// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include "common.h"
#include "memkind_allocator.h"
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

// Tests for memkind::allocator class.
class MemkindAllocatorTests: public ::testing::Test
{
public:
    libmemkind::static_kind::allocator<int> int_alc_regular{
        libmemkind::kinds::REGULAR};
    libmemkind::static_kind::allocator<int> int_alc_default{
        libmemkind::kinds::DEFAULT};

protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

// Compare two same kind different type allocators
TEST_F(MemkindAllocatorTests,
       test_TC_MEMKIND_AllocatorCompare_SameKindDifferentType_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_regular};
    libmemkind::static_kind::allocator<char> alc2{int_alc_regular};
    ASSERT_TRUE(alc1 == alc2);
    ASSERT_FALSE(alc1 != alc2);
}

// Compare two same kind same type
TEST_F(MemkindAllocatorTests,
       test_TC_MEMKIND_AllocatorCompare_SameKindSameType_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_default};
    libmemkind::static_kind::allocator<int> alc2{int_alc_default};
    ASSERT_TRUE(alc1 == alc2);
    ASSERT_FALSE(alc1 != alc2);
}

////Compare two different kind different type
TEST_F(MemkindAllocatorTests,
       test_TC_MEMKIND_AllocatorCompare_DifferentKindDifferentType_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_regular};
    libmemkind::static_kind::allocator<char> alc2{int_alc_default};
    ASSERT_FALSE(alc1 == alc2);
    ASSERT_TRUE(alc1 != alc2);
}

////Compare two different kind same type allocators
TEST_F(MemkindAllocatorTests,
       test_TC_MEMKIND_AllocatorCompare_DifferentKindSameType_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_regular};
    libmemkind::static_kind::allocator<int> alc2{int_alc_default};
    ASSERT_FALSE(alc1 == alc2);
    ASSERT_TRUE(alc1 != alc2);
}

////Copy assignment test
TEST_F(MemkindAllocatorTests, test_TC_MEMKIND_Allocator_CopyAssignment_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_regular};
    libmemkind::static_kind::allocator<int> alc2{int_alc_default};
    ASSERT_TRUE(alc1 != alc2);
    alc1 = alc2;
    ASSERT_TRUE(alc1 == alc2);
}

////Move constructor test
TEST_F(MemkindAllocatorTests, test_TC_MEMKIND_Allocator_MoveConstructor_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_default};
    libmemkind::static_kind::allocator<int> alc2{int_alc_default};
    ASSERT_TRUE(alc2 == alc1);
    libmemkind::static_kind::allocator<int> alc3 = std::move(alc1);
    ASSERT_TRUE(alc2 == alc3);
}

////Move assignment test
TEST_F(MemkindAllocatorTests, test_TC_MEMKIND_Allocator_MoveAssignment_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_regular};
    libmemkind::static_kind::allocator<int> alc2{int_alc_default};
    ASSERT_TRUE(alc1 != alc2);

    {
        libmemkind::static_kind::allocator<int> alc3{alc2};
        alc1 = std::move(alc3);
    }
    ASSERT_TRUE(alc1 == alc2);
}

////Single allocation-deallocation test
TEST_F(MemkindAllocatorTests,
       test_TC_MEMKIND_Allocator_SingleAllocationDeallocation_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_default};
    int *created_object = alc1.allocate(1);
    alc1.deallocate(created_object, 1);
}

////Shared cross-allocation-deallocation test
TEST_F(MemkindAllocatorTests,
       test_TC_MEMKIND_Allocator_SharedAllocationDeallocation_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_default};
    libmemkind::static_kind::allocator<int> alc2{int_alc_default};
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

////Construct-destroy test
TEST_F(MemkindAllocatorTests, test_TC_MEMKIND_Allocator_ConstructDestroy_Test)
{
    libmemkind::static_kind::allocator<int> alc1{int_alc_regular};
    int *created_object = alc1.allocate(1);
    alc1.construct(created_object);
    alc1.destroy(created_object);
}

////Testing thread-safety support
TEST_F(MemkindAllocatorTests,
       test_TC_MEMKIND_Allocator_MultithreadingSupport_Test)
{
    const size_t num_threads = 1000;
    const size_t iteration_count = 5000;

    std::vector<std::thread> thds;
    for (size_t i = 0; i < num_threads; ++i) {

        thds.push_back(std::thread([&]() {
            std::vector<libmemkind::static_kind::allocator<int>>
                allocators_local;
            for (size_t j = 0; j < iteration_count; j++) {
                allocators_local.push_back(
                    libmemkind::static_kind::allocator<int>(int_alc_regular));
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

////Test multiple memkind allocator usage
TEST_F(MemkindAllocatorTests, test_TC_MEMKIND_MultipleAllocatorUsage_Test)
{
    {
        libmemkind::static_kind::allocator<int> alloc{libmemkind::kinds::HBW};
    }

    {
        libmemkind::static_kind::allocator<int> alloc{
            libmemkind::kinds::DAX_KMEM};
    }
}

////Test vector
TEST_F(MemkindAllocatorTests, test_TC_MEMKIND_AllocatorUsage_Vector_Test)
{
    libmemkind::static_kind::allocator<int> alc{int_alc_default};
    std::vector<int, libmemkind::static_kind::allocator<int>> vector{alc};

    const int num = 20;

    for (int i = 0; i < num; ++i) {
        vector.push_back(0xDEAD + i);
    }

    for (int i = 0; i < num; ++i) {
        ASSERT_TRUE(vector[i] == 0xDEAD + i);
    }
}

////Test list
TEST_F(MemkindAllocatorTests, test_TC_MEMKIND_AllocatorUsage_List_Test)
{
    libmemkind::static_kind::allocator<int> alc{int_alc_regular};

    std::list<int, libmemkind::static_kind::allocator<int>> list{alc};

    const int num = 4;

    for (int i = 0; i < num; ++i) {
        list.emplace_back(0xBEAC011 + i);
        ASSERT_TRUE(list.back() == 0xBEAC011 + i);
    }

    for (int i = 0; i < num; ++i) {
        list.pop_back();
    }
}

////Test map
TEST_F(MemkindAllocatorTests, test_TC_MEMKIND_AllocatorUsage_Map_Test)
{
    typedef std::basic_string<std::pair<const std::string, std::string>,
                              libmemkind::static_kind::allocator<char>>
        memkind_string;
    libmemkind::static_kind::allocator<memkind_string> alc{
        libmemkind::kinds::REGULAR};

    std::map<std::string, std::string, std::less<std::string>,
             libmemkind::static_kind::allocator<
                 std::pair<const std::string, std::string>>>
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
TEST_F(MemkindAllocatorTests,
       test_TC_MEMKIND_AllocatorUsage_VectorOfString_Test)
{
    typedef std::basic_string<char, std::char_traits<char>,
                              libmemkind::static_kind::allocator<char>>
        memkind_string;
    typedef libmemkind::static_kind::allocator<memkind_string> memkind_alloc;

    memkind_alloc alc{libmemkind::kinds::DEFAULT};
    libmemkind::static_kind::allocator<char> st_alc{alc};

    std::vector<memkind_string, std::scoped_allocator_adaptor<memkind_alloc>>
        vec{alc};
    memkind_string arg{"Very very loooong striiiing", st_alc};

    vec.push_back(arg);
    ASSERT_TRUE(vec.back() == arg);
}

////Test map of int strings
TEST_F(MemkindAllocatorTests,
       test_TC_MEMKIND_AllocatorScopedUsage_MapOfIntString_Test)
{
    typedef std::basic_string<char, std::char_traits<char>,
                              libmemkind::static_kind::allocator<char>>
        memkind_string;
    typedef int key_t;
    typedef memkind_string value_t;
    typedef std::pair<const key_t, value_t> target_pair;
    typedef libmemkind::static_kind::allocator<target_pair> pmem_alloc;
    typedef libmemkind::static_kind::allocator<char> str_allocator_t;
    typedef std::map<key_t, value_t, std::less<key_t>,
                     std::scoped_allocator_adaptor<pmem_alloc>>
        map_t;

    pmem_alloc map_allocator(libmemkind::kinds::REGULAR);

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
