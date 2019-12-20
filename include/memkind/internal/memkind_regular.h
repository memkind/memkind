// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2019 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind.h>

/*
 * Header file for the regular memory memkind operations.
 * More details in memkind_regular(3) man page.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */

extern struct memkind_ops MEMKIND_REGULAR_OPS;
int memkind_regular_all_get_mbind_nodemask(struct memkind *kind,
                                           unsigned long *nodemask,
                                           unsigned long maxnode);

#ifdef __cplusplus
}
#endif
