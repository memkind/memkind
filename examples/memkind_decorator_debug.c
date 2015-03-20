/*
 * Copyright (C) 2014, 2015 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <memkind.h>

/* This is an example that enables debug printing on every alloction call */

static void memkind_debug(const char *func, const char *kind, size_t size, void *ptr)
{
    fprintf(stderr, "[ DEBUG ] func=%s kind=%s size=%zu ptr=0x%lx\n", func, kind, size, (size_t)ptr);
}

void memkind_malloc_post(struct memkind *kind, size_t size, void **result)
{
    memkind_debug("memkind_malloc", kind->name, size, *result);
}

void memkind_calloc_post(struct memkind *kind, size_t nmemb, size_t size, void **result)
{
    memkind_debug("memkind_calloc", kind->name, nmemb * size, *result);
}

void memkind_posix_memalign_post(struct memkind *kind, void **memptr, size_t alignment, size_t size, int *err)
{
    memkind_debug("memkind_posix_memalign", kind->name, size, *memptr);
}

void memkind_realloc_post(struct memkind *kind, void *ptr, size_t size, void **result)
{
    memkind_debug("memkind_realloc", kind->name, size, *result);
}

void memkind_free_pre(struct memkind **kind, void **ptr)
{
    memkind_debug("memkind_free", (*kind)->name, 0, *ptr);
}
