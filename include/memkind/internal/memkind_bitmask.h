// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <numa.h>
#include <stdbool.h>

typedef int (*get_node_bitmask)(struct bitmask **);

int set_closest_numanode(get_node_bitmask get_bitmask,
                         void **closest_numanode, int num_cpu, bool is_single_node);
void set_bitmask_for_all_closest_numanodes(unsigned long *nodemask,
                                           unsigned long maxnode, const void *closest_numanode,
                                           int num_cpu);
int set_bitmask_for_current_closest_numanode(unsigned long *nodemask,
                                             unsigned long maxnode, const void *closest_numanode,
                                             int num_cpu);
int memkind_env_get_nodemask(char *nodes_env, struct bitmask **bm);


#ifdef __cplusplus
}
#endif
