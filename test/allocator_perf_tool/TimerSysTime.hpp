// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <sys/time.h>

class TimerSysTime
{
public:
    void start()
    {
        gettimeofday(&last, 0);
    }

    double getElapsedTime() const
    {
        struct timeval now;
        gettimeofday(&now, 0);
        double time_delta_sec = ((double)now.tv_sec - (double)last.tv_sec);
        double time_delta_usec =
            ((double)now.tv_usec - (double)last.tv_usec) / 1000000.0;
        return time_delta_sec + time_delta_usec;
    }

private:
    struct timeval last;
};
