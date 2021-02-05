// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */

#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    MESSAGE_TYPE_INFO,
    MESSAGE_TYPE_ERROR,
    MESSAGE_TYPE_FATAL,
    MESSAGE_TYPE_MAX_VALUE,
} message_type_t;

static char *message_prefixes[MESSAGE_TYPE_MAX_VALUE] = {
    [MESSAGE_TYPE_INFO] = "MEMKIND_INFO",
    [MESSAGE_TYPE_ERROR] = "MEMKIND_ERROR",
    [MESSAGE_TYPE_FATAL] = "MEMKIND_FATAL",
};

static bool log_enabled;

static pthread_once_t init_once = PTHREAD_ONCE_INIT;
static void log_init_once(void)
{
    char *memkind_debug_env = memkind_get_env("MEMKIND_DEBUG");

    if (memkind_debug_env) {
        if (strcmp(memkind_debug_env, "1") == 0) {
            log_enabled = true;
        } else {
            fprintf(
                stderr,
                "MEMKIND_WARNING: debug option \"%s\" unknown; Try man memkind for available options.\n",
                memkind_debug_env);
        }
    }
}

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static void log_generic(message_type_t type, const char *format, va_list args)
{
    pthread_once(&init_once, log_init_once);
    if (log_enabled || (type == MESSAGE_TYPE_FATAL)) {
        if (pthread_mutex_lock(&log_lock) != 0)
            assert(0 && "failed to acquire mutex");
        fprintf(stderr, "%s: ", message_prefixes[type]);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
        if (pthread_mutex_unlock(&log_lock) != 0)
            assert(0 && "failed to release mutex");
    }
}

void log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_generic(MESSAGE_TYPE_INFO, format, args);
    va_end(args);
}

void log_err(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_generic(MESSAGE_TYPE_ERROR, format, args);
    va_end(args);
}

void log_fatal(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_generic(MESSAGE_TYPE_FATAL, format, args);
    va_end(args);
}
