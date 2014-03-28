#include <stdlib.h>
#include <stdio.h>
#include <numaif.h>
#include <check.h>


int check_page_hbw(size_t num_bandwidth, const int *bandwidth, 
                   const void *ptr, size_t size)
{
    int err = 0;
    const size_t page_size = 4096;
    int i, nr_pages, max_bandwidth;
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
                        "failed in call to malloc(%i)\n", 
                        nr_pages * sizeof(void*));
        return -2;
    }
    status = (int*)malloc(sizeof(int) * nr_pages);
    if (!address) {
        fprintf(stderr, "WARNING:  <check_page_hbw> "
                        "failed in call to malloc(%i)\n", 
                        nr_pages * sizeof(int));
        free(address);
        return -2;
    }

    for (i = 0; i < nr_pages; ++i) {
        address[i] = (char *)ptr + i * page_size;
    }

    move_pages(0, nr_pages, address, NULL, status, MPOL_MF_MOVE);

    max_bandwidth = 0;
    for (i = 0; i < num_bandwidth; ++i) {
        max_bandwidth = bandwidth[i] > max_bandwidth ? 
                        bandwidth[i] : max_bandwidth;
    }

    if (bandwidth[status[0]] != max_bandwidth) {
        err = -1;
    }
    for (i = 1; i < nr_pages && !err; ++i) {
        if (status[i] >= num_bandwidth ||
            bandwidth[status[i]] != max_bandwidth) {
            err = i;
        }
    }
    free(status);
    free(address);
    return err;
            
}
int check_page_size(void *ptr, size_t size, size_t page_size)
{

}

