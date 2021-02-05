// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (C) 2018 - 2020 Intel Corporation. */

#include <memkind.h>

#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PMEM_MAX_SIZE (1024 * 1024 * 32)
#define NUM_THREADS 10

static char path[PATH_MAX] = "/tmp/";

void *thread_ind(void *arg);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
}

int main(int argc, char *argv[])
{
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [pmem_kind_dir_path]\n", argv[0]);
        return 1;
    } else if (argc == 2 && (realpath(argv[1], path) == NULL)) {
        fprintf(stderr, "Incorrect pmem_kind_dir_path %s\n", argv[1]);
        return 1;
    }

    fprintf(
        stdout,
        "This example shows how to use multithreading with independent pmem kinds."
        "\nPMEM kind directory: %s\n",
        path);

    pthread_t pmem_threads[NUM_THREADS];
    int t;

    // Create many independent threads
    for (t = 0; t < NUM_THREADS; t++) {
        if (pthread_create(&pmem_threads[t], NULL, thread_ind, NULL) != 0) {
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

    fprintf(stdout,
            "Threads successfully allocated memory in the PMEM kinds.\n");

    return 0;
}

void *thread_ind(void *arg)
{
    struct memkind *pmem_kind;

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

    int status = memkind_check_dax_path(path);
    if (!status) {
        fprintf(stdout, "PMEM kind %s is on DAX-enabled File system.\n", path);
    } else {
        fprintf(stdout, "PMEM kind %s is not on DAX-enabled File system.\n",
                path);
    }

    int err = memkind_create_pmem(path, PMEM_MAX_SIZE, &pmem_kind);
    if (err) {
        print_err_message(err);
        return NULL;
    }

    void *test = memkind_malloc(pmem_kind, 32);
    if (test == NULL) {
        fprintf(stderr, "Unable to allocate pmem (test) in thread.\n");
        return NULL;
    }

    memkind_free(pmem_kind, test);

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
    }

    return NULL;
}
