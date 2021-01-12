// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind.h>
#include <numa.h>

int get_per_cpu_hi_cap_local_nodes_mask(struct bitmask ***nodes_mask);
int get_mem_attributes_hbw_nodes_mask(struct bitmask **hbw_node_mask);

#ifdef __cplusplus
}
#endif
