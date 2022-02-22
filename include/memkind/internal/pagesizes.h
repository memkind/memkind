// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "stdbool.h"
#include "stddef.h"
#include "unistd.h"

/// @warning
/// For the sake of this project, multiple different page sizes are used:
///     - bigary page: granularity of mmap,
///     - traced page: granularity of memory tracing (each *traced page*
///     contains metadata),
///     - system page: value dependent on the system
/// Please check the documentation of each variable for more info

/// granularity of memory tracing; should be a multiple of *sysytem_pagesize*
/// each traced page contains some traced metadata
#define TRACED_PAGESIZE (4 * 1024u)

/// granularity of mmap
#define BIGARY_PAGESIZE (2 * 1024 * 1024ULL)

/// @brief returns the size of page used by the OS
extern size_t traced_pagesize_get_sysytem_pagesize(void);
extern bool traced_pagesize_check_correctness(void);
