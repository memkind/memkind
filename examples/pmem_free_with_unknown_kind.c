/*
 * Copyright (c) 2018 - 2019 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memkind.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static char path[PATH_MAX]="/tmp/";
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
    char *ptr[100] = { NULL };
    int i = 0;
    int err = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [pmem_kind_dir_path]\n", argv[0]);
        return 1;
    } else if (argc == 2 && (realpath(argv[1], path) == NULL)) {
        fprintf(stderr, "Incorrect pmem_kind_dir_path %s\n", argv[1]);
        return 1;
    }

    fprintf(stdout,
            "This example shows how to use memkind_free with unknown kind as a parameter.\n");

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
    fprintf(stdout,
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
