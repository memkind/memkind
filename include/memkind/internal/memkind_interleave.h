// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2019 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Header file for the interleave memory memkind operations.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */

void memkind_interleave_init_once(void);

extern struct memkind_ops MEMKIND_INTERLEAVE_OPS;

#ifdef __cplusplus
}
#endif
