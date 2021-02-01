// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "memkind.h"

#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

/**
 * return value can be one of the following:
 * 0 - on success
 * 1 - on wrong number of parameters passed to this binary
 * 2 - on unsupported parameter passed to this binary
 * 3 - on memkind_stats_print() function failure
 */

void write_cb(void *cbopaque, const char *s)
{
    int fd = *((int *)cbopaque);
    syscall(SYS_write, fd, s, strlen(s));
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Error: Wrong number of parameters. Exactly one expected.\n");
        return 1;
    }

    int fd = 0;
    if (strcmp(argv[1], "default") == 0) {
        // default write_cb function should be called, write to stderr
        memkind_stats_print(NULL, NULL, MEMKIND_STAT_PRINT_ALL);
    } else if (strcmp(argv[1], "stdout") == 0) {
        fd = 1; // write to stdout
        memkind_stats_print(&write_cb, (void *)&fd, MEMKIND_STAT_PRINT_ALL);
    } else if (strcmp(argv[1], "no_write_cb") == 0) {
        // default write_cb function should be called, write to stderr
        fd = 1; // write to stdout
        memkind_stats_print(NULL, (void *)&fd, MEMKIND_STAT_PRINT_ALL);
    } else if (strcmp(argv[1], "pass_opts") == 0) {
        // default write_cb function should be called, write to stderr
        memkind_stats_print(NULL, NULL,
                            MEMKIND_STAT_PRINT_JSON_FORMAT | MEMKIND_STAT_PRINT_OMIT_GENERAL |
                            MEMKIND_STAT_PRINT_OMIT_PER_SIZE_CLASS_LARGE);
    } else if (strcmp(argv[1], "negative_test") == 0) {
        fd = -1; // wrong file descriptor
        memkind_stats_print(&write_cb, (void *)&fd, MEMKIND_STAT_PRINT_ALL);
    } else if (strcmp(argv[1], "opts_negative_test") == 0) {
        int retcode = memkind_stats_print(NULL, NULL, 1024);
        if (retcode == MEMKIND_ERROR_INVALID) {
            return 3;
        }
    } else {
        printf("Error: unknown parameter '%s'\n", argv[1]);
        return 2;
    }

    return 0;
}
