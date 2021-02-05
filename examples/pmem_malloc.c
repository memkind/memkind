// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */

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
        "This example shows how to allocate memory and possibility to exceed pmem kind size."
        "\nPMEM kind directory: %s\n",
        path);

    int status = memkind_check_dax_path(path);
    if (!status) {
        fprintf(stdout, "PMEM kind %s is on DAX-enabled file system.\n", path);
    } else {
        fprintf(stdout, "PMEM kind %s is not on DAX-enabled file system.\n",
                path);
    }

    // Create PMEM partition with specific size
    err = memkind_create_pmem(path, PMEM_MAX_SIZE, &pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    char *pmem_str1 = NULL;
    char *pmem_str2 = NULL;
    char *pmem_str3 = NULL;
    char *pmem_str4 = NULL;

    // Allocate 512 Bytes of 32 MB available
    pmem_str1 = (char *)memkind_malloc(pmem_kind, 512);
    if (pmem_str1 == NULL) {
        fprintf(stderr, "Unable to allocate pmem string (pmem_str1).\n");
        return 1;
    }

    // Allocate 8 MB of 31.9 MB available
    pmem_str2 = (char *)memkind_malloc(pmem_kind, 8 * 1024 * 1024);
    if (pmem_str2 == NULL) {
        fprintf(stderr, "Unable to allocate pmem string (pmem_str2).\n");
        return 1;
    }

    // Allocate 16 MB of 23.9 MB available
    pmem_str3 = (char *)memkind_malloc(pmem_kind, 16 * 1024 * 1024);
    if (pmem_str3 == NULL) {
        fprintf(stderr, "Unable to allocate pmem string (pmem_str3).\n");
        return 1;
    }

    // Allocate 16 MB of 7.9 MB available - Out Of Memory expected
    pmem_str4 = (char *)memkind_malloc(pmem_kind, 16 * 1024 * 1024);
    if (pmem_str4 != NULL) {
        fprintf(
            stderr,
            "Failure, this allocation should not be possible (expected result was NULL).\n");
        return 1;
    }

    sprintf(pmem_str1, "Hello world from pmem - pmem_str1.\n");
    sprintf(pmem_str2, "Hello world from pmem - pmem_str2.\n");
    sprintf(pmem_str3, "Hello world from persistent memory - pmem_str3.\n");

    fprintf(stdout, "%s", pmem_str1);
    fprintf(stdout, "%s", pmem_str2);
    fprintf(stdout, "%s", pmem_str3);

    memkind_free(pmem_kind, pmem_str1);
    memkind_free(pmem_kind, pmem_str2);
    memkind_free(pmem_kind, pmem_str3);

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    fprintf(stdout, "Memory was successfully allocated and released.\n");

    return 0;
}
