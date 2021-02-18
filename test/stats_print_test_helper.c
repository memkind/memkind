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
    if (argc != 2) {
        printf("Error: Wrong number of parameters. Exactly one expected.\n");
        return -1;
    }

    int fd = -1;
    if (strcmp(argv[1], "default") == 0) {
        // default write_cb function should be called, write to stderr
        return memkind_stats_print(NULL, NULL, MEMKIND_STAT_PRINT_ALL);
    } else if (strcmp(argv[1], "stdout") == 0) {
        fd = STDOUT_FILENO;
        return memkind_stats_print(&write_cb, (void *)&fd, MEMKIND_STAT_PRINT_ALL);
    } else if (strcmp(argv[1], "no_write_cb") == 0) {
        // default write_cb function should be called, write to stderr
        fd = STDOUT_FILENO;
        return memkind_stats_print(NULL, (void *)&fd, MEMKIND_STAT_PRINT_ALL);
    } else if (strcmp(argv[1], "pass_opts") == 0) {
        // default write_cb function should be called, write to stderr
        void *ptr = memkind_malloc(MEMKIND_REGULAR, 1024);

        int ret = memkind_stats_print(NULL, NULL,
                                      MEMKIND_STAT_PRINT_JSON_FORMAT
                                      | MEMKIND_STAT_PRINT_OMIT_PER_ARENA
                                      | MEMKIND_STAT_PRINT_OMIT_EXTENT);

        memkind_free(MEMKIND_REGULAR, ptr);
        return ret;
    } else if (strcmp(argv[1], "all_opts") == 0) {
        const char *dir = "/tmp";
        size_t max_size = MEMKIND_PMEM_MIN_SIZE;
        struct memkind *kind = NULL;
        memkind_create_pmem(dir, max_size, &kind);
        void *ptr = memkind_malloc(kind, 1024);
        memkind_free(kind, ptr);
        memkind_destroy_kind(kind);
        memkind_create_pmem(dir, max_size, &kind);
        ptr = memkind_malloc(kind, 1024);

        int ret = memkind_stats_print(NULL, NULL,
                                      MEMKIND_STAT_PRINT_JSON_FORMAT
                                      | MEMKIND_STAT_PRINT_OMIT_GENERAL
                                      | MEMKIND_STAT_PRINT_OMIT_MERGED_ARENA
                                      | MEMKIND_STAT_PRINT_OMIT_DESTROYED_MERGED_ARENA
                                      | MEMKIND_STAT_PRINT_OMIT_PER_ARENA
                                      | MEMKIND_STAT_PRINT_OMIT_PER_SIZE_CLASS_BINS
                                      | MEMKIND_STAT_PRINT_OMIT_PER_SIZE_CLASS_LARGE
                                      | MEMKIND_STAT_PRINT_OMIT_MUTEX
                                      | MEMKIND_STAT_PRINT_OMIT_EXTENT);
        memkind_free(kind, ptr);
        memkind_destroy_kind(kind);
        return ret;
    } else if (strcmp(argv[1], "negative_test") == 0) {
        fd = -1; // wrong file descriptor
        return memkind_stats_print(&write_cb, (void *)&fd, MEMKIND_STAT_PRINT_ALL);
    } else if (strcmp(argv[1], "opts_negative_test") == 0) {
        int retcode = memkind_stats_print(NULL, NULL, 1024);
        return retcode == MEMKIND_ERROR_INVALID ? MEMKIND_SUCCESS :
               MEMKIND_ERROR_INVALID;
    }

    printf("Error: unknown parameter '%s'\n", argv[1]);
    return -1;
}
