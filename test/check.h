// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#ifndef check_include_h
#define check_include_h

#include "trial_generator.h"
#include <list>

typedef struct {
    unsigned long start_addr;
    unsigned long end_addr;
    size_t pagesize;
} smaps_entry_t;

using namespace std;

class Check
{
public:
    Check(const void *p, const trial_t &trial);
    Check(const void *p, const size_t size, const size_t page_size);
    Check(const Check &);
    ~Check();
    int check_page_size(size_t page_size);
    int check_zero(void);
    int check_align(size_t align);

private:
    const void *ptr;
    size_t size;
    void **address;
    size_t num_address;
    list<smaps_entry_t> smaps_table;
    ifstream ip;
    string skip_to_next_entry(ifstream &);
    string skip_to_next_kpage(ifstream &);
    void get_address_range(string &line, unsigned long long *start_addr,
                           unsigned long long *end_addr);
    size_t get_kpagesize(string line);
    int check_page_size(size_t page_size, void *vaddr);
    int populate_smaps_table();
};

#endif
