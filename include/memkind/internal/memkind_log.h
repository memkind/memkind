// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define PRINTF_FORMAT __attribute__((format(printf, 1, 2)))

/*
 * For printing informational messages
 * Requires environment variable MEMKIND_DEBUG to be set to appropriate value
 */
void log_info(const char *format, ...) PRINTF_FORMAT;

/*
 * For printing messages regarding errors and failures
 * Requires environment variable MEMKIND_DEBUG to be set to appropriate value
 */
void log_err(const char *format, ...) PRINTF_FORMAT;

/*
 * For printing messages regarding fatal errors before calling abort()
 * Works *no matter* of MEMKIND_DEBUG state
 */
void log_fatal(const char *format, ...) PRINTF_FORMAT;

#ifdef __cplusplus
}
#endif
