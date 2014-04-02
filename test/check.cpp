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

#include "check.h"


using namespace std;

Check::Check(const void *ptr, size_t size)
{
    const int min_page_size = 4096;
    int i;
    if (ptr && size) {
        num_address = size / min_page_size;
        num_address += size % min_page_size ? 1 : 0;
        address = new void* [num_address];
        for (i = 0; i < num_address; ++i) {
            address[i] = (char *)ptr + i * min_page_size;
        }
    }
    else {
        address = NULL;
    }
}

Check::~Check()
{
    if (address) {
        delete[] address;
    }
}

Check::Check(const Check &other)
{
    num_address = other.num_address;

    address = new void* [num_address];
    for (int i = 0; i < num_address; ++i) {
        address[i] = other.address[i];
    }
}

int Check::check_node_hbw(size_t num_bandwidth, const int *bandwidth)
{
    int err = 0;
    int i, max_bandwidth;
    size_t j;
    int *status = NULL;


    status = new int [num_address];

    move_pages(0, num_address, address, NULL, status, MPOL_MF_MOVE);

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

void Check::skip_lines(ifstream &ip, int num_lines){
   
    int i;
    string temp;
    for (i = 0; i < num_lines;
         i++){
      getline (ip, temp);
    }
}

void Check::get_address_range(string &line,
                              unsigned long long *start_addr,
                              unsigned long long *end_addr){
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

size_t Check::get_kpagesize(string line){
  
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
    int i;

    ip.open ("/proc/self/smaps");

    if (!check_page_size(page_size, address[0])) {
        err = -1;
    }
    for (i = 1; i < num_address && !err; ++i) {
        if (check_page_size(page_size, address[i])) {
            err = i;
        }
    }
    return err;
}

int Check::check_page_size(size_t page_size, void *vaddr){
  

    string read;
    unsigned long long virt_addr;
    size_t lpagesize;
    
    virt_addr = (unsigned long long)(vaddr);

    while (!ip.eof()){
      ip >> read;
      start_addr = end_addr = 0;
      get_address_range(read, &start_addr,
                        &end_addr);
      if ((virt_addr >= start_addr) &&
          (virt_addr <= end_addr)){
        
        skip_lines(ip, 12);
        getline(ip, read);
        lpagesize = get_kpagesize(read);
        if (lpagesize == page_size){
          return 1;
        }
      }
      else{
        skip_lines(ip,14);
      }
    }

    /*Never found a match!*/
    return 0;
}

