/*
 * Copyright (C) 2014, 2015 Intel Corporation.
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <numaif.h>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#include "check.h"


using namespace std;


/*Check each page between the start
and the end address additionally also
check the end address for pagesize*/
Check::Check(const void *p, const trial_t &trial)
{
    const size_t min_page_size = 4096;
    this->ptr = p;
    this->size = trial.size;
    size_t page_size = (trial.page_size >= min_page_size ? trial.page_size : min_page_size);
    if (p && size)
    {
        num_address = size / page_size;
        num_address += size % page_size ? 1 : 0;

        address = new void* [num_address];
        size_t i;
        for (i = 0; i < num_address - 1; ++i)
        {
            address[i] = (char *)ptr + i * page_size;
        }
        address[i] = (char *)p + size - 1;
    }
    else
    {
        address = NULL;
    }
}

Check::~Check()
{
     delete[] address;
}

Check::Check(const Check &other)
{
    num_address = other.num_address;

    address = new void* [num_address];
    for (size_t i = 0; i < num_address; ++i) {
        address[i] = other.address[i];
    }
}

int Check::check_node_hbw(size_t num_bandwidth, const int *bandwidth)
{
    int err = 0;
    int max_bandwidth;
    size_t i, j;
    int *status = NULL;

    status = new int [num_address];

    for (i = 0; i < num_address; i++)
    {
        get_mempolicy(&status[i], NULL, 0, address[i], MPOL_F_NODE | MPOL_F_ADDR);
    }

    max_bandwidth = 0;
    for (j = 0; j < num_bandwidth; ++j) {
        max_bandwidth = bandwidth[j] > max_bandwidth ?
                        bandwidth[j] : max_bandwidth;
    }

    if ((size_t)status[0] >= num_bandwidth ||
        bandwidth[status[0]] != max_bandwidth) {
        err = -1;
    }
    for (i = 1; i < num_address && !err; ++i) {
        if ((size_t)status[i] >= num_bandwidth ||
            bandwidth[status[i]] != max_bandwidth) {
            err = i;
        }
    }

    delete[] status;

    return err;

}

int Check::check_node_hbw_interleave(size_t num_bandwidth, const int *bandwidth)
{
    printf("Running check for hbw interleave\n");
    printf("num_bandwidth = %zd\n num_address = %zd \n",num_bandwidth, num_address );

    int err = 0;
    int max_bandwidth;
    int *status = NULL;
    int *hbw_nodes =  NULL;
    bool page_in_node = false;

    status = new int [num_address];
    hbw_nodes = new int[num_bandwidth];

    move_pages(0, num_address, address, NULL, status, MPOL_MF_MOVE);
    max_bandwidth = 0;
    memset(hbw_nodes, 0, sizeof(int)*num_bandwidth);

    // Here we will identify which nodes are HBW and set the max
    // bandwidth in the systeml
    for (size_t i = 0; i < num_bandwidth; ++i) {
        if (bandwidth[i] > max_bandwidth) {
            // Set hbw_nodes to 0 since a new max bandwidth
            // has been identified, and then set the node to 1
            memset(hbw_nodes, 0, sizeof(int)*num_bandwidth);
            max_bandwidth = bandwidth[i];
            hbw_nodes[i] = 1;
        }
        else if (bandwidth[i] == max_bandwidth) {
            hbw_nodes[i] = 1;
        }
    }

    for (size_t i = 0 ; i < num_bandwidth && !err; ++i) {
        page_in_node = false;
        //Find a hbw_node
        if (hbw_nodes[i]) {
            //Itereate through num_address to see
            //if a status position is set to the
            // hbw node
            for(size_t j = 0; j < num_address; ++j) {
                //Minus 1 is because nodes are 1-based
                //and hbw_nodes is a 0-based index array
                if(((size_t)status[j] - 1) == i) {
                    page_in_node = true;
                    break;
                }
            }
            //None page has been assign to hbw node
            if (!page_in_node) {
                err = 1;
            }
        }
    }

    delete[] status;
    delete[] hbw_nodes;
    return err;
}

