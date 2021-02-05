// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2018 - 2020 Intel Corporation. */

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
    struct memkind *pmem_kind_unlimited = NULL;
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
        "This example shows how to allocate memory with unlimited kind size."
        "\nPMEM kind directory: %s\n",
        path);

    int status = memkind_check_dax_path(path);
    if (!status) {
        fprintf(stdout, "PMEM kind %s is on DAX-enabled file system.\n", path);
    } else {
        fprintf(stdout, "PMEM kind %s is not on DAX-enabled file system.\n",
                path);
    }

    // Create PMEM partition with unlimited size
    err = memkind_create_pmem(path, 0, &pmem_kind_unlimited);
    if (err) {
        print_err_message(err);
        return 1;
    }

    char *pmem_str10 = NULL;
    char *pmem_str11 = NULL;

    // Huge allocation
    pmem_str10 = (char *)memkind_malloc(pmem_kind_unlimited, 32 * 1024 * 1024);
    if (pmem_str10 == NULL) {
        fprintf(stderr, "Unable to allocate pmem string (pmem_str10).\n");
        return 1;
    }

    // Another huge allocation, kind size is only limited by OS resources
    pmem_str11 = (char *)memkind_malloc(pmem_kind_unlimited, 32 * 1024 * 1024);
    if (pmem_str11 == NULL) {
        fprintf(stderr, "Unable to allocate pmem string (pmem_str11).\n");
        return 1;
    }

    memkind_free(pmem_kind_unlimited, pmem_str10);
    memkind_free(pmem_kind_unlimited, pmem_str11);

    err = memkind_destroy_kind(pmem_kind_unlimited);
    if (err) {
        print_err_message(err);
        return 1;
    }

    fprintf(stdout, "Memory was successfully allocated and released.\n");

    return 0;
}
