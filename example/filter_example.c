#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <numakind.h>

int main(int argc, char **argv)
{
    const size_t stream_len = 1024 * 1024;
    const size_t filter_len = 1024;
    const size_t num_filter = stream_len / filter_len;
    size_t i, j;
    int k, l;
    double *stream = NULL;
    double *filter = NULL;
    double *result = NULL;

    srandom(0);
    numakind_init();

    stream = (double *)numakind_malloc(NUMAKIND_DEFAULT, stream_len * sizeof(double));
    if (stream == NULL) {
        perror("<numakind>");
        fprintf(stderr, "Unable to allocate stream\n");
        return errno ? -errno : 1;
    }

    filter = (double *)numakind_malloc(NUMAKIND_HBW, filter_len * sizeof(double));
    if (filter == NULL) {
        perror("<numakind>");
        fprintf(stderr, "Unable to allocate filter\n");
        return errno ? -errno : 1;
    }

    result = (double *)numakind_calloc(NUMAKIND_HBW, filter_len, sizeof(double));
    if (result == NULL) {
        perror("<numakind>");
        fprintf(stderr, "Unable to allocate result\n");
        return errno ? -errno : 1;
    }

    for (i = 0; i < stream_len; i++) {
        stream[i] = (double)(random())/(double)(RAND_MAX);
    }

    for (i = 0; i < filter_len; i++) {
        filter[i] = (double)(i)/(double)(filter_len);
    }

    for (i = 0; i < num_filter; i++) {
        for (j = 0; j < filter_len; j++) {
            result[j] += stream[i * filter_len + j] * filter[j];
        }
    }

    for (i = 0; i < filter_len; i++) {
        fprintf(stdout, "%.6e\n", result[i]);
    }

    return 0;
}
