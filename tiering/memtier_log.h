// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <stdio.h>

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MESSAGE_TYPE_ERROR, // default
    MESSAGE_TYPE_INFO,
    MESSAGE_TYPE_DEBUG,
    MESSAGE_TYPE_MAX_VALUE,
} message_type_t;

char *utils_get_env(const char *name);
void log_info(const char *format, ...);
void log_err(const char *format, ...);
void log_debug(const char *format, ...);
void log_init_once(void);

#ifdef __cplusplus
}
#endif
