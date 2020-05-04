// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#ifndef COMMON_H
#define COMMON_H

#include <hbwmalloc.h>

#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <gtest/gtest.h>

#define MB 1048576ULL
#define GB 1073741824ULL
#define KB 1024ULL

#define HBW_SUCCESS 0
#define HBW_ERROR -1

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif
