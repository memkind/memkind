// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2020 - 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind/internal/memkind_private.h>
#include <numa.h>

typedef enum memory_attribute_t
{
    MEM_ATTR_CAPACITY = 0,
    MEM_ATTR_BANDWIDTH = 1,
    MEM_ATTR_LATENCY = 2
} memory_attribute_t;

int get_per_cpu_local_nodes_mask(struct bitmask ***nodes_mask,
                                 memkind_node_variant_t node_variant,
                                 memory_attribute_t attr);
int set_closest_numanode_mem_attr(void **numanode,
                                  memkind_node_variant_t node_variant);

#ifdef __cplusplus
}
#endif
