// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include <memkind.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    const size_t stream_len = 1024 * 1024;
    const size_t filter_len = 1024;
    const size_t num_filter = stream_len / filter_len;
    size_t i, j;
    double *stream = NULL;
    double *filter = NULL;
    double *result = NULL;

    srandom(0);

    stream =
        (double *)memkind_malloc(MEMKIND_DEFAULT, stream_len * sizeof(double));
    if (stream == NULL) {
        perror("<memkind>");
        fprintf(stderr, "Unable to allocate stream\n");
        return errno ? -errno : 1;
    }

    filter = (double *)memkind_malloc(MEMKIND_HBW, filter_len * sizeof(double));
    if (filter == NULL) {
        perror("<memkind>");
        fprintf(stderr, "Unable to allocate filter\n");
        return errno ? -errno : 1;
    }

    result = (double *)memkind_calloc(MEMKIND_HBW, filter_len, sizeof(double));
    if (result == NULL) {
        perror("<memkind>");
        fprintf(stderr, "Unable to allocate result\n");
        return errno ? -errno : 1;
    }

    for (i = 0; i < stream_len; i++) {
        stream[i] = (double)(random()) / (double)(RAND_MAX);
    }

    for (i = 0; i < filter_len; i++) {
        filter[i] = (double)(i) / (double)(filter_len);
    }

    for (i = 0; i < num_filter; i++) {
        for (j = 0; j < filter_len; j++) {
            result[j] += stream[i * filter_len + j] * filter[j];
        }
    }

    for (i = 0; i < filter_len; i++) {
        fprintf(stdout, "%.6e\n", result[i]);
    }

    memkind_free(MEMKIND_HBW, result);
    memkind_free(MEMKIND_HBW, filter);
    memkind_free(MEMKIND_DEFAULT, stream);

    return 0;
}
