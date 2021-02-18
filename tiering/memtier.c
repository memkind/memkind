// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "memtier_log.h"

#include <pthread.h>

#define MEMTIER_EXPORT __attribute__((visibility("default")))
#define MEMTIER_INIT   __attribute__((constructor))
#define MEMTIER_FINI   __attribute__((destructor))

// glibc defines these, a version portable to others libcs would need to call
// dlsym() at runtime.
extern void *__libc_malloc(size_t size);
extern void *__libc_calloc(size_t nmemb, size_t size);
extern void *__libc_realloc(void *ptr, size_t size);
extern void *__libc_memalign(size_t boundary, size_t size);
extern void __libc_free(void *ptr);

MEMTIER_EXPORT void *malloc(size_t size)
{
    void *ret = __libc_malloc(size);
    log_debug("malloc(%zu) = %p", size, ret);
    return ret;
}

MEMTIER_EXPORT void *calloc(size_t nmemb, size_t size)
{
    void *ret = __libc_calloc(nmemb, size);
    log_debug("calloc(%zu, %zu) = %p", nmemb, size, ret);
    return ret;
}

MEMTIER_EXPORT void *realloc(void *ptr, size_t size)
{
    void *ret = __libc_realloc(ptr, size);
    log_debug("realloc(%p, %zu) = %p", ptr, size, ret);
    return ret;
}

MEMTIER_EXPORT void *memalign(size_t boundary, size_t size)
{
    void *ret = __libc_memalign(boundary, size);
    log_debug("memalign(%zu, %zu) = %p", boundary, size, ret);
    return ret;
}

MEMTIER_EXPORT void free(void *ptr)
{
    log_debug("free(%p)", ptr);
    return __libc_free(ptr);
}

static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static MEMTIER_INIT void memtier_init(void)
{
    pthread_once(&init_once, log_init_once);

    log_info("Memkind memtier lib loaded!");
}

static MEMTIER_FINI void memtier_fini(void)
{
    log_info("Unloading memkind memtier lib!");
}

UTILS_EXPORT void *test(size_t size)
{
    log_debug("test");

    return 0;
}

#ifndef __MALLOC_HOOK_VOLATILE
#define __MALLOC_HOOK_VOLATILE
#endif

void *(*__MALLOC_HOOK_VOLATILE __test_hook)(size_t size,
                                            const void *caller) = (void *)test;
