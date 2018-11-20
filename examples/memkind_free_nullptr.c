/*
 * Copyright (c) 2018 Intel Corporation
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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static char* PMEM_DIR = "/tmp/";
static const size_t PMEM_PART_SIZE = MEMKIND_PMEM_MIN_SIZE + 4 * 1024;

int main(int argc, char **argv)
{
    const size_t size = 512;
    struct memkind *pmem_kind = NULL;
    const int arraySize = 100;
    char *ptr[arraySize];

    int err = memkind_create_pmem(PMEM_DIR, PMEM_PART_SIZE, &pmem_kind);
    if (err) {
        perror("memkind_create_pmem()");
        fprintf(stderr, "Unable to create pmem partition err=%d errno=%d\n", err,
                errno);
        return errno ? -errno : 1;
    }

    // alloc default kinds
    int i = 0;

    for (i = 0; i < arraySize; ++i) {
        ptr[i] = NULL;
    }

    for (i = 0; i < arraySize; ++i) {
        if (i < 50) {
            ptr[i] = memkind_malloc(MEMKIND_DEFAULT, size);
        } else {
            ptr[i] = memkind_malloc(pmem_kind, size);
        }
    }
    printf("memkind default kind and pmem kind allocated\n");

    // free pointers whithout specifying kind
    for (i = 0; i < arraySize; ++i) {
        memkind_free(NULL, ptr[i]);
    }
    printf("memkind default kind and pmem kind freed\n");

    //destroy pmem kind before finish
    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        perror("memkind_destroy_kind()");
        fprintf(stderr, "Unable to destroy pmem partition\n");
        return errno ? -errno : 1;
    }

    return 0;
}
