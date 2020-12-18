// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#include <memkind.h>
#ifdef MEMKIND_HWLOC
#include <hwloc.h>
#endif // MEMKIND_HWLOC

extern struct memkind_ops MEMKIND_HIGHEST_CAPACITY_OPS;
extern struct memkind_ops MEMKIND_HIGHEST_CAPACITY_PREFERRED_OPS;
extern struct memkind_ops MEMKIND_HIGHEST_CAPACITY_LOCAL_OPS;

#ifdef __cplusplus
}
#endif
