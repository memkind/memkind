// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once
#include <memkind_memtier.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tier_draft {
    char *kind_name;
    char *pmem_path;
    size_t pmem_size;
    unsigned ratio_value;
} tier_draft;

struct memtier_kind *ctl_create_tier_kind_from_env(char *env_var_string);
void ctl_destroy_kind(struct memtier_kind *kind);

#ifdef __cplusplus
}
#endif
