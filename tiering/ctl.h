// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

struct memtier_kind *ctl_create_tier_kind_from_env(char *env_var_string);
void ctl_destroy_kind(struct memtier_kind *kind);

#ifdef __cplusplus
}
#endif
