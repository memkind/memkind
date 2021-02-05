// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include <memkind.h>
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
};

struct decorators_flags *decorators_state;

extern "C" {

void memkind_malloc_pre(struct memkind *kind, size_t size)
{
    decorators_state->malloc_pre++;
}

void memkind_malloc_post(struct memkind *kind, size_t size, void **result)
{
    decorators_state->malloc_post++;
}

void memkind_calloc_pre(struct memkind *kind, size_t nmemb, size_t size)
{
    decorators_state->calloc_pre++;
}

void memkind_calloc_post(struct memkind *kind, size_t nmemb, size_t size,
                         void **result)
{
    decorators_state->calloc_post++;
}

void memkind_posix_memalign_pre(struct memkind *kind, void **memptr,
                                size_t alignment, size_t size)
{
    decorators_state->posix_memalign_pre++;
}

void memkind_posix_memalign_post(struct memkind *kind, void **memptr,
                                 size_t alignment, size_t size, int *err)
{
    decorators_state->posix_memalign_post++;
}

void memkind_realloc_pre(struct memkind *kind, void *ptr, size_t size)
{
    decorators_state->realloc_pre++;
}

void memkind_realloc_post(struct memkind *kind, void *ptr, size_t size,
                          void **result)
{
    decorators_state->realloc_post++;
}

void memkind_free_pre(struct memkind **kind, void **ptr)
{
    decorators_state->free_pre++;
}

void memkind_free_post(struct memkind **kind, void **ptr)
{
    decorators_state->free_post++;
}
}
