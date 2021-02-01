// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "memkind.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int err = 0;

    if (argc != 2) {
        printf("Error: Wrong number of parameters. Exactly one expected.\n");
        err = -1;
        return err;
    }

    if (strcmp(argv[1], "default") == 0) {
        memkind_stats_print(NULL, NULL);
    } else {
        printf("Error: unknown parameter '%s'\n", argv[1]);
        err = -1;
    }

    return err;
}
