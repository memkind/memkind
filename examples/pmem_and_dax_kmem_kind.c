// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include <memkind.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static char path[PATH_MAX] = "/tmp/";

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
}

int main(int argc, char *argv[])
{
    const size_t size = 512;
    struct memkind *pmem_kind = NULL;
    int err = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [pmem_kind_dir_path]\n", argv[0]);
        return 1;
    } else if (argc == 2) {
        if (realpath(argv[1], path) == NULL) {
            fprintf(stderr, "Incorrect pmem_kind_dir_path %s\n", argv[1]);
            return 1;
        }
    }

    char *ptr_dax_kmem = NULL;
    char *ptr_pmem = NULL;

    fprintf(
        stdout,
        "This example shows how to allocate to PMEM memory using file-backed memory (pmem kind) "
        "and persistent memory NUMA node (MEMKIND_DAX_KMEM).\nPMEM kind directory: %s\n",
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

    ptr_dax_kmem = (char *)memkind_malloc(MEMKIND_DAX_KMEM, size);
    if (!ptr_dax_kmem) {
        fprintf(stderr,
                "Unable allocate 512 bytes in persistent memory NUMA node.\n");
        return 1;
    }

    ptr_pmem = (char *)memkind_malloc(pmem_kind, size);
    if (!ptr_pmem) {
        fprintf(stderr, "Unable allocate 512 bytes in file-backed memory.\n");
        return 1;
    }

    snprintf(ptr_dax_kmem, size,
             "Hello world from persistent memory NUMA node - ptr_dax_kmem.\n");
    snprintf(ptr_pmem, size,
             "Hello world from file-backed memory - ptr_pmem.\n");

    fprintf(stdout, "%s", ptr_dax_kmem);
    fprintf(stdout, "%s", ptr_pmem);

    memkind_free(MEMKIND_DAX_KMEM, ptr_dax_kmem);
    memkind_free(pmem_kind, ptr_pmem);

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    fprintf(stdout, "Memory was successfully allocated and released.\n");

    return 0;
}
