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

// to avoid VSCode editor errors
// TODO remove after POC (generate this define with ./configure)
#ifndef CPU_LOGICAL_CORES_NUMBER
#define CPU_LOGICAL_CORES_NUMBER 1
#endif

typedef struct PebsMetadata {
    int pebs_fd[CPU_LOGICAL_CORES_NUMBER];
    char *pebs_mmap[CPU_LOGICAL_CORES_NUMBER];
    __u64 last_head[CPU_LOGICAL_CORES_NUMBER];
    touch_cb cb;
} PebsMetadata;

void pebs_create(PebsMetadata *pebs, touch_cb cb);
void pebs_destroy(PebsMetadata *pebs);
void pebs_monitor(PebsMetadata *pebs);

#ifdef __cplusplus
}
#endif
