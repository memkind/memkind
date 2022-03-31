// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/timespec.h"
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

struct timespec timespec_diff(const struct timespec *time1,
                              const struct timespec *time0)
{
    const uint64_t S_TO_NS = 1e9;

    assert(time1 && time1->tv_nsec < S_TO_NS);
    assert(time0 && time0->tv_nsec < S_TO_NS);
    
    struct timespec diff;
    diff.tv_sec = time1->tv_sec - time0->tv_sec;
    diff.tv_nsec = time1->tv_nsec - time0->tv_nsec;

    if (diff.tv_nsec < 0) {
        diff.tv_nsec += S_TO_NS;
        diff.tv_sec--;
    }

    return diff;
}

bool timespec_not_zero(const struct timespec *time)
{
    return ((time->tv_sec > 0) || ((time->tv_sec == 0) && (time->tv_nsec > 0)));
}

void timespec_get_time(struct timespec *time)
{
    // calculate time to wait
    int ret = clock_gettime(CLOCK_MONOTONIC, time);
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
