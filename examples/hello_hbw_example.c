// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include <hbwmalloc.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    const size_t size = 512;
    char *default_str = NULL;
    char *hbw_str = NULL;
    char *hbw_hugetlb_str = NULL;
    int err = 0;

    default_str = (char *)malloc(size);
    if (default_str == NULL) {
        perror("malloc()");
        fprintf(stderr, "Unable to allocate default string\n");
        err = errno ? -errno : 1;
        goto exit;
    }
    hbw_str = (char *)hbw_malloc(size);
    if (hbw_str == NULL) {
        perror("hbw_malloc()");
        fprintf(stderr, "Unable to allocate hbw string\n");
        err = errno ? -errno : 1;
        goto exit;
    }
    err = hbw_posix_memalign_psize((void **)&hbw_hugetlb_str, 2097152, size,
                                   HBW_PAGESIZE_2MB);
    if (err) {
        perror("hbw_posix_memalign()");
        fprintf(stderr, "Unable to allocate hbw hugetlb string\n");
        err = errno ? -errno : 1;
        goto exit;
    }

    sprintf(default_str, "Hello world from standard memory\n");
    sprintf(hbw_str, "Hello world from high bandwidth memory\n");
    sprintf(hbw_hugetlb_str,
            "Hello world from high bandwidth 2 MB paged memory\n");

    fprintf(stdout, "%s", default_str);
    fprintf(stdout, "%s", hbw_str);
    fprintf(stdout, "%s", hbw_hugetlb_str);

exit:
    if (hbw_hugetlb_str) {
        hbw_free(hbw_hugetlb_str);
    }
    if (hbw_str) {
        hbw_free(hbw_str);
    }
    if (default_str) {
        free(default_str);
    }
    return err;
}
