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
    struct memkind *pmem_kinds[10], *pmem_kind, *pmem_kind_unlimited;
    int err;

    /* create PMEM partition with specific size */
    err = memkind_create_pmem(".", PMEM_MAX_SIZE, &pmem_kind);
    if (err) {
        perror("memkind_create_pmem()");
        fprintf(stderr, "Unable to create pmem partition\n");
        return errno ? -errno : 1;
    }

    /* create PMEM partition with unlimited size */
    err = memkind_create_pmem(".", 0, &pmem_kind_unlimited);
    if (err) {
        perror("memkind_create_pmem()");
        fprintf(stderr, "Unable to create pmem partition\n");
        return errno ? -errno : 1;
    }

    /* and delete them */
    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        perror("memkind_destroy_kind()");
        fprintf(stderr, "Unable to destroy pmem partition\n");
        return errno ? -errno : 1;
    }

    err = memkind_destroy_kind(pmem_kind_unlimited);
    if (err) {
        perror("memkind_destroy_kind()");
        fprintf(stderr, "Unable to destroy pmem partition\n");
        return errno ? -errno : 1;
    }	

    /* create many PMEM kinds */
    for(int i=0; i<10; i++)
    {
        err = memkind_create_pmem(".", MEMKIND_PMEM_MIN_SIZE, &pmem_kinds[i]);
        if (err) {
            perror("memkind_create_pmem()");
            fprintf(stderr, "Unable to create pmem partition\n");
            return errno ? -errno : 1;
        }
    }

    /* and delete them */
    for(int i=0; i<10; i++)
    {
        err = memkind_destroy_kind(pmem_kinds[i]);
        if (err) {
            perror("memkind_pmem_destroy()");
            fprintf(stderr, "Unable to destroy pmem partition\n");
            return errno ? -errno : 1;
        }
    }

    return 0;
}