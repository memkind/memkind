/*
 * Copyright (c) 2015 - 2018 Intel Corporation
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
#include <memkind/internal/memkind_pmem.h>

#include <sys/param.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define PMEM_MAX_SIZE	(MEMKIND_PMEM_MIN_SIZE * 2)

int
main(int argc, char *argv[])
{
    struct memkind *pmem_kind;
    int err;

    /* create PMEM partition with PMEM_MAX_SIZE size */
    err = memkind_create_pmem(".", PMEM_MAX_SIZE, &pmem_kind);
    if (err) {
        perror("memkind_create_pmem()");
        fprintf(stderr, "Unable to create pmem partition\n");
        return errno ? -errno : 1;
    }

    char *pmem_str10 = NULL;
    char *pmem_str11 = NULL;

    /* Lets make two 32 bytes allocations */
    pmem_str10 = (char *)memkind_malloc(pmem_kind, 32);
    if (pmem_str10 == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate pmem string (pmem_str10)\n");
        return errno ? -errno : 1;
    }

    pmem_str11 = (char *)memkind_malloc(pmem_kind, 32);
    if (pmem_str11 == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate pmem string (pmem_str11)\n");
        return errno ? -errno : 1;
    }

    /* Probably they will be very close to each other in memory */
    if(pmem_str11 - pmem_str10 != 32)
    {
        fprintf(stderr, "Something went wrong\n");
        return 1;
    }

    memkind_free(pmem_kind, pmem_str10);
    memkind_free(pmem_kind, pmem_str11);

    /* Lets make two 32 bytes allocations with alignment 64 this time */
    err = memkind_posix_memalign(pmem_kind, (void **)&pmem_str10, 64, 32);
    if (err) {
        perror("memkind_posix_memalign()");
        fprintf(stderr, "Unable to allocate pmem string (pmem_str10) with alignment\n");
        return errno ? -errno : 1;
    }

    err = memkind_posix_memalign(pmem_kind, (void **)&pmem_str11, 64, 32);
    if (err) {
        perror("memkind_posix_memalign()");
        fprintf(stderr, "Unable to allocate pmem string (pmem_str11) with alignment\n");
        return errno ? -errno : 1;
    }

    /* This time addresses are not close to each other, they are aligned to 64 */
    if(pmem_str11 - pmem_str10 != 64)
    {
        fprintf(stderr, "Something went wrong with alignment allocation\n");
        return 1;
    }

    memkind_free(pmem_kind, pmem_str10);
    memkind_free(pmem_kind, pmem_str11);

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        perror("memkind_destroy_kind()");
        fprintf(stderr, "Unable to destroy pmem partition\n");
        return errno ? -errno : 1;
    }

    return 0;
}