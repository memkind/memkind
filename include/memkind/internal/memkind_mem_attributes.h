// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind.h>
#include <numa.h>

int get_mem_attributes_hbw_nodes_mask(struct bitmask **hbw_node_mask);

#ifdef __cplusplus
}
#endif
