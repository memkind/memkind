// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

int ctl_load_config(char *buf, char **kind_name, char **pmem_path,
                    unsigned *pmem_size, unsigned *ratio_value);

#ifdef __cplusplus
}
#endif
