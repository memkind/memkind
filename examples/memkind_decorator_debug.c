// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include <memkind.h>

#include <stdio.h>
#include <stdlib.h>

/* This is an example that enables debug printing on every alloction call */

static void memkind_debug(const char *func, memkind_t kind, size_t size,
                          void *ptr)
{
    fprintf(stderr, "[ DEBUG ] func=%s kind=%p size=%zu ptr=0x%lx\n", func,
            kind, size, (size_t)ptr);
}

void memkind_malloc_post(memkind_t kind, size_t size, void **result)
{
    memkind_debug("memkind_malloc", kind, size, *result);
}

void memkind_calloc_post(memkind_t kind, size_t nmemb, size_t size,
                         void **result)
{
    memkind_debug("memkind_calloc", kind, nmemb * size, *result);
}

void memkind_posix_memalign_post(memkind_t kind, void **memptr,
                                 size_t alignment, size_t size, int *err)
{
    memkind_debug("memkind_posix_memalign", kind, size, *memptr);
}

void memkind_realloc_post(memkind_t kind, void *ptr, size_t size, void **result)
{
    memkind_debug("memkind_realloc", kind, size, *result);
}

void memkind_free_pre(memkind_t kind, void **ptr)
{
    memkind_debug("memkind_free", kind, 0, *ptr);
}
