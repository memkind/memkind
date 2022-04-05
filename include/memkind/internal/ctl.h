// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int ctl_parse_u(const char *str, unsigned *dest);
int ctl_parse_size_t(const char *str, size_t *dest);
int ctl_parse_size(char **sptr, size_t *sizep);

#ifdef __cplusplus
}
#endif
