#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <numaif.h>
#include <errno.h>
#include "check.h"


Check::Check(const void *ptr, size_t size)
{
    const int min_page_size = 4096;

    num_address = size / min_page_size;
    num_address += size % min_page_size ? 1 : 0;

    address = new (void *)[num_address];

    for (i = 0; i < num_address; ++i) {
        address[i] = (char *)ptr + i * page_size;
    }

    pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
    if (pagemap_fd < 0) {
        throw errno;
    }
}

Check::~Check()
{
    delete[] address;
    if (close(pagemap_fd)){
        throw errno;
    }
}

Check::Check(const Check &other)
{
    num_address = other.num_address;

    address = new (void *)[num_address];
    for (i = 0; i < num_address; ++i) {
        address[i] = other.address[i];

    pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
    if (pagemap_fd < 0) {
        throw errno;
    }
}

int Check::check_node_hbw(size_t num_bandwidth, const int *bandwidth)
{
    int err = 0;
    int i, num_address, max_bandwidth;
    size_t j;
    int *status = NULL;

    if (size == 0) {
        return 0;
    }

    status = new (int)[num_address]

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

int Check::check_page_size(size_t page_size)
{
    int err = 0;
    int i;

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
    int err = 0;
    const int PAGEMAP_ENTRY = 8;
    int n;
    unsigned long long addr;

    n = pread(pagemap_fd, &addr, PAGEMAP_ENTRY,
              (((unsigned long long)vaddr/page_size)*PAGEMAP_ENTRY));

    if (n != PAGEMAP_ENTRY) {
        throw errno;
    }

    if (!(addr & (1ULL<<63))) {
        err = 1;
    }

    return err;
}
