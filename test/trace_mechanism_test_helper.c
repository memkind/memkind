// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */
#include "memkind.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int err = 0;
    const size_t size = 1024 * 1024;
    void *buf = NULL;

    // It is expected that "MEMKIND_HBW" or "MEMKIND_HUGETLB" argument is passed
    if (argc != 2) {
        printf("Error: Wrong number of parameters\n");
        err = -1;
        return err;
    }

    if (strcmp(argv[1], "MEMKIND_HBW") == 0) {
        buf = memkind_malloc(MEMKIND_HBW, size);
        if (buf == NULL) {
            printf("Allocation of MEMKIND_HBW failed\n");
            err = -1;
        }
        memkind_free(MEMKIND_HBW, buf);
    } else if (strcmp(argv[1], "MEMKIND_HUGETLB") == 0) {
        buf = memkind_malloc(MEMKIND_HUGETLB, size);
        if (buf == NULL) {
            printf("Allocation of MEMKIND_HUGETLB failed\n");
            err = -1;
        }
        memkind_free(MEMKIND_HUGETLB, buf);
    } else {
        printf("Error: unknown parameter\n");
        err = -1;
    }

    return err;
}
