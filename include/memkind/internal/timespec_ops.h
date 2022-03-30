// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void timespec_add(struct timespec *modified, const struct timespec *toadd);
void timespec_millis_to_timespec(double millis, struct timespec *out);
void timespec_get_time(clockid_t clock_id, struct timespec *time);

#ifdef __cplusplus
}
#endif
