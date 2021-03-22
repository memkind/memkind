// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once
#include <memkind_memtier.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tier_skeleton {
    char *kind_name;
    char *pmem_path;
    size_t pmem_size;
    unsigned ratio_value;
} tier_skeleton;

int ctl_load_config(char *buf, tier_skeleton *tier, memtier_policy_t *policy);

#ifdef __cplusplus
}
#endif
