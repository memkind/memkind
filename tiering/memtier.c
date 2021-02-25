// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "../config.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define UTILS_EXPORT __attribute__((visibility("default")))
#define UTILS_INIT   __attribute__((constructor))
#define UTILS_FINI   __attribute__((destructor))

typedef enum
{
    MESSAGE_TYPE_ERROR, // default
    MESSAGE_TYPE_INFO,
    MESSAGE_TYPE_DEBUG,
    MESSAGE_TYPE_MAX_VALUE,
} message_type_t;

static char *message_prefixes[MESSAGE_TYPE_MAX_VALUE] = {
    [MESSAGE_TYPE_ERROR] = "MEMKIND_MEM_TIERING_LOG_ERROR",
    [MESSAGE_TYPE_INFO] = "MEMKIND_MEM_TIERING_LOG_INFO",
    [MESSAGE_TYPE_DEBUG] = "MEMKIND_MEM_TIERING_LOG_DEBUG",
};

static size_t log_level;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;

char *utils_get_env(const char *name)
{
#ifdef MEMKIND_HAVE_SECURE_GETENV
    return secure_getenv(name);
#else
    return getenv(name);
#endif
}

void log_generic(message_type_t type, const char *format, va_list args);

void log_info(const char *format, ...)
{
    if (log_level >= MESSAGE_TYPE_INFO) {
        va_list args;
        va_start(args, format);
        log_generic(MESSAGE_TYPE_INFO, format, args);
        va_end(args);
    }
}

void log_err(const char *format, ...)
{
    if (log_level >= MESSAGE_TYPE_ERROR) {
        va_list args;
        va_start(args, format);
        log_generic(MESSAGE_TYPE_ERROR, format, args);
        va_end(args);
    }
}

void log_debug(const char *format, ...)
{
    if (log_level >= MESSAGE_TYPE_DEBUG) {
        va_list args;
        va_start(args, format);
        log_generic(MESSAGE_TYPE_DEBUG, format, args);
        va_end(args);
    }
}

void log_init_once(void)
{
    log_level = MESSAGE_TYPE_ERROR;

    char *log_level_env = utils_get_env("MEMKIND_MEM_TIERING_LOG_LEVEL");
    if (log_level_env) {
        char *end;
        errno = 0;
        log_level = strtoul(log_level_env, &end, 10);
        if ((log_level >= MESSAGE_TYPE_MAX_VALUE) || (*end != '\0') ||
            (errno != 0)) {
            log_err("Wrong value of MEMKIND_MEM_TIERING_LOG_LEVEL=%s",
                    log_level_env);
            abort();
        } else {
            log_debug("Setting log level to: %d", log_level);
        }
    }
}

void log_generic(message_type_t type, const char *format, va_list args)
{
    if (pthread_mutex_lock(&log_lock) != 0) {
        assert(0 && "failed to acquire log mutex");
    }

    fprintf(stderr, "%s: ", message_prefixes[type]);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    if (pthread_mutex_unlock(&log_lock) != 0) {
        assert(0 && "failed to release log mutex");
    }
}

// glibc defines these, a version portable to others libcs would need to call
// dlsym() at runtime.
extern void *__libc_malloc(size_t size);
extern void *__libc_calloc(size_t nmemb, size_t size);
extern void *__libc_realloc(void *ptr, size_t size);
extern void __libc_free(void *ptr);

void *memtier_malloc(size_t size)
{
    return __libc_malloc(size);
}

void *memtier_calloc(size_t nmemb, size_t size)
{
    return __libc_calloc(nmemb, size);
}

void *memtier_realloc(void *ptr, size_t size)
{
    return __libc_realloc(ptr, size);
}

void memtier_free(void *ptr)
{
    return __libc_free(ptr);
}

static void UTILS_INIT utils_init(void)
{
    pthread_once(&init_once, log_init_once);

    log_info("Memkind mem tiering utils lib loaded!");
}

static void UTILS_FINI utils_fini(void)
{
    log_info("Unloading memkind mem tiering utils lib!");
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
