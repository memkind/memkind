// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */

#include "Allocation_info.hpp"
#include <numaif.h>

double convert_bytes_to_mb(uint64_t bytes)
{
    return bytes / (1024.0 * 1024.0);
}

int get_numa_node_id(void *ptr)
{
    int status = -1;

    get_mempolicy(&status, NULL, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
    return status;
}
