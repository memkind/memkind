// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2018 - 2020 Intel Corporation. */

#include <memkind.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define PMEM_MAX_SIZE (1024 * 1024 * 32)
#define NUM_KINDS 10

static char path[PATH_MAX] = "/tmp/";

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
}

int main(int argc, char *argv[])
{
    struct memkind *pmem_kinds[NUM_KINDS] = {NULL};
    struct memkind *pmem_kind = NULL;
    struct memkind *pmem_kind_unlimited = NULL;

    int err = 0, i = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [pmem_kind_dir_path]\n", argv[0]);
        return 1;
    } else if (argc == 2 && (realpath(argv[1], path) == NULL)) {
        fprintf(stderr, "Incorrect pmem_kind_dir_path %s\n", argv[1]);
        return 1;
    }

    fprintf(
        stdout,
        "This example shows how to create and destroy pmem kind with defined or unlimited size."
        "\nPMEM kind directory: %s\n",
        path);

    int status = memkind_check_dax_path(path);
    if (!status) {
        fprintf(stdout, "PMEM kind %s is on DAX-enabled file system.\n", path);
    } else {
        fprintf(stdout, "PMEM kind %s is not on DAX-enabled file system.\n",
                path);
    }

    // Create first PMEM partition with specific size
    err = memkind_create_pmem(path, PMEM_MAX_SIZE, &pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    // Create second PMEM partition with unlimited size
    err = memkind_create_pmem(path, 0, &pmem_kind_unlimited);
    if (err) {
        print_err_message(err);
        return 1;
    }

    // Destroy both PMEM partitions
    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    err = memkind_destroy_kind(pmem_kind_unlimited);
    if (err) {
        print_err_message(err);
        return 1;
    }

    // Create many PMEM kinds with same specific size
    for (i = 0; i < NUM_KINDS; i++) {
        err = memkind_create_pmem(path, PMEM_MAX_SIZE, &pmem_kinds[i]);
        if (err) {
            print_err_message(err);
            return 1;
        }
    }

    // Destroy all of them
    for (i = 0; i < NUM_KINDS; i++) {
        err = memkind_destroy_kind(pmem_kinds[i]);
        if (err) {
            print_err_message(err);
            return 1;
        }
    }

    fprintf(stdout,
            "PMEM kinds have been successfully created and destroyed.\n");

    return 0;
}
