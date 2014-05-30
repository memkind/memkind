/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
 */

#ifndef check_include_h
#define check_include_h

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
    Check(const void *ptr, size_t size);
    Check(const Check &);
    ~Check();
    int check_node_hbw(size_t num_bandwidth, const int *bandwidth);
    int check_page_size(size_t page_size);
    int check_zero(void);
    int check_align(size_t align);
private:
    const void *ptr;
    size_t size;
    void **address;
    list<smaps_entry_t>smaps_table;
    ifstream ip;
    int smaps_fd;
    int num_address;
    string skip_to_next_entry(ifstream &);
    string skip_to_next_kpage(ifstream &);
    void get_address_range(string &line, unsigned long long *start_addr,
                           unsigned long long *end_addr);
    size_t get_kpagesize(string line);
    int check_page_size(size_t page_size, void *vaddr);
    int populate_smaps_table();
};

#endif
