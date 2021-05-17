// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2021 Intel Corporation. */

#include <memkind_memtier.h>
#include <stdio.h>
#include <stdlib.h>

struct decorators_flags {
    int malloc_pre;
    int malloc_post;
    int calloc_pre;
    int calloc_post;
    int posix_memalign_pre;
    int posix_memalign_post;
    int realloc_pre;
    int realloc_post;
    int free_pre;
    int free_post;
    int malloc_usable_size_pre;
    int malloc_usable_size_post;
};

struct decorators_flags *decorators_state;

extern "C" {
    

}

