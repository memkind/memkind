/*
 * Copyright (C) 2019 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <numa.h>

struct bandwidth_nodes_t;

struct bandwidth_closest_numanode_t {
    int init_err;
    int num_cpu;
    int *closest_numanode;
};

typedef int (*get_node_bitmask)(struct bitmask *);
typedef int (*fill_bandwidth_values)(int *);

int bandwidth_create_nodes(const int *bandwidth, int *num_unique,
                           struct bandwidth_nodes_t **bandwidth_nodes);
int bandwidth_fill(int *bandwidth, get_node_bitmask get_bitmask);
int bandwidth_fill_nodes(int *bandwidth, fill_bandwidth_values fill,
                         const char *env);
int bandwidth_set_closest_numanode(int num_unique,
                                   const struct bandwidth_nodes_t *bandwidth_nodes,
                                   int num_cpunode, int *closest_numanode);

#ifdef __cplusplus
}
#endif
