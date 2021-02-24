// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <tiering/ctl.h>
#include <tiering/memtier_log.h>

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

static int parse_env_string(char *env_var_string)
{
    char *kind_name = NULL;
    char *pmem_path = NULL;
    char *pmem_size = NULL;
    unsigned ratio_value = -1;
    int ret = ctl_load_config(env_var_string, &kind_name, &pmem_path,
                              &pmem_size, &ratio_value);
    if (ret != 0) {
        return -1;
    }

    log_debug("kind_name: %s", kind_name);
    log_debug("pmem_path: %s", pmem_path);
    log_debug("pmem_size: %s", pmem_size);
    log_debug("ratio_value: %u", ratio_value);

    return 0;
}

static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static MEMTIER_INIT void memtier_init(void)
{
    pthread_once(&init_once, log_init_once);

    // TODO: Handle failure when this variable (or config variable) is not
    // present
    char *env_var = utils_get_env("MEMKIND_MEM_TIERING_CONFIG");
    if (env_var) {
        int ret = parse_env_string(env_var);
        if (ret != 0) {
            log_err("Couldn't load MEMKIND_MEM_TIERING_CONFIG env var");
        }
    }

    log_info("Memkind memtier lib loaded!");
}

static MEMTIER_FINI void memtier_fini(void)
{
    log_info("Unloading memkind memtier lib!");
}
