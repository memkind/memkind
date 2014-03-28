#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <numaif.h>
#include "check.h"


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

    move_pages(0, nr_pages, address, NULL, status, MPOL_MF_MOVE);

    max_bandwidth = 0;
    for (j = 0; j < num_bandwidth; ++j) {
        max_bandwidth = bandwidth[j] > max_bandwidth ? 
                        bandwidth[j] : max_bandwidth;
    }

    if (bandwidth[status[0]] != max_bandwidth) {
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
int Check::check_page_size(void *ptr, size_t size, size_t page_size)
{
    int err = 0;
    size_t i;
    size_t num_check, test= 0;
    num_check = size / 4096;
    num_check += size % 4096 ? 1 : 0;
    err = check_page_size(ptr, &test);
    err = err ? -1 : 0;
    if (test != page_size) {
        err = -1;
    }
    for (i = 1; i < num_check && !err; ++i) {
        check_page_size((char *)ptr + i * 4096, &test);
        if (test != page_size) {
            err = i;
        }
    }
    return err;
}


int Check::check_page_size(void *ptr, size_t *page_size)
{
  
  size_t lpsize;
  int err = 0;
  unsigned long long phys_addr;

  phys_addr = get_physaddr(ptr, &lpsize);
  if (!phys_addr){
    err = -1;
    goto exit;
  }

  switch (lpsize){
  case 12:
    *page_size = 4 * 1024;
    break;
  case 21:
    *page_size = 2 * 1024 * 1024;
    break;
  case 31:
    *page_size = 1024 * 1024 * 1024;
    break;
  default:
    err = -1;
    break;
  }

 exit:
  return err;
}

unsigned long long Check::get_physaddr(void *vaddr, size_t *page_size)
{
   unsigned int pagemap_fd;
   unsigned long long addr;
   
   pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
   if (pagemap_fd < 0) {
       return 0;
   }
   int n = pread(pagemap_fd, &addr, 8, ((unsigned long long)vaddr / 4096) * 8);

   if (n != 8) {
     perror("pread");
     return 0;
   }
   if (!(addr & (1ULL<<63))) { 
     return 0;
   }
   *page_size = (addr >> 55) & 0x3fUL;

   addr &= ~(1ULL<<60)-1;
   addr <<= 12;
   return addr + ((unsigned long long)vaddr  & (4096-1));
}
