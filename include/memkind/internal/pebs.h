// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

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

#include <memkind/internal/memkind_private.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct perf_event_mmap_page perf_event_mmap_page_t;
typedef struct perf_event_header perf_event_header_t;

typedef void (*touch_cb)(uintptr_t address, uint64_t timestamp);

typedef struct PebsMetadata {
    int *pebs_fd;
    void **pebs_mmap;
    __u64 *last_head;
    size_t nof_cpus;
    touch_cb cb;
} PebsMetadata;

void pebs_create(PebsMetadata *pebs, touch_cb cb);
void pebs_destroy(PebsMetadata *pebs);
void pebs_monitor(PebsMetadata *pebs);

#ifdef __cplusplus
}
#endif
