// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include <memkind.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    const size_t size = 512;
    char *default_str = NULL;
    char *hugetlb_str = NULL;
    char *hbw_str = NULL;
    char *hbw_hugetlb_str = NULL;
    char *hbw_preferred_str = NULL;
    char *hbw_preferred_hugetlb_str = NULL;
    char *hbw_interleave_str = NULL;
    char *highest_capacity_str = NULL;

    default_str = (char *)memkind_malloc(MEMKIND_DEFAULT, size);
    if (default_str == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate default string\n");
        return errno ? -errno : 1;
    }
    hugetlb_str = (char *)memkind_malloc(MEMKIND_HUGETLB, size);
    if (hugetlb_str == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate hugetlb string\n");
        return errno ? -errno : 1;
    }
    hbw_str = (char *)memkind_malloc(MEMKIND_HBW, size);
    if (hbw_str == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate hbw string\n");
        return errno ? -errno : 1;
    }
    hbw_hugetlb_str = (char *)memkind_malloc(MEMKIND_HBW_HUGETLB, size);
    if (hbw_hugetlb_str == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate hbw_hugetlb string\n");
        return errno ? -errno : 1;
    }
    hbw_preferred_str = (char *)memkind_malloc(MEMKIND_HBW_PREFERRED, size);
    if (hbw_preferred_str == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate hbw_preferred string\n");
        return errno ? -errno : 1;
    }
    hbw_preferred_hugetlb_str =
        (char *)memkind_malloc(MEMKIND_HBW_PREFERRED_HUGETLB, size);
    if (hbw_preferred_hugetlb_str == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate hbw_preferred_hugetlb string\n");
        return errno ? -errno : 1;
    }
    hbw_interleave_str = (char *)memkind_malloc(MEMKIND_HBW_INTERLEAVE, size);
    if (hbw_interleave_str == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate hbw_interleave string\n");
        return errno ? -errno : 1;
    }
    highest_capacity_str =
        (char *)memkind_malloc(MEMKIND_HIGHEST_CAPACITY, size);
    if (highest_capacity_str == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate highest_capacity string\n");
        return errno ? -errno : 1;
    }

    sprintf(default_str, "Hello world from standard memory\n");
    sprintf(hugetlb_str, "Hello world from standard memory with 2 MB pages\n");
    sprintf(hbw_str, "Hello world from high bandwidth memory\n");
    sprintf(hbw_hugetlb_str,
            "Hello world from high bandwidth 2 MB paged memory\n");
    sprintf(
        hbw_preferred_str,
        "Hello world from high bandwidth memory if sufficient resources exist\n");
    sprintf(
        hbw_preferred_hugetlb_str,
        "Hello world from high bandwidth 2 MB paged memory if sufficient resources exist\n");
    sprintf(hbw_interleave_str,
            "Hello world from high bandwidth interleaved memory\n");
    sprintf(highest_capacity_str, "Hello world from highest capacity memory\n");

    fprintf(stdout, "%s", default_str);
    fprintf(stdout, "%s", hugetlb_str);
    fprintf(stdout, "%s", hbw_str);
    fprintf(stdout, "%s", hbw_hugetlb_str);
    fprintf(stdout, "%s", hbw_preferred_str);
    fprintf(stdout, "%s", hbw_preferred_hugetlb_str);
    fprintf(stdout, "%s", hbw_interleave_str);
    fprintf(stdout, "%s", highest_capacity_str);

    memkind_free(MEMKIND_HBW_INTERLEAVE, hbw_interleave_str);
    memkind_free(MEMKIND_HBW_PREFERRED_HUGETLB, hbw_preferred_hugetlb_str);
    memkind_free(MEMKIND_HBW_PREFERRED, hbw_preferred_str);
    memkind_free(MEMKIND_HBW_HUGETLB, hbw_hugetlb_str);
    memkind_free(MEMKIND_HBW, hbw_str);
    memkind_free(MEMKIND_HUGETLB, hugetlb_str);
    memkind_free(MEMKIND_DEFAULT, default_str);
    memkind_free(MEMKIND_HIGHEST_CAPACITY, highest_capacity_str);

    return 0;
}
