// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#include <memkind.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int MemkindInitDone = 0;
static int MemkindDestructed = 0;
static memkind_t current_kind = NULL;

#define MEMKIND_WRAPPER_EXPORT __attribute__((visibility("default")))

#define MEMKIND_WRAPPER_INIT __attribute__((constructor))
#define MEMKIND_WRAPPER_FINI __attribute__((destructor))

#define WRAPPER_LIKELY(x) __builtin_expect((x), 1)
#define WRAPPER_UNLIKELY(x) __builtin_expect((x), 0)

#ifdef DEBUG_WRAPPER
#define LOG(...)                                                               \
    do {                                                                       \
        fprintf(stderr, __VA_ARGS__);                                          \
    } while (0)

MEMKIND_WRAPPER_EXPORT void memkind_malloc_post(memkind_t kind, size_t size,
                                                void **result)
{
    if (WRAPPER_LIKELY(MemkindInitDone)) {
        LOG("\t malloc: size %zu\n", size);
    }
}

MEMKIND_WRAPPER_EXPORT void memkind_calloc_post(memkind_t kind, size_t nmemb,
                                                size_t size, void **result)
{
    if (WRAPPER_LIKELY(MemkindInitDone)) {
        LOG("\t calloc: size %zu\n", size);
    }
}

MEMKIND_WRAPPER_EXPORT void memkind_posix_memalign_post(memkind_t kind,
                                                        void **memptr,
                                                        size_t alignment,
                                                        size_t size, int *err)
{
    if (WRAPPER_LIKELY(MemkindInitDone)) {
        LOG("\t posix_memalign: size %zu\n", size);
    }
}

MEMKIND_WRAPPER_EXPORT void memkind_realloc_post(memkind_t *kind, void *ptr,
                                                 size_t size, void **result)
{
    if (WRAPPER_LIKELY(MemkindInitDone)) {
        LOG("\t realloc: size %zu\n", size);
    }
}

MEMKIND_WRAPPER_EXPORT void memkind_free_post(memkind_t kind, void *ptr)
{
    if (WRAPPER_LIKELY(MemkindInitDone)) {
        size_t size = memkind_malloc_usable_size(kind, ptr);
        LOG("\t free: size %zu\n", size);
    }
}
#else
#define LOG(...)
#endif // DEBUG_WRAPPER

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
    const char *pmem_path = getenv("PMEM_KIND_PATH");
    const char *dax_option = getenv("MEMKIND_DAX_KMEM_ON");
    if (pmem_path && dax_option) {
        fprintf(
            stderr,
            "PMEM_KIND_PATH and MEMKIND_DAX_KMEM_ON are mutually exclusive options.\n");
        abort();
    }
    if (pmem_path) {
        LOG("Initialize PMEM kind\n");
        int err = memkind_create_pmem(pmem_path, 0, &current_kind);
        if (err) {
            fprintf(stderr, "Create PMEM kind failed\n");
            abort();
        }
    } else if (!is_dax_kmem_enabled(dax_option)) {
        LOG("Initialize MEMKIND_DAX_KMEM kind\n");
        current_kind = MEMKIND_DAX_KMEM;
    } else {
        LOG("Initialize MEMKIND_DEFAULT kind\n");
        current_kind = MEMKIND_DEFAULT;
    }

    void *pp = memkind_malloc(current_kind, 16);
    if (pp == 0) {
        fprintf(stderr, "Initialize kind failed\n");
        abort();
    }

    memkind_free(current_kind, pp);
    MemkindInitDone = 1;
    LOG("Initialize finish\n");
}

static void MEMKIND_WRAPPER_FINI fini_memkind_function(void)
{
    LOG("Finalize called\n");
    const char *pmem_path = getenv("PMEM_KIND_PATH");
    if (pmem_path) {
        if (!memkind_destroy_kind(current_kind)) {
            LOG("Destroy PMEM kind\n");
        } else {
            fprintf(stderr, "Destroy PMEM kind failed\n");
            abort();
        }
    }
    MemkindInitDone = 0;
    MemkindDestructed = 1;
    LOG("Finalize finish\n");
}

static memkind_t get_current_kind(void)
{
    if (WRAPPER_LIKELY(MemkindInitDone)) {
        return current_kind;
    } else {
        return MEMKIND_DEFAULT;
    }
}

MEMKIND_WRAPPER_EXPORT void *malloc(size_t size)
{
    if (WRAPPER_UNLIKELY(MemkindDestructed))
        return NULL;
    memkind_t kind = get_current_kind();
    return memkind_malloc(kind, size);
}

MEMKIND_WRAPPER_EXPORT void free(void *ptr)
{
    if (WRAPPER_UNLIKELY(MemkindDestructed))
        return;
    memkind_t kind = get_current_kind();
    memkind_free(kind, ptr);
}

MEMKIND_WRAPPER_EXPORT void *calloc(size_t nmemb, size_t size)
{
    if (WRAPPER_UNLIKELY(MemkindDestructed))
        return NULL;
    memkind_t kind = get_current_kind();
    return memkind_calloc(kind, nmemb, size);
}

MEMKIND_WRAPPER_EXPORT void *realloc(void *ptr, size_t size)
{
    if (WRAPPER_UNLIKELY(MemkindDestructed))
        return NULL;
    memkind_t kind = get_current_kind();
    return memkind_realloc(kind, ptr, size);
}

MEMKIND_WRAPPER_EXPORT size_t malloc_usable_size(void *ptr)
{
    if (WRAPPER_UNLIKELY(MemkindDestructed))
        return 0;
    memkind_t kind = get_current_kind();
    return memkind_malloc_usable_size(kind, ptr);
}

MEMKIND_WRAPPER_EXPORT int posix_memalign(void **memptr, size_t alignment,
                                          size_t size)
{
    if (WRAPPER_UNLIKELY(MemkindDestructed))
        return ENOMEM;
    memkind_t kind = get_current_kind();
    return memkind_posix_memalign(kind, memptr, alignment, size);
}

MEMKIND_WRAPPER_EXPORT void *valloc(size_t size)
{
    if (WRAPPER_UNLIKELY(MemkindDestructed))
        return NULL;
    void *memptr = NULL;
    memkind_t kind = get_current_kind();
    int status =
        memkind_posix_memalign(kind, memptr, sysconf(_SC_PAGESIZE), size);
    if (status == 0 && memptr)
        return memptr;

    return NULL;
}

MEMKIND_WRAPPER_EXPORT void *memalign(size_t alignment, size_t size)
{
    if (WRAPPER_UNLIKELY(MemkindDestructed))
        return NULL;
    void *memptr = NULL;
    memkind_t kind = get_current_kind();
    int status = memkind_posix_memalign(kind, &memptr, alignment, size);
    if (status == 0 && memptr)
        return memptr;

    return NULL;
}

#ifndef __MALLOC_HOOK_VOLATILE
#define __MALLOC_HOOK_VOLATILE
#endif

/*
 * Interpose malloc hooks in glibc.
 */
void *(*__MALLOC_HOOK_VOLATILE __malloc_hook)(size_t size, const void *caller) =
    (void *)malloc;
void *(*__MALLOC_HOOK_VOLATILE __realloc_hook)(
    void *ptr, size_t size, const void *caller) = (void *)realloc;
void (*__MALLOC_HOOK_VOLATILE __free_hook)(void *ptr,
                                           const void *caller) = (void *)free;
void *(*__MALLOC_HOOK_VOLATILE __memalign_hook)(
    size_t size, size_t alignment, const void *caller) = (void *)memalign;
