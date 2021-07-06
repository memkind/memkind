// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind.h>
#include <stdio.h>
#include <string.h>

#define SIZE ((size_t)16777216)

int main(int argc, char *argv[])
{
    void *ptr = memkind_malloc(MEMKIND_DAX_KMEM_BALANCED, SIZE);

    if (!ptr) {
        printf("\nERROR: MEMKIND_DAX_KMEM_BALANCED is not supported\n");
        return -1;
    }
    memset(ptr, 0, SIZE);

    memkind_free(MEMKIND_DAX_KMEM_BALANCED, ptr);

    printf("\nOK: MEMKIND_DAX_KMEM_BALANCED is supported\n");

    return 0;
}
