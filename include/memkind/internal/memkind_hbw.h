// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind.h>

/*
 * Header file for the high bandwidth memory memkind operations.
 * More details in memkind_hbw(3) man page.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */

int memkind_hbw_check_available(struct memkind *kind);
int memkind_hbw_hugetlb_check_available(struct memkind *kind);
int memkind_hbw_get_mbind_nodemask(struct memkind *kind,
                                   unsigned long *nodemask,
                                   unsigned long maxnode);
int memkind_hbw_all_get_mbind_nodemask(struct memkind *kind,
                                       unsigned long *nodemask,
                                       unsigned long maxnode);
void memkind_hbw_init_once(void);
void memkind_hbw_all_init_once(void);
void memkind_hbw_hugetlb_init_once(void);
void memkind_hbw_all_hugetlb_init_once(void);
void memkind_hbw_preferred_init_once(void);
void memkind_hbw_preferred_hugetlb_init_once(void);
void memkind_hbw_interleave_init_once(void);

extern struct memkind_ops MEMKIND_HBW_OPS;
extern struct memkind_ops MEMKIND_HBW_ALL_OPS;
extern struct memkind_ops MEMKIND_HBW_HUGETLB_OPS;
extern struct memkind_ops MEMKIND_HBW_ALL_HUGETLB_OPS;
extern struct memkind_ops MEMKIND_HBW_PREFERRED_OPS;
extern struct memkind_ops MEMKIND_HBW_PREFERRED_HUGETLB_OPS;
extern struct memkind_ops MEMKIND_HBW_INTERLEAVE_OPS;

#ifdef __cplusplus
}
#endif
