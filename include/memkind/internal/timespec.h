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
struct timespec timespec_diff(const struct timespec *time1,
                              const struct timespec *time0);
bool timespec_not_zero(const struct timespec *time);
void timespec_millis_to_timespec(double millis, struct timespec *out);
void timespec_get_time(struct timespec *time);

#ifdef __cplusplus
}
#endif
