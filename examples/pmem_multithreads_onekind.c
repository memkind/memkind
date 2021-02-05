// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2018 - 2020 Intel Corporation. */

#include <memkind.h>

#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 10
#define NUM_ALLOCS 100
static char path[PATH_MAX] = "/tmp/";

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
    } else if (argc == 2 && (realpath(argv[1], path) == NULL)) {
        fprintf(stderr, "Incorrect pmem_kind_dir_path %s\n", argv[1]);
        return 1;
    }

    fprintf(
        stdout,
        "This example shows how to use multithreading with one main pmem kind."
        "\nPMEM kind directory: %s\n",
        path);

    int status = memkind_check_dax_path(path);
    if (!status) {
        fprintf(stdout, "PMEM kind %s is on DAX-enabled file system.\n", path);
    } else {
        fprintf(stdout, "PMEM kind %s is not on DAX-enabled file system.\n",
                path);
    }

    // Create PMEM partition with unlimited size
    err = memkind_create_pmem(path, 0, &pmem_kind_unlimited);
    if (err) {
        print_err_message(err);
        return 1;
    }

    // Create a few threads which will access to our main pmem_kind
    pthread_t pmem_threads[NUM_THREADS];
    int *pmem_tint[NUM_THREADS][NUM_ALLOCS];
    int t = 0, i = 0;

    struct arg_struct *args[NUM_THREADS];

    for (t = 0; t < NUM_THREADS; t++) {
        args[t] = malloc(sizeof(struct arg_struct));
        args[t]->id = t;
        args[t]->ptr = &pmem_tint[t][0];
        args[t]->kind = pmem_kind_unlimited;

        if (pthread_create(&pmem_threads[t], NULL, thread_onekind,
                           (void *)args[t]) != 0) {
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
            if (*pmem_tint[t][i] != t) {
                fprintf(
                    stderr,
                    "pmem_tint value has not been saved correctly in the thread.\n");
                return 1;
            }
            memkind_free(args[t]->kind, *(args[t]->ptr + i));
        }
        free(args[t]);
    }

    fprintf(stdout,
            "Threads successfully allocated memory in the PMEM kind.\n");

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
        *(args->ptr + i) = (int *)memkind_malloc(args->kind, sizeof(int));
        if (*(args->ptr + i) == NULL) {
            fprintf(stderr, "Unable to allocate pmem int.\n");
            return NULL;
        }
        **(args->ptr + i) = args->id;
    }

    return NULL;
}
