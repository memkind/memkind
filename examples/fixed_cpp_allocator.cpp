// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2021-2023 Intel Corporation. */

#include "fixed_allocator.h"

#include <cassert>
#include <deque>
#include <forward_list>
#include <iostream>
#include <scoped_allocator>
#include <set>
#include <string>
#include <sys/mman.h>

#define STL_FWLIST_TEST
#define STL_DEQUE_TEST
#if _GLIBCXX_USE_CXX11_ABI
#define STL_MULTISET_TEST
#endif

#define FIXED_MAP_SIZE (1024 * 1024 * 32) // 32 MB

int main(int argc, char *argv[])
{
    std::cout << "TEST SCOPE: HELLO" << std::endl;

    void *addr = mmap(NULL, FIXED_MAP_SIZE, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    assert(addr != MAP_FAILED);

#ifdef STL_FWLIST_TEST
    {
        std::cout << "FORWARD LIST OPEN" << std::endl;
        try {
            libmemkind::fixed::allocator<int> alc{addr, FIXED_MAP_SIZE};
            std::forward_list<int, libmemkind::fixed::allocator<int>> fwlist{
                alc};
            int i;

            for (i = 0; i <= 20; ++i) {
                fwlist.push_front(i);
            }

            fwlist.sort(std::greater<int>());

            for (auto &el : fwlist) {
                i--;
                assert(el == i);
            }

            fwlist.clear();
        } catch (const std::exception &e) {
            std::cout
                << "Something went wrong (it's most likely a memory allocation failure): "
                << e.what() << std::endl;
        }
        std::cout << "FORWARD LIST CLOSE" << std::endl;
    }
#endif

#ifdef STL_DEQUE_TEST
    {
        std::cout << "DEQUE OPEN" << std::endl;
        try {
            libmemkind::fixed::allocator<int> int_alc{addr, FIXED_MAP_SIZE};
            std::deque<int, libmemkind::fixed::allocator<int>> deque{int_alc};

            for (int i = 0; i < 10; i++) {
                deque.push_back(i);
            }

            assert(deque.size() == 10);

            while (!deque.empty()) {
                deque.pop_front();
            }

            assert(deque.size() == 0);
        } catch (const std::exception &e) {
            std::cout
                << "Something went wrong (it's most likely a memory allocation failure): "
                << e.what() << std::endl;
        }
        std::cout << "DEQUE CLOSE" << std::endl;
    }
#endif

#ifdef STL_MULTISET_TEST
    {
        std::cout << "MULTISET OPEN" << std::endl;
        try {
            typedef libmemkind::fixed::allocator<std::string> multiset_alloc_t;
            multiset_alloc_t multiset_alloc{addr, FIXED_MAP_SIZE};
            std::multiset<std::string, std::less<std::string>,
                          std::scoped_allocator_adaptor<multiset_alloc_t>>
                multiset{std::scoped_allocator_adaptor<multiset_alloc_t>(
                    multiset_alloc)};

            multiset.insert("S");
            multiset.insert("A");
            multiset.insert("C");
            multiset.insert("S");
            multiset.insert("C");
            multiset.insert("E");

            std::string test;
            for (auto &n : multiset) {
                test.append(n);
            }
            assert(test == "ACCESS");
        } catch (const std::exception &e) {
            std::cout
                << "Something went wrong (it's most likely a memory allocation failure): "
                << e.what() << std::endl;
        }
        std::cout << "MULTISET CLOSE" << std::endl;
    }
#endif

    munmap(addr, FIXED_MAP_SIZE);
    std::cout << "TEST SCOPE: GOODBYE" << std::endl;

    return 0;
}
