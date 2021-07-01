// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2021 Intel Corporation. */

#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#ifdef MEMKIND_LOG_TO_FILE
static void save_log_to_file(char *log, char *fileName)
{
    int fd = open(fileName, O_WRONLY | O_APPEND);
    if (fd == -1) {
        int new_fd = open(fileName, O_CREAT | O_WRONLY, 0600);
        if (new_fd == -1) {
            fprintf(stderr, "MEMKIND_ERROR: cannot open file to save log\n");
        } else {
            (void)!write(new_fd, log, strlen(log));
            close(new_fd);
        }
    } else {
        (void)!write(fd, log, strlen(log));
        close(fd);
    }
}
#endif

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static void log_generic(message_type_t type, const char *format, va_list args)
{
    pthread_once(&init_once, log_init_once);

    static char buf[4096];
    sprintf(buf, "%s: ", message_prefixes[type]);

#ifdef MEMKIND_LOG_TO_FILE
    int pid = getpid();
    char fileName[11];
    sprintf(fileName, "%d.log", pid);
#endif
    if (log_enabled || (type == MESSAGE_TYPE_FATAL)) {
        if (pthread_mutex_lock(&log_lock) != 0)
            assert(0 && "failed to acquire mutex");
#ifdef MEMKIND_LOG_TO_FILE
        save_log_to_file(buf, fileName);
#endif
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
