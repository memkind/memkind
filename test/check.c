#include <check.h>
#include <asm/io.h

static int bandwith_to_hbw_mask(size_t num_bandwidth, int *bandwidth, 
                                 struct bitmask *hbw_mask)
{
    max_bandwidth = 0;
    for (i = 0; i < num_bandwidth; ++i) {
        max_bandwidth = bandwidth[i] > max_bandwidth ? 
                        bandwidth[i] : max_bandwidth;
    }

    if (max_bandwidth == 0) {
        fprintf(stderr, "WARNING:  <bandwidth_to_hbw_mask> "
                        "all bandwidths specified are zero\n");
        numa_bitmask_clearall(*hbw_mask);
        return -3;
    }

    for (i = 0; i < num_bandwidth; ++i) {
        if (bandwidth[i] == max_bandwidth) {
            num_bitmask_setbit(hbw_mask, i);
        }
    }
    return 0;
}

int check_page_hbw(void *ptr, size_t size, size_t page_size, 
                   size_t num_bandwidth, int *bandwidth)
{
    int err = 0;
    int i, nr_pages;
    struct bitmask *hbw_mask = NULL;
    void **address = NULL;

    if (size == 0) {
        return 0;
    }
    hbw_mask = numa_bitmask_alloc(num_bandwidth);
    if (hbw_mask == NULL) {
        fprintf(stderr, "WARNING:  <check_page_hbw> "
                        "failed in call to numa_bitmask_alloc(%i)\n", 
                num_bandwidth);
        return -2;
    }

    err = bandwidth_to_hbw_mask(num_bandwidth, hbw_mask);
    if (err) {
        numa_bitmask_free(hbw_mask);
        return err;
    }

    nr_pages = size / page_size;
    nr_pages += size % page_size ? 1 : 0;

    address = malloc(sizeof(void *) * nr_pages);
    if (!address) {
        fprintf(stderr, "WARNING:  <check_pages_in_numa_group> "
                        "failed in call to malloc(%i)\n", 
                sizeof(void*) * nr_pages);
        numa_bitmask_free(hbw_mask);
        return -2;
    }

    for (i = 0; i < nr_pages; ++i) {
        address[i] = ptr + i * pages_size;
    }

    move_pages(0, nr_pages, address, NULL, status, MPOL_MF_MOVE);

    if (!numa_bitmask_isbitset(nodmaask.n, status[0])) {
        err = -1;
    }
    for (i = 1; i < nr_pages && !err; ++i) {
        if (!numa_bitmask_isbitset(nodemask.n, status[i])) {
            err = i;
        }
    }
    free(adress);
    numa_bitmask_free(hbw_mask);
    return err;
            
}
int check_page_size(void *ptr, size_t size, size_t page_size)
{

}

