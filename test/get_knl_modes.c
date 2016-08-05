/*
 * Copyright (c) 2015, 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2009 CNRS
 * Copyright (c) 2009 inria.  All rights reserved.
 * Copyright (c) 2009 Université Bordeaux
 * Copyright (c) 2009 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012 Blue Brain Project, EPFL. All rights reserved.
 * Copyright (c) 2016 Inria.  All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>

#include <hwloc.h>

int main(void)
{
    hwloc_topology_t topology;
    hwloc_obj_t root;
    const char *cluster_mode;
    const char *memory_mode;

    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    root = hwloc_get_root_obj(topology);

    cluster_mode = hwloc_obj_get_info_by_name(root, "ClusterMode");
    memory_mode = hwloc_obj_get_info_by_name(root, "MemoryMode");

    printf ("ClusterMode is '%s' MemoryMode is '%s'\n",
        cluster_mode ? cluster_mode : "NULL",
        memory_mode ? memory_mode : "NULL");

    hwloc_topology_destroy(topology);
    return 0;
}

