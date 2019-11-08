/*
 * Copyright (c) 2019 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "memkind_allocator.h"

#include <cassert>
#include <forward_list>
#include <iostream>
#include <scoped_allocator>
#include <set>
#include <string>
#include <deque>

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
        libmemkind::static_kind::allocator<int> alc{ libmemkind::kinds::REGULAR };
        std::forward_list<int, libmemkind::static_kind::allocator<int>> fwlist {alc};
        int i;

        for (i = 0; i <= 20; ++i) {
            fwlist.push_front(i);
        }

        fwlist.sort(std::greater<int>());

        for(auto &el: fwlist) {
            i--;
            assert(el == i);
        }

        fwlist.clear();

        std::cout << "FORWARD LIST CLOSE" << std::endl;
    }
#endif

#ifdef STL_DEQUE_TEST
    std::cout << "DEQUE OPEN" << std::endl;
    libmemkind::static_kind::allocator<int> int_alc{ libmemkind::kinds::REGULAR };
    std::deque<int, libmemkind::static_kind::allocator<int>> deque {int_alc};

    for (int i=0; i<10; i++) {
        deque.push_back(i);
    }

    assert(deque.size() == 10);

    while(!deque.empty()) {
        deque.pop_front();
    }

    assert(deque.size() == 0);

    std::cout << "DEQUE CLOSE" << std::endl;
#endif

#ifdef STL_MULTISET_TEST
    {
        std::cout << "MULTISET OPEN" << std::endl;
        typedef libmemkind::static_kind::allocator<std::string> multiset_alloc_t;
        multiset_alloc_t multiset_alloc { libmemkind::kinds::DEFAULT };
        std::multiset<std::string, std::less<std::string>, std::scoped_allocator_adaptor<multiset_alloc_t> >
        multiset{ std::scoped_allocator_adaptor<multiset_alloc_t>(multiset_alloc) };

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
