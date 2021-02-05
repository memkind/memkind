// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int err = 0;
    const size_t size = 1024 * 1024;
    void *buf = NULL;

    // It is expected that "malloc", "calloc", "realloc" or "posix_memalign"
    // argument is passed
    if (argc != 2) {
        printf("Error: Wrong number of parameters\n");
        err = -1;
        return err;
    }

    if (strcmp(argv[1], "malloc") == 0) {
        buf = malloc(size);
        if (buf == NULL) {
            printf("Error: malloc returned NULL\n");
            err = -1;
        }
        free(buf);
    } else if (strcmp(argv[1], "calloc") == 0) {
        buf = calloc(size, 1);
        if (buf == NULL) {
            printf("Error: calloc returned NULL\n");
            err = -1;
        }
        free(buf);
    } else if (strcmp(argv[1], "realloc") == 0) {
        buf = malloc(size);
        if (buf == NULL) {
            printf("Error: malloc before realloc returned NULL\n");
            err = -1;
        }
        buf = realloc(buf, size * 2);
        if (buf == NULL) {
            printf("Error: realloc returned NULL\n");
            err = -1;
        }
        free(buf);
    } else if (strcmp(argv[1], "posix_memalign") == 0) {
        err = posix_memalign(&buf, 64, size);
        if (err != 0) {
            printf("Error: posix_memalign returned %d\n", err);
        }
        free(buf);
    } else {
        printf("Error: unknown parameter\n");
        err = -1;
    }

    return err;
}
