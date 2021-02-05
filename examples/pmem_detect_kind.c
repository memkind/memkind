// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include <memkind.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static char path[PATH_MAX] = "/tmp/";

#define MALLOC_SIZE 512U
#define REALLOC_SIZE 2048U
#define ALLOC_LIMIT 1000U

static void *alloc_buffer[ALLOC_LIMIT];

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
}

static int allocate_pmem_and_default_kind(struct memkind *pmem_kind)
{
    unsigned i;
    for (i = 0; i < ALLOC_LIMIT; i++) {
        if (i % 2)
            alloc_buffer[i] = memkind_malloc(pmem_kind, MALLOC_SIZE);
        else
            alloc_buffer[i] = memkind_malloc(MEMKIND_DEFAULT, MALLOC_SIZE);

        if (!alloc_buffer[i]) {
            return 1;
        }
    }

    return 0;
}

static int realloc_using_get_kind_only_on_pmem()
{
    unsigned i;
    for (i = 0; i < ALLOC_LIMIT; i++) {
        if (memkind_detect_kind(alloc_buffer[i]) != MEMKIND_DEFAULT) {
            void *temp = memkind_realloc(NULL, alloc_buffer[i], REALLOC_SIZE);
            if (!temp) {
                return 1;
            }
            alloc_buffer[i] = temp;
        }
    }

    return 0;
}

static int verify_allocation_size(struct memkind *pmem_kind, size_t pmem_size)
{
    unsigned i;
    for (i = 0; i < ALLOC_LIMIT; i++) {
        void *val = alloc_buffer[i];
        if (i % 2) {
            if (memkind_malloc_usable_size(pmem_kind, val) != pmem_size) {
                return 1;
            }
        } else {
            if (memkind_malloc_usable_size(MEMKIND_DEFAULT, val) !=
                MALLOC_SIZE) {
                return 1;
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{

    struct memkind *pmem_kind = NULL;
    int err = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [pmem_kind_dir_path]\n", argv[0]);
        return 1;
    } else if (argc == 2 && (realpath(argv[1], path) == NULL)) {
        fprintf(stderr, "Incorrect pmem_kind_dir_path %s\n", argv[1]);
        return 1;
    }

    fprintf(
        stdout,
        "This example shows how to distinguish allocation from different kinds using detect kind function"
        "\nPMEM kind directory: %s\n",
        path);

    int status = memkind_check_dax_path(path);
    if (!status) {
        fprintf(stdout, "PMEM kind %s is on DAX-enabled file system.\n", path);
    } else {
        fprintf(stdout, "PMEM kind %s is not on DAX-enabled file system.\n",
                path);
    }

    err = memkind_create_pmem(path, 0, &pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    fprintf(stdout, "Allocate to PMEM and DEFAULT kind.\n");

    if (allocate_pmem_and_default_kind(pmem_kind)) {
        fprintf(stderr, "allocate_pmem_and_default_kind().\n");
        return 1;
    }

    if (verify_allocation_size(pmem_kind, MALLOC_SIZE)) {
        fprintf(stderr, "verify_allocation_size() before resize.\n");
        return 1;
    }

    fprintf(
        stdout,
        "Reallocate memory only on PMEM kind using memkind_detect_kind().\n");

    if (realloc_using_get_kind_only_on_pmem()) {
        fprintf(stderr, "realloc_using_get_kind_only_on_pmem().\n");
        return 1;
    }

    if (verify_allocation_size(pmem_kind, REALLOC_SIZE)) {
        fprintf(stderr, "verify_allocation_size() after resize.\n");
        return 1;
    }

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    fprintf(stdout, "Memory from PMEM kind was successfully reallocated.\n");

    return 0;
}
