// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2018 - 2020 Intel Corporation. */

#include <memkind.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define PMEM_MAX_SIZE (1024 * 1024 * 32)

static char path[PATH_MAX] = "/tmp/";

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
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
        "This example shows how to use memkind alignment and how it affects allocations.\nPMEM kind directory: %s\n",
        path);

    int status = memkind_check_dax_path(path);
    if (!status) {
        fprintf(stdout, "PMEM kind %s is on DAX-enabled file system.\n", path);
    } else {
        fprintf(stdout, "PMEM kind %s is not on DAX-enabled file system.\n",
                path);
    }

    err = memkind_create_pmem(path, PMEM_MAX_SIZE, &pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    char *pmem_str10 = NULL;
    char *pmem_str11 = NULL;

    // Perform two allocations - 32 bytes
    pmem_str10 = (char *)memkind_malloc(pmem_kind, 32);
    if (pmem_str10 == NULL) {
        fprintf(stderr, "Unable to allocate pmem string (pmem_str10).\n");
        return 1;
    }

    pmem_str11 = (char *)memkind_malloc(pmem_kind, 32);
    if (pmem_str11 == NULL) {
        fprintf(stderr, "Unable to allocate pmem string (pmem_str11)\n");
        return 1;
    }

    // The allocations should be very close to each other in memory
    if (pmem_str11 - pmem_str10 != 32) {
        fprintf(stderr, "Something went wrong with allocations.\n");
        return 1;
    }

    memkind_free(pmem_kind, pmem_str10);
    memkind_free(pmem_kind, pmem_str11);

    // Perform two allocations - 32 bytes with alignment 64
    err = memkind_posix_memalign(pmem_kind, (void **)&pmem_str10, 64, 32);
    if (err) {
        fprintf(
            stderr,
            "Unable to allocate pmem string (pmem_str10) with alignment.\n");
        return 1;
    }

    err = memkind_posix_memalign(pmem_kind, (void **)&pmem_str11, 64, 32);
    if (err) {
        fprintf(
            stderr,
            "Unable to allocate pmem string (pmem_str11) with alignment.\n");
        return 1;
    }

    // The addresses of allocations are not close to each other in memory, they
    // are aligned to 64
    if (pmem_str11 - pmem_str10 != 64) {
        fprintf(stderr, "Something went wrong with alignment allocations.\n");
        return 1;
    }

    memkind_free(pmem_kind, pmem_str10);
    memkind_free(pmem_kind, pmem_str11);

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    fprintf(
        stdout,
        "The memory has been successfully allocated using memkind alignment.\n");

    return 0;
}
