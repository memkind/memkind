// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include "memkind_allocator.h"

#include <cassert>
#include <deque>
#include <forward_list>
#include <iostream>
#include <scoped_allocator>
#include <set>
#include <string>

#define STL_FWLIST_TEST
#define STL_DEQUE_TEST
#if _GLIBCXX_USE_CXX11_ABI
#define STL_MULTISET_TEST
#endif

int main(int argc, char *argv[])
{
    std::cout << "TEST SCOPE: HELLO" << std::endl;

#ifdef STL_FWLIST_TEST
    {
        std::cout << "FORWARD LIST OPEN" << std::endl;
        libmemkind::static_kind::allocator<int> alc{libmemkind::kinds::REGULAR};
        std::forward_list<int, libmemkind::static_kind::allocator<int>> fwlist{
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

        std::cout << "FORWARD LIST CLOSE" << std::endl;
    }
#endif

#ifdef STL_DEQUE_TEST
    std::cout << "DEQUE OPEN" << std::endl;
    libmemkind::static_kind::allocator<int> int_alc{libmemkind::kinds::REGULAR};
    std::deque<int, libmemkind::static_kind::allocator<int>> deque{int_alc};

    for (int i = 0; i < 10; i++) {
        deque.push_back(i);
    }

    assert(deque.size() == 10);

    while (!deque.empty()) {
        deque.pop_front();
    }

    assert(deque.size() == 0);

    std::cout << "DEQUE CLOSE" << std::endl;
#endif

#ifdef STL_MULTISET_TEST
    {
        std::cout << "MULTISET OPEN" << std::endl;
        typedef libmemkind::static_kind::allocator<std::string>
            multiset_alloc_t;
        multiset_alloc_t multiset_alloc{libmemkind::kinds::DEFAULT};
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

        std::cout << "MULTISET CLOSE" << std::endl;
    }
#endif

    std::cout << "TEST SCOPE: GOODBYE" << std::endl;

    return 0;
}
