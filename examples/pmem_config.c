// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

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
        "This example shows how to use custom configuration to create pmem kind."
        "\nPMEM kind directory: %s\n",
        path);

    struct memkind_config *test_cfg = memkind_config_new();
    if (!test_cfg) {
        fprintf(stderr, "Unable to create memkind cfg.\n");
        return 1;
    }

    memkind_config_set_path(test_cfg, path);
    memkind_config_set_size(test_cfg, PMEM_MAX_SIZE);
    memkind_config_set_memory_usage_policy(
        test_cfg, MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE);

    int status = memkind_check_dax_path(path);
    if (!status) {
        fprintf(stdout, "PMEM kind %s is on DAX-enabled file system.\n", path);
    } else {
        fprintf(stdout, "PMEM kind %s is not on DAX-enabled file system.\n",
                path);
    }

    // Create PMEM partition with specific configuration
    err = memkind_create_pmem_with_config(test_cfg, &pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    memkind_config_delete(test_cfg);

    fprintf(
        stdout,
        "PMEM kind and configuration was successfully created and destroyed.\n");

    return 0;
}
