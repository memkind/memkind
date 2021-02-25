// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "../config.h"
#include "memtier_log.h"

#include <pthread.h>

#define MEMTIER_EXPORT __attribute__((visibility("default")))
#define MEMTIER_INIT   __attribute__((constructor))
#define MEMTIER_FINI   __attribute__((destructor))

// functions below are defined in glibc
extern void *__libc_malloc(size_t size);
extern void *__libc_calloc(size_t nmemb, size_t size);
extern void *__libc_realloc(void *ptr, size_t size);
extern void *__libc_memalign(size_t boundary, size_t size);
extern void __libc_free(void *ptr);

void MEMTIER_EXPORT *malloc(size_t size)
{
    void *ret = __libc_malloc(size);
    log_debug("malloc(%u) = %p", size, ret);
    return ret;
}

void MEMTIER_EXPORT *calloc(size_t nmemb, size_t size)
{
    void *ret = __libc_calloc(nmemb, size);
    log_debug("calloc(%u, %u) = %p", nmemb, size, ret);
    return ret;
}

void MEMTIER_EXPORT *realloc(void *ptr, size_t size)
{
    void *ret = __libc_realloc(ptr, size);
    log_debug("realloc(%p, %u) = %p", ptr, size, ret);
    return ret;
}

void MEMTIER_EXPORT *memalign(size_t boundary, size_t size)
{
    void *ret = __libc_memalign(boundary, size);
    log_debug("memalign(%u, %u) = %p", boundary, size, ret);
    return ret;
}

void MEMTIER_EXPORT free(void *ptr)
{
    log_debug("free(%p)", ptr);
    return __libc_free(ptr);
}

static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static void MEMTIER_INIT memtier_init(void)
{
    pthread_once(&init_once, log_init_once);

    log_info("Memkind memtier lib loaded!");
}

static void MEMTIER_FINI memtier_fini(void)
{
    log_info("Unloading memkind memtier lib!");
}
