/*
 * Copyright (C) 2018 Intel Corporation.
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


#include "pmem_allocator.h"
#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <utility>
#include <string>
#include <algorithm>
#include <scoped_allocator>
#include <cassert>

#define STL_VECTOR_TEST
#define STL_LIST_TEST
#define STL_VEC_STRING_TEST
#define STL_MAP_INT_STRING_TEST

void cpp_allocator_test() {
    std::cout << "TEST SCOPE: HELLO" << std::endl;

    const char* pmem_directory = "/dev/shm";
    size_t pmem_max_size = 1024*1024*1024;

    /*
     * Because of the issue #57 we cannot create multiple PMEM kinds.
     * Therefore, we create alloc_source object of pmem::allocator class
     * and use this object to copy-construct all other required allocators.
     * In this case all allocators use the same PMEM kind created inside alloc_source object.
     * When the issue #57 is fixed, we can remove alloc_source allocator
     * and create required allocators from scratch.
     */

    pmem::allocator<int> alloc_source { pmem_directory, pmem_max_size } ;

#ifdef STL_VECTOR_TEST
    {
        std::cout << "VECTOR OPEN" << std::endl;
        pmem::allocator<int> alc{ alloc_source  };
        std::vector<int, pmem::allocator<int>> vector{ alc };

        for (int i = 0; i < 20; ++i) {
            vector.push_back(0xDEAD + i);
            assert(vector.back() == 0xDEAD + i);

        }

        std::cout << "VECTOR CLOSE" << std::endl;
    }
#endif

#ifdef STL_LIST_TEST
    {
        std::cout << "LIST OPEN" << std::endl;
        pmem::allocator<int> alc{ alloc_source };
        std::list<int, pmem::allocator<int>> list{ alc };

        const int nx2 = 4;
        for (int i = 0; i < nx2; ++i) {
            list.emplace_back(0xBEAC011 + i);
            assert(list.back() == 0xBEAC011 + i);
        }

        for (int i = 0; i < nx2; ++i) {
            list.pop_back();
        }

        std::cout << "LIST CLOSE" << std::endl;
    }
#endif

#ifdef STL_VEC_STRING_TEST
    {
        std::cout << "STRINGED VECTOR OPEN" << std::endl;
        typedef pmem::allocator<char> str_alloc_t;
        typedef std::basic_string<char, std::char_traits<char>, str_alloc_t> pmem_string;
        typedef pmem::allocator<pmem_string> vec_alloc_t;

        vec_alloc_t vec_alloc{ alloc_source };
        str_alloc_t str_alloc{ alloc_source };

        std::vector<pmem_string, std::scoped_allocator_adaptor<vec_alloc_t> > vec{ std::scoped_allocator_adaptor<vec_alloc_t>(vec_alloc) };

        pmem_string arg{ "Very very loooong striiiing", str_alloc };

        vec.push_back(arg);
        assert(vec.back() == arg);

        std::cout << "STRINGED VECTOR CLOSE" << std::endl;
    }

#endif

#ifdef STL_MAP_INT_STRING_TEST
    {
        std::cout << "INT_STRING MAP OPEN" << std::endl;
        typedef std::basic_string<char, std::char_traits<char>, pmem::allocator<char> > pmem_string;
        typedef int key_t;
        typedef pmem_string value_t;
        typedef std::pair<key_t, value_t> target_pair;
        typedef pmem::allocator<target_pair> pmem_alloc;
        typedef pmem::allocator<char> str_allocator_t;
        typedef std::map<key_t, value_t, std::less<key_t>, std::scoped_allocator_adaptor<pmem_alloc> > map_t;

        pmem_alloc map_allocator( alloc_source );

        str_allocator_t str_allocator( map_allocator );

        value_t source_str1( "Lorem ipsum dolor " , str_allocator);
        value_t source_str2( "sit amet consectetuer adipiscing elit", str_allocator );

        map_t target_map{ std::scoped_allocator_adaptor<pmem_alloc>(map_allocator) };

        target_map[key_t(165)] = source_str1;
        assert(target_map[key_t(165)] == source_str1);
        target_map[key_t(165)] = source_str2;
        assert(target_map[key_t(165)] == source_str2);

        std::cout << "INT_STRING MAP CLOSE" << std::endl;
    }
#endif
    std::cout << "TEST SCOPE: GOODBYE" << std::endl;
}

int main() {
    cpp_allocator_test();
    return 0;
}
