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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 10
#define NUM_ALLOCS 100
static char *PMEM_DIR = "/tmp/";

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
}

struct arg_struct {
    int id;
    struct memkind *kind;
    int **ptr;
};

void *thread_onekind(void *arg);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[])
{
    struct memkind *pmem_kind_unlimited = NULL;
    int err = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [pmem_kind_dir_path]\n", argv[0]);
        return 1;
    } else if (argc == 2) {
        PMEM_DIR = argv[1];
    }

    fprintf(stdout,
            "This example shows how to use multithreading with one main pmem kind."
            "\nPMEM kind directory: %s\n",
            PMEM_DIR);

    // Create PMEM partition with unlimited size
    err = memkind_create_pmem(PMEM_DIR, 0, &pmem_kind_unlimited);
    if (err) {
        print_err_message(err);
        return 1;
    }

    // Create a few threads which will access to our main pmem_kind
    pthread_t pmem_threads[NUM_THREADS];
    int *pmem_tint[NUM_THREADS][NUM_ALLOCS];
    int t = 0, i = 0;

    struct arg_struct *args[NUM_THREADS];

    for (t = 0; t<NUM_THREADS; t++) {
        args[t] = malloc(sizeof(struct arg_struct));
        args[t]->id = t;
        args[t]->ptr = &pmem_tint[t][0];
        args[t]->kind = pmem_kind_unlimited;

        if (pthread_create(&pmem_threads[t], NULL, thread_onekind,
                           (void *)args[t])!= 0) {
            fprintf(stderr, "Unable to create a thread.\n");
            return 1;
        }
    }

    sleep(1);
    if (pthread_cond_broadcast(&cond) != 0) {
        fprintf(stderr, "Unable to broadcast a condition.\n");
        return 1;
    }

    for (t = 0; t < NUM_THREADS; t++) {
        if (pthread_join(pmem_threads[t], NULL) != 0) {
            fprintf(stderr, "Thread join failed.\n");
            return 1;
        }
    }

    // Check if we can read the values outside of threads and free resources
    for (t = 0; t < NUM_THREADS; t++) {
        for (i = 0; i < NUM_ALLOCS; i++) {
            if(*pmem_tint[t][i] != t) {
                fprintf(stderr,
                        "pmem_tint value has not been saved correctly in the thread.\n");
                return 1;
            }
            memkind_free(args[t]->kind, *(args[t]->ptr+i));
        }
        free(args[t]);
    }

    fprintf(stdout, "Threads successfully allocated memory in the PMEM kind.\n");

    return 0;
}

void *thread_onekind(void *arg)
{
    struct arg_struct *args = (struct arg_struct *)arg;
    int i;

    if (pthread_mutex_lock(&mutex) != 0) {
        fprintf(stderr, "Failed to acquire mutex.\n");
        return NULL;
    }
    if (pthread_cond_wait(&cond, &mutex) != 0) {
        fprintf(stderr, "Failed to block mutex on condition.\n");
        return NULL;
    }
    if (pthread_mutex_unlock(&mutex) != 0) {
        fprintf(stderr, "Failed to release mutex.\n");
        return NULL;
    }

    // Lets alloc int and put there thread ID
    for (i = 0; i < NUM_ALLOCS; i++) {
        *(args->ptr+i) = (int *)memkind_malloc(args->kind, sizeof(int));
        if (*(args->ptr+i) == NULL) {
            fprintf(stderr, "Unable to allocate pmem int.\n");
            return NULL;
        }
        **(args->ptr+i) = args->id;
    }

    return NULL;
}
