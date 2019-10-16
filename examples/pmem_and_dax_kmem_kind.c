/*
 * Copyright (c) 2019 Intel Corporation
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

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
}

int main(int argc, char *argv[])
{
    const size_t size = 512;
    struct memkind *pmem_kind = NULL;
    int err = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [pmem_kind_dir_path]\n", argv[0]);
        return 1;
    } else if (argc == 2) {
        if (realpath(argv[1], path) == NULL) {
            fprintf(stderr, "Incorrect pmem_kind_dir_path %s\n", argv[1]);
            return 1;
        }
    }

    char *ptr_dax_kmem = NULL;
    char *ptr_pmem = NULL;

    fprintf(stdout,
            "This example shows how to allocate to PMEM memory using file-backed memory (pmem kind) "
            "and persistent memory NUMA node (MEMKIND_DAX_KMEM).\nPMEM kind directory: %s\n",
            path);

    err = memkind_create_pmem(path, 0, &pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    ptr_dax_kmem = (char *)memkind_malloc(MEMKIND_DAX_KMEM, size);
    if (!ptr_dax_kmem) {
        fprintf(stderr, "Unable allocate 512 bytes in persistent memory NUMA node.\n");
        return 1;
    }

    ptr_pmem = (char *)memkind_malloc(pmem_kind, size);
    if (!ptr_pmem) {
        fprintf(stderr, "Unable allocate 512 bytes in file-backed memory.\n");
        return 1;
    }

    snprintf(ptr_dax_kmem, size,
             "Hello world from persistent memory NUMA node - ptr_dax_kmem.\n");
    snprintf(ptr_pmem, size, "Hello world from file-backed memory - ptr_pmem.\n");

    fprintf(stdout, "%s", ptr_dax_kmem);
    fprintf(stdout, "%s", ptr_pmem);

    memkind_free(MEMKIND_DAX_KMEM, ptr_dax_kmem);
    memkind_free(pmem_kind, ptr_pmem);

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
        return 1;
    }

    fprintf(stdout, "Memory was successfully allocated and released.\n");

    return 0;
}
