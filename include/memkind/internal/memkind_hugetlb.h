// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind.h>

#include <numa.h>

/*
 * Header file for the hugetlb memory memkind operations.
 * More details in memkind_hugetlb(3) man page.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */

int memkind_hugetlb_get_mmap_flags(struct memkind *kind, int *flags);
void memkind_hugetlb_init_once(void);
int memkind_hugetlb_check_available_2mb(struct memkind *kind);
int get_nr_hugepages_cached(size_t pagesize, struct bitmask *nodemask,
                            size_t *out);
int get_nr_overcommit_hugepages_cached(size_t pagesize, size_t *out);

extern struct memkind_ops MEMKIND_HUGETLB_OPS;

#ifdef __cplusplus
}
#endif
