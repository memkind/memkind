// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once
int ctl_load_config(char *buf, char **kind_name, char **pmem_path,
                    char **pmem_size, long long *ratio_value);