int Check::check_zero(void)
{
    size_t i;
    const char *cptr = (char *)ptr;
    for (i = 0; i < size; ++i) {
        if (cptr[i] != '\0') {
            return -1;
        }
    }
    return 0;
}

int Check::check_data(int data)
{
    int ret;
    void *p;
    p = malloc(size);
    memset(p, data, size);
    memset((void*)ptr, data, size);
    ret = memcmp(p, ptr, size);
    free(p);
    return ret;
}

int Check::check_align(size_t align)
{
    return (size_t)ptr % align;
}

string Check::skip_to_next_entry (ifstream &ip)
{

    string temp, token;
    size_t found = 0;
    string empty ="";

    while (!ip.eof()) {
        getline (ip, temp);
        found = temp.find("-");
        if (found != string::npos) {
            istringstream iss(temp);
            getline(iss, token, ' ');
            return token;
        }
    }
    return empty;
}

string Check::skip_to_next_kpage(ifstream &ip)
{

    string temp, token;
    size_t found = 0;
    string empty ="";

    while (!ip.eof()) {
        getline (ip, temp);
        found = temp.find("KernelPageSize:");
        if (found != string::npos) {
            return temp;
        }
    }
    return empty;
}


void Check::get_address_range(string &line,
                              unsigned long long *start_addr,
                              unsigned long long *end_addr)
{
    stringstream ss(line);
    string token;

    getline(ss, token, '-');
    *start_addr = strtoul(token.c_str(),
                          NULL,
                          16);
    getline(ss, token, '-');
    *end_addr = strtoul(token.c_str(),
                        NULL,
                        16);
}

size_t Check::get_kpagesize(string line)
{

    stringstream ss(line);
    string token;
    size_t pagesize;


    ss  >> token;
    ss  >> token;

    pagesize = atol(token.c_str());

    return (size_t)pagesize;
}

int Check::check_page_size(size_t page_size)
{
    int err = 0;
    size_t i;

    ip.open ("/proc/self/smaps");

    populate_smaps_table();
    if (check_page_size(page_size, address[0])) {
        err = -1;
    }
    for (i = 1; i < num_address && !err; ++i) {
        if (check_page_size(page_size, address[i])) {
            err = i;
        }
    }
    return err;
}

int Check::populate_smaps_table ()
{

    string read;
    size_t lpagesize;
    smaps_entry_t lentry;
    unsigned long long start_addr;
    unsigned long long end_addr;

    ip >> read;
    while (!ip.eof()) {

        start_addr = end_addr = 0;
        get_address_range (read,
                           &start_addr,
                           &end_addr);
        read = skip_to_next_kpage(ip);
        getline(ip, read);
        lpagesize = get_kpagesize(read);
        lpagesize *= 1024;
        lentry.start_addr = start_addr;
        lentry.end_addr = end_addr;
        lentry.pagesize = lpagesize;
        smaps_table.push_back(lentry);
        read = skip_to_next_entry(ip);
        if (read.empty()) {
            break;
        }
    }

    if (0 == smaps_table.size()) {
        fprintf(stderr,"Empty smaps table\n");
        return -1;
    }
    else {
        return 0;
    }

}

int Check::check_page_size(size_t page_size, void *vaddr)
{


    string read;
    unsigned long long virt_addr;
    size_t lpagesize;
    list<smaps_entry_t>::iterator it;
    unsigned long long start_addr;
    unsigned long long end_addr;

    virt_addr = (unsigned long long)(vaddr);

    for (it = smaps_table.begin();
         it != smaps_table.end();
         it++) {

        start_addr = it->start_addr;
        end_addr = it->end_addr;

        if ((virt_addr >= start_addr) &&
            (virt_addr < end_addr)) {
            lpagesize = it->pagesize;
            if (lpagesize == page_size) {
                return 0;
            }
            else {
                /*The pagesize of allocation and req don't match*/
                fprintf(stderr,"%zd does not match entry in SMAPS (%zd)\n",
                        page_size, lpagesize);
                return -1;
            }
        }
    }
    /*Never found a match!*/
    return 1;
}
