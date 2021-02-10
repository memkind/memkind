// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2018 - 2020 Intel Corporation. */

#include "pmem_allocator.h"

#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <scoped_allocator>
#include <string>
#include <sys/stat.h>
#include <vector>

#define STL_VECTOR_TEST
#define STL_LIST_TEST
#if _GLIBCXX_USE_CXX11_ABI
#define STL_VEC_STRING_TEST
#define STL_MAP_INT_STRING_TEST
#endif

void cpp_allocator_test(const char *pmem_directory)
{
    std::cout << "TEST SCOPE: HELLO" << std::endl;

    size_t pmem_max_size = 1024 * 1024 * 1024;

#ifdef STL_VECTOR_TEST
    {
        std::cout << "VECTOR OPEN" << std::endl;
        libmemkind::pmem::allocator<int> alc{pmem_directory, pmem_max_size};
        std::vector<int, libmemkind::pmem::allocator<int>> vector{alc};

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
        libmemkind::pmem::allocator<int> alc{pmem_directory, pmem_max_size};
        std::list<int, libmemkind::pmem::allocator<int>> list{alc};

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
        typedef libmemkind::pmem::allocator<char> str_alloc_t;
        typedef std::basic_string<char, std::char_traits<char>, str_alloc_t>
            pmem_string;
        typedef libmemkind::pmem::allocator<pmem_string> vec_alloc_t;

        vec_alloc_t vec_alloc{pmem_directory, pmem_max_size};
        str_alloc_t str_alloc{pmem_directory, pmem_max_size};

        std::vector<pmem_string, std::scoped_allocator_adaptor<vec_alloc_t>>
            vec{std::scoped_allocator_adaptor<vec_alloc_t>(vec_alloc)};

        pmem_string arg{"Very very loooong striiiing", str_alloc};

        vec.push_back(arg);
        assert(vec.back() == arg);

        std::cout << "STRINGED VECTOR CLOSE" << std::endl;
    }

#endif

#ifdef STL_MAP_INT_STRING_TEST
    {
        std::cout << "INT_STRING MAP OPEN" << std::endl;
        typedef std::basic_string<char, std::char_traits<char>,
                                  libmemkind::pmem::allocator<char>>
            pmem_string;
        typedef int key_t;
        typedef pmem_string value_t;
        typedef libmemkind::pmem::allocator<std::pair<const key_t, value_t>>
            allocator_t;
        typedef std::map<key_t, value_t, std::less<key_t>,
                         std::scoped_allocator_adaptor<allocator_t>>
            map_t;

        allocator_t allocator(pmem_directory, pmem_max_size);

        value_t source_str1("Lorem ipsum dolor ", allocator);
        value_t source_str2("sit amet consectetuer adipiscing elit", allocator);

        map_t target_map{std::scoped_allocator_adaptor<allocator_t>(allocator)};

        target_map[key_t(165)] = source_str1;
        assert(target_map[key_t(165)] == source_str1);
        target_map[key_t(165)] = source_str2;
        assert(target_map[key_t(165)] == source_str2);

        std::cout << "INT_STRING MAP CLOSE" << std::endl;
    }
#endif
    std::cout << "TEST SCOPE: GOODBYE" << std::endl;
}

int main(int argc, char *argv[])
{
    const char *pmem_directory = "/tmp/";

    if (argc > 2) {
        std::cerr
            << "Usage: pmem_cpp_allocator [directory path]\n"
            << "\t[directory path] - directory where temporary file is created (default = \"/tmp/\")"
            << std::endl;
        return 0;
    } else if (argc == 2) {
        struct stat st;
        if (stat(argv[1], &st) != 0 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "%s : Invalid path to pmem kind directory\n",
                    argv[1]);
            return 1;
        }
        int status = memkind_check_dax_path(argv[1]);
        if (!status) {
            std::cout << "PMEM kind %s is on DAX-enabled File system.\n"
                      << std::endl;
        } else {
            std::cout << "PMEM kind %s is not on DAX-enabled File system.\n"
                      << std::endl;
        }
        pmem_directory = argv[1];
    }

    cpp_allocator_test(pmem_directory);
    return 0;
}
