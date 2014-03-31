#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <numaif.h>
#include "check.h"

static int pagemap_fd = -1;

int Check::check_page_hbw(size_t num_bandwidth, const int *bandwidth, 
                          const void *ptr, size_t size)
{
    int err = 0;
    const int page_size = 4096;
    int i, nr_pages, max_bandwidth;
    size_t j;
    void **address = NULL;
    int *status = NULL;

    if (size == 0) {
        return 0;
    }

    nr_pages = size / page_size;
    nr_pages += size % page_size ? 1 : 0;

    address = (void**)malloc(sizeof(void *) * nr_pages);
    if (!address) {
        fprintf(stderr, "WARNING:  <check_page_hbw> "
                        "failed in call to malloc(%lui)\n", 
                        nr_pages * sizeof(void*));
        return -2;
    }
    status = (int*)malloc(sizeof(int) * nr_pages);
    if (!address) {
        fprintf(stderr, "WARNING:  <check_page_hbw> "
                        "failed in call to malloc(%lui)\n", 
                        nr_pages * sizeof(int));
        free(address);
        return -2;
    }

    for (i = 0; i < nr_pages; ++i) {
        address[i] = (char *)ptr + i * page_size;
    }

    if (!check_page_size(address, page_size,
                         nr_pages)){
      fprintf(stderr, "ERROR:  <check_page_hbw> "
              "failed in call to check_page_size\n");
    }


    move_pages(0, nr_pages, address, NULL, status, MPOL_MF_MOVE);

    max_bandwidth = 0;
    for (j = 0; j < num_bandwidth; ++j) {
        max_bandwidth = bandwidth[j] > max_bandwidth ? 
                        bandwidth[j] : max_bandwidth;
    }

    if ((size_t)status[0] >= num_bandwidth ||
        bandwidth[status[0]] != max_bandwidth) {
        err = -1;
    }
    for (i = 1; i < nr_pages && !err; ++i) {
        if ((size_t)status[i] >= num_bandwidth ||
            bandwidth[status[i]] != max_bandwidth) {
            err = i;
        }
    }
    free(status);
    free(address);
    return err;
            
}

int Check::check_page_size(void **addr_ptr, size_t page_size, 
                           int nr_pages){
  
  int i, ret = 1;
  
  if (pagemap_fd < 0){
    pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
    if (pagemap_fd < 0)
      return 0;
  }
  
  for (i = 0; i < nr_pages; i++){
    if (!is_page_present(addr_ptr[i],
                         page_size)){
      return 0;
    }
  }  
  return ret;
}

int Check::is_page_present(void *vaddr, size_t page_size){
  
  int n;
  unsigned long long addr;
  int ret = 1;
  
  n = pread(pagemap_fd, &addr, 
            PAGEMAP_ENTRY,
            (((unsigned long long)vaddr/page_size)*PAGEMAP_ENTRY));
  
  if (n != PAGEMAP_ENTRY){
    perror("pread");
    ret = -1;
  }
  
  if (!(addr & (1ULL<<63))) {
    ret = 0;
  }
  
  return ret;
}
