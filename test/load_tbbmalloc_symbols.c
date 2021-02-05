// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */

#include "tbbmalloc.h"

int load_tbbmalloc_symbols()
{
    const char so_name[] = "libtbbmalloc.so.2";
    void *tbb_handle = dlopen(so_name, RTLD_LAZY);
    if (!tbb_handle) {
        printf("Cannot load %s\n", so_name);
        return -1;
    }

    scalable_malloc = dlsym(tbb_handle, "scalable_malloc");
    if (!scalable_malloc) {
        printf("Cannot load scalable_malloc symbol from %s\n", so_name);
        return -1;
    }

    scalable_realloc = dlsym(tbb_handle, "scalable_realloc");
    if (!scalable_realloc) {
        printf("Cannot load scalable_realloc symbol from %s\n", so_name);
        return -1;
    }

    scalable_calloc = dlsym(tbb_handle, "scalable_calloc");
    if (!scalable_calloc) {
        printf("Cannot load scalable_calloc symbol from %s\n", so_name);
        return -1;
    }

    scalable_free = dlsym(tbb_handle, "scalable_free");
    if (!scalable_free) {
        printf("Cannot load scalable_free symbol from %s\n", so_name);
        return -1;
    }

    return 0;
}
