// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind/internal/memkind_private.h>

#include <numa.h>

typedef int (*get_node_bitmask)(struct bitmask **);

int set_closest_numanode(get_node_bitmask get_bitmask, void **numanode,
                         memkind_node_variant_t node_variant);
int set_bitmask_for_current_numanode(unsigned long *nodemask,
                                     unsigned long maxnode,
                                     const void *numanode);
int memkind_env_get_nodemask(char *nodes_env, struct bitmask **bm);

#ifdef __cplusplus
}
#endif
