// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2018 - 2020 Intel Corporation. */

#include <memkind.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static char path[PATH_MAX] = "/tmp/";
static const size_t PMEM_PART_SIZE = MEMKIND_PMEM_MIN_SIZE + 4 * 1024;

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
}

int main(int argc, char **argv)
{
    const size_t size = 512;
    struct memkind *pmem_kind = NULL;
    const int arraySize = 100;
    char *ptr[100] = {NULL};
    int i = 0;
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
        "This example shows how to use memkind_free with unknown kind as a parameter.\n");

    int status = memkind_check_dax_path(path);
    if (!status) {
        fprintf(stdout, "PMEM kind %s is on DAX-enabled file system.\n", path);
    } else {
        fprintf(stdout, "PMEM kind %s is not on DAX-enabled file system.\n",
                path);
    }

    err = memkind_create_pmem(path, PMEM_PART_SIZE, &pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    for (i = 0; i < arraySize; ++i) {
        if (i < 50) {
            ptr[i] = memkind_malloc(MEMKIND_DEFAULT, size);
            if (ptr[i] == NULL) {
                fprintf(stderr, "Unable to allocate memkind default.\n");
                return 1;
            }
        } else {
            ptr[i] = memkind_malloc(pmem_kind, size);
            if (ptr[i] == NULL) {
                fprintf(stderr, "Unable to allocate pmem.\n");
                return 1;
            }
        }
    }
    fprintf(
        stdout,
        "Memory was successfully allocated in default kind and pmem kind.\n");

    sprintf(ptr[10], "Hello world from standard memory - ptr[10].\n");
    sprintf(ptr[40], "Hello world from standard memory - ptr[40].\n");
    sprintf(ptr[80], "Hello world from persistent memory - ptr[80].\n");

    fprintf(stdout, "%s", ptr[10]);
    fprintf(stdout, "%s", ptr[40]);
    fprintf(stdout, "%s", ptr[80]);

    fprintf(stdout, "Free memory without specifying kind.\n");
    for (i = 0; i < arraySize; ++i) {
        memkind_free(NULL, ptr[i]);
    }

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    fprintf(stdout, "Memory was successfully released.\n");

    return 0;
}
