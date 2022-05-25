// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    DRAM = 0,
    DAX_KMEM = 1
} memory_type_t;

void init_nodemasks();
int move_page_metadata(uintptr_t page, memory_type_t memory_type);
memory_type_t get_page_memory_type(uintptr_t page);
#ifdef __cplusplus
}
#endif
