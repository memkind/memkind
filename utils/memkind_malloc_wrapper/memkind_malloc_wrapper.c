// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <memkind.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MEMKIND_WRAPPER_EXPORT __attribute__((visibility("default")))
#define MEMKIND_WRAPPER_INIT __attribute__((constructor))

static int MemkindInitDone = 0;
static memkind_t current_kind = NULL;

static int is_dax_kmem_enabled(const char *env)
{
    if (env && strcmp(env, "1") == 0 &&
        !memkind_check_available(MEMKIND_DAX_KMEM)) {
        return 0;
    }
    return 1;
}

static void MEMKIND_WRAPPER_INIT init_memkind_function(void)
{
    const char *pmem_path = secure_getenv("PMEM_KIND_PATH");
    const char *dax_option = secure_getenv("MEMKIND_DAX_KMEM_ON");
    if (pmem_path && dax_option) {
        fprintf(stderr,
                "PMEM_KIND_PATH and MEMKIND_DAX_KMEM_ON are mutually exclusive options.");
        abort();
    }
    if (pmem_path) {
        fprintf(stderr, "Initialize PMEM kind\n");
        int err = memkind_create_pmem(pmem_path, 0, &current_kind);
        if (err) {
            fprintf(stderr, "Initialize kind failed");
            abort();
        }
    } else if (!is_dax_kmem_enabled(dax_option)) {
        fprintf(stderr, "Initialize MEMKIND_DAX_KMEM kind\n");
        current_kind = MEMKIND_DAX_KMEM;
    } else {
        fprintf(stderr, "Initialize MEMKIND_DEFAULT kind\n");
        current_kind = MEMKIND_DEFAULT;
    }

    void *pp = memkind_malloc(current_kind, 16);
    if (pp == 0) {
        fprintf(stderr, "Initialize kind failed");
        abort();
    }
    fprintf(stderr, "Initialize ended\n");

    memkind_free(current_kind, pp);
    MemkindInitDone = 1;
}

static memkind_t get_current_kind(void)
{
    if (!MemkindInitDone)
        return MEMKIND_DEFAULT;
    return current_kind;
}

MEMKIND_WRAPPER_EXPORT void *malloc(size_t size)
{
    memkind_t kind = get_current_kind();
    return memkind_malloc(kind, size);
}

MEMKIND_WRAPPER_EXPORT void free(void *ptr)
{
    memkind_t kind = get_current_kind();
    memkind_free(kind, ptr);
}

MEMKIND_WRAPPER_EXPORT void *calloc(size_t nmemb, size_t size)
{
    memkind_t kind = get_current_kind();
    return memkind_calloc(kind, nmemb, size);
}

MEMKIND_WRAPPER_EXPORT void *realloc(void *ptr, size_t size)
{
    memkind_t kind = get_current_kind();
    return memkind_realloc(kind, ptr, size);
}

MEMKIND_WRAPPER_EXPORT size_t malloc_usable_size(void *ptr)
{
    memkind_t kind = get_current_kind();
    return memkind_malloc_usable_size(kind, ptr);
}

MEMKIND_WRAPPER_EXPORT int posix_memalign(void **memptr, size_t alignment,
                                          size_t size)
{
    memkind_t kind = get_current_kind();
    return memkind_posix_memalign(kind, memptr, alignment, size);
}

MEMKIND_WRAPPER_EXPORT void *valloc(size_t size)
{
    void *memptr = NULL;
    memkind_t kind = get_current_kind();
    int status = memkind_posix_memalign(kind, memptr, sysconf(_SC_PAGESIZE), size);
    if (status == 0 && memptr)
        return memptr;

    return NULL;
}

MEMKIND_WRAPPER_EXPORT void *memalign(size_t alignment, size_t size)
{
    void *memptr = NULL;
    memkind_t kind = get_current_kind();
    int status = memkind_posix_memalign(kind, &memptr, alignment, size);
    if (status == 0 && memptr)
        return memptr;

    return NULL;
}