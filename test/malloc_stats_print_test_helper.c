// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "memkind.h"

#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

void write_cb(void *cbopaque, const char *s)
{
    int fd = *((int *)cbopaque);
    syscall(SYS_write, fd, s, strlen(s));
}

int main(int argc, char *argv[])
{
    int err = 0;

    if (argc != 2) {
        printf("Error: Wrong number of parameters. Exactly one expected.\n");
        err = -1;
        return err;
    }

    if (strcmp(argv[1], "default") == 0) {
        // default write_cb function should be called, write to stderr
        memkind_stats_print(NULL, NULL, NONE);
    } else if (strcmp(argv[1], "stdout") == 0) {
        int fd = 1; // write to stdout
        memkind_stats_print(&write_cb, (void *)&fd, NONE);
    } else if (strcmp(argv[1], "no_write_cb") == 0) {
        // default write_cb function should be called, write to stderr
        int fd = 1; // write to stdout
        memkind_stats_print(NULL, (void *)&fd, NONE);
    } else if (strcmp(argv[1], "pass_opts") == 0) {
        // default write_cb function should be called, write to stderr
        memkind_stats_print(NULL, NULL,
                            JSON_FORMAT | OMIT_GENERAL | OMIT_PER_SIZE_CLASS_LARGE);
    } else if (strcmp(argv[1], "negative_test") == 0) {
        int fd = -1; // wrong file descriptor
        memkind_stats_print(&write_cb, (void *)&fd, NONE);
    } else {
        printf("Error: unknown parameter '%s'\n", argv[1]);
        err = -1;
    }

    return err;
}
