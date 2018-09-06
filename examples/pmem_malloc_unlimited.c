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

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define PMEM_MAX_SIZE (1024 * 1024 * 32)

char* PMEM_DIR = "/tmp/";

int main(int argc, char *argv[])
{
    struct memkind *pmem_kind_unlimited = NULL;
    int err = 0, opt = 0;
	struct stat st;

	while((opt = getopt(argc, argv, "hd:")) != -1) {
        switch (opt) {
            case 'd':
                PMEM_DIR = optarg;
                err = stat(PMEM_DIR, &st);
                 if (err != 0 || !S_ISDIR(st.st_mode)) {
                    printf("%s : Error in getting path status or"
                           "invalid or non-existent directory\n", PMEM_DIR);
                    return -1;
                }
                 break;
            case 'h':
                printf("\nMemkind options:\n"
                       "-d <directory_path>   change directory on which PMEM kinds\n"
                       "                      are created (default /tmp/)\n");
                break;
        }
    }

    printf("Memory allocation example with the use of unlimited kind size.\nPMEM kind directory: %s\n", PMEM_DIR);

    /* Create PMEM partition with unlimited size */
    err = memkind_create_pmem(PMEM_DIR, 0, &pmem_kind_unlimited);
    if (err) {
        perror("memkind_create_pmem()");
        fprintf(stderr, "Unable to create pmem partition\n");
        return errno ? -errno : 1;
    }

    char *pmem_str10 = NULL;
    char *pmem_str11 = NULL;

    /* Huge allocation */
    pmem_str10 = (char *)memkind_malloc(pmem_kind_unlimited, 32 * 1024 * 1024);
    if (pmem_str10 == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate pmem string (pmem_str10)\n");
        return errno ? -errno : 1;
    }

    /* Another huge allocation, kind size is only limited by OS resources */
    pmem_str11 = (char *)memkind_malloc(pmem_kind_unlimited, 32 * 1024 * 1024);
    if (pmem_str11 == NULL) {
        perror("memkind_malloc()");
        fprintf(stderr, "Unable to allocate pmem string (pmem_str11)\n");
        return errno ? -errno : 1;
    }

    memkind_free(pmem_kind_unlimited, pmem_str10);
    memkind_free(pmem_kind_unlimited, pmem_str11);

    err = memkind_destroy_kind(pmem_kind_unlimited);
    if (err) {
        perror("memkind_destroy_kind()");
        fprintf(stderr, "Unable to destroy pmem partition\n");
        return errno ? -errno : 1;
    }

    printf("Memory was successfully allocated and released.\n");

    return 0;
}