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
#include <pthread.h>

#define PMEM_MAX_SIZE	(MEMKIND_PMEM_MIN_SIZE * 2)

struct arg_struct
{
    int id;
    struct memkind *kind;
    int **ptr;
};
void *thread_onekind(void *arg);
void *thread_ind(void *arg);

int
main(int argc, char *argv[])
{
    struct memkind *pmem_kind_unlimited;
    int err;

    /* create PMEM partition with unlimited size */
    err = memkind_create_pmem(".", 0, &pmem_kind_unlimited);
    if (err) {
        perror("memkind_create_pmem()");
        fprintf(stderr, "Unable to create pmem partition\n");
        return errno ? -errno : 1;
    }

    /* Create a few threads which will access to our main pmem_kind */
    pthread_t pmem_threads[10];
    int *pmem_tint[10];

    struct arg_struct *args[10];

    for(int t = 0; t<10; t++)
    {
        args[t] = malloc(sizeof(struct arg_struct));
        args[t]->id = t;
        args[t]->ptr = &pmem_tint[t];
        args[t]->kind = pmem_kind_unlimited;

        pthread_create(&pmem_threads[t], NULL, thread_onekind, (void *)args[t]);
	}

    for(int t = 0; t<10; t++)
        pthread_join(pmem_threads[t], NULL);

    /* Check if we can read the values outside of threads and free resources */
    for(int t=0; t<10; t++)
    {
        if(*pmem_tint[t] != t)
        {
            perror("read thread memkind_malloc()");
            fprintf(stderr, "read thread memkind_malloc()\n");
            return 1;
        }
        memkind_free(args[t]->kind, *(args[t]->ptr));
        free(args[t]);
    }

    /* Lets create many independent threads */
    for(int t = 0; t<10; t++)
        pthread_create(&pmem_threads[t], NULL, thread_ind, NULL);

    for(int t = 0; t<10; t++)
        pthread_join(pmem_threads[t], NULL);

    err = memkind_destroy_kind(pmem_kind_unlimited);
    if (err) {
        perror("memkind_destroy_kind()");
        fprintf(stderr, "Unable to destroy pmem partition\n");
        return errno ? -errno : 1;
    }

    return 0;
}

void *thread_onekind(void *arg)
{
    struct arg_struct *args = arg;

    /* Lets alloc int and put there thread ID */
    *(args->ptr) = (int *)memkind_malloc(args->kind, sizeof(int));
    if(*(args->ptr) == NULL)
    {
        perror("thread memkind_malloc()");
        fprintf(stderr, "Unable to allocate pmem int\n");
        return NULL;
    }

    **(args->ptr) = args->id;

    return NULL;
}

void *thread_ind(void *arg)
{
    struct memkind *pmem_kind;

    /* Create a pmem kind in thread */
    int err = memkind_create_pmem(".", PMEM_MAX_SIZE, &pmem_kind);
    if (err) {
        perror("thread memkind_create_pmem()");
        fprintf(stderr, "Unable to create pmem partition\n");
        return NULL;
    }

    /* Alloc something */
    void *test = memkind_malloc(pmem_kind, 32);
    if (test == NULL) {
        perror("thread memkind_malloc()");
        fprintf(stderr, "Unable to allocate pmem (test)\n");
        return NULL;
    }

    /* Free resources */
    memkind_free(pmem_kind, test);

    /* And destroy pmem kind */
    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        perror("thread memkind_pmem_destroy()");
        fprintf(stderr, "Unable to destroy pmem partition\n");
        return NULL;
    }

    return NULL;
}