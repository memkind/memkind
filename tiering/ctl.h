// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

struct memtier_memory *
ctl_create_tier_memory_from_env(char *tiers_var_string,
                                char *thresholds_var_string);
void ctl_destroy_tier_memory(struct memtier_memory *kind);

#ifdef __cplusplus
}
#endif
