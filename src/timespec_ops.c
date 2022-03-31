// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/timespec_ops.h"
#include "memkind/internal/memkind_log.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void timespec_add(struct timespec *modified, const struct timespec *toadd)
{
    const uint64_t S_TO_NS = 1e9;
    modified->tv_sec += toadd->tv_sec;
    modified->tv_nsec += toadd->tv_nsec;
    modified->tv_sec += (modified->tv_nsec / S_TO_NS);
    modified->tv_nsec %= S_TO_NS;
}

void timespec_get_time(clockid_t clock_id, struct timespec *time)
{
    // calculate time to wait
    int ret = clock_gettime(clock_id, time);
    if (ret != 0) {
        log_fatal("ASSERT PEBS Monitor clock_gettime() FAILURE: %s",
                  strerror(errno));
        exit(-1);
    }
}

void timespec_millis_to_timespec(double millis, struct timespec *out)
{
    out->tv_sec = (uint64_t)(millis / 1e3);
    double ms_to_convert = millis - out->tv_sec * 1000ul;
    out->tv_nsec = (uint64_t)(ms_to_convert * 1e6);
}
