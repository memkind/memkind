// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#define USE_LIBPFM 0

#include <linux/hw_breakpoint.h> // definition of HW_* constants
#include <linux/perf_event.h>    // definition of PERF_* constants
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h> // definition of SYS_* constants
#include <sys/types.h>
#include <unistd.h>

#if USE_LIBPFM
#include <perfmon/pfmlib.h> // pfm_get_os_event_encoding
#include <perfmon/pfmlib_perf_event.h>
#endif

#include <memkind/internal/memkind_private.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct perf_event_mmap_page perf_event_mmap_page_t;
typedef struct perf_event_header perf_event_header_t;

typedef void (*touch_cb)(uintptr_t address, uint64_t timestamp);

void pebs_create();
void pebs_destroy();
void pebs_monitor(touch_cb cb);

#ifdef __cplusplus
}
#endif
