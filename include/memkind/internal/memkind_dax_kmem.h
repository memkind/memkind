// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind.h>

/*
 * Header file for the DAX KMEM memory memkind operations.
 * More details in memkind_dax_kmem(3) man page.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */

int memkind_dax_kmem_all_get_mbind_nodemask(struct memkind *kind,
                                            unsigned long *nodemask,
                                            unsigned long maxnode);

extern struct memkind_ops MEMKIND_DAX_KMEM_OPS;
extern struct memkind_ops MEMKIND_DAX_KMEM_ALL_OPS;
extern struct memkind_ops MEMKIND_DAX_KMEM_PREFERRED_OPS;
extern struct memkind_ops MEMKIND_DAX_KMEM_INTERLEAVE_OPS;

#ifdef __cplusplus
}
#endif
