// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind/internal/vec.h>
#include <numa.h>
#include <stdbool.h>

struct vec_cpu_node;

typedef struct bitmask *(*get_node_bitmask)(void);

int set_closest_numanode(get_node_bitmask get_bitmask,
                         struct vec_cpu_node **closest_numanode, int num_cpu, bool is_single_node);
void set_bitmask_for_all_closest_numanodes(unsigned long *nodemask,
                                           unsigned long maxnode, const struct vec_cpu_node *closest_numanode,
                                           int num_cpu);
int set_bitmask_for_current_closest_numanode(unsigned long *nodemask,
                                             unsigned long maxnode, const struct vec_cpu_node *closest_numanode,
                                             int num_cpu);
struct bitmask *memkind_env_get_nodemask(char *nodes_env);


#ifdef __cplusplus
}
#endif
