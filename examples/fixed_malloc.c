// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <assert.h>
#include <memkind.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define FIXED_MAP_SIZE (1024 * 1024 * 32)

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
}

int main(int argc, char *argv[])
{
    struct memkind *fixed_kind = NULL;
    int err = 0;

    void *addr = mmap(NULL, FIXED_MAP_SIZE, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    assert(addr != MAP_FAILED);

    // Create PMEM partition with specific size
    err = memkind_create_fixed(addr, FIXED_MAP_SIZE, &fixed_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    char *fixed_str1 = NULL;
    char *fixed_str2 = NULL;
    char *fixed_str3 = NULL;
    char *fixed_str4 = NULL;

    // Allocate 512 Bytes of 32 MB available
    fixed_str1 = (char *)memkind_malloc(fixed_kind, 512);
    if (fixed_str1 == NULL) {
        fprintf(stderr, "Unable to allocate fixed string (fixed_str1).\n");
        return 1;
    }

    // Allocate 8 MB of 31.9 MB available
    fixed_str2 = (char *)memkind_malloc(fixed_kind, 8 * 1024 * 1024);
    if (fixed_str2 == NULL) {
        fprintf(stderr, "Unable to allocate fixed string (fixed_str2).\n");
        return 1;
    }

    // Allocate 16 MB of 23.9 MB available
    fixed_str3 = (char *)memkind_malloc(fixed_kind, 16 * 1024 * 1024);
    if (fixed_str3 == NULL) {
        fprintf(stderr, "Unable to allocate fixed string (fixed_str3).\n");
        return 1;
    }

    // Allocate 16 MB of 7.9 MB available - Out Of Memory expected
    fixed_str4 = (char *)memkind_malloc(fixed_kind, 16 * 1024 * 1024);
    if (fixed_str4 != NULL) {
        fprintf(
            stderr,
            "Failure, this allocation should not be possible (expected result was NULL).\n");
        return 1;
    }

    sprintf(fixed_str1, "Hello world from fixed - fixed_str1.\n");
    sprintf(fixed_str2, "Hello world from fixed - fixed_str2.\n");
    sprintf(fixed_str3, "Hello world from persistent memory - fixed_str3.\n");

    fprintf(stdout, "%s", fixed_str1);
    fprintf(stdout, "%s", fixed_str2);
    fprintf(stdout, "%s", fixed_str3);

    memkind_free(fixed_kind, fixed_str1);
    memkind_free(fixed_kind, fixed_str2);
    memkind_free(fixed_kind, fixed_str3);

    err = memkind_destroy_kind(fixed_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    fprintf(stdout, "Memory was successfully allocated and released.\n");

    munmap(addr, FIXED_MAP_SIZE);

    return 0;
}
