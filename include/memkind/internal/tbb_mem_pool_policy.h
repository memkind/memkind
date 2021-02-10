// SPDX-License-Identifier: Apache-2.0
/* Copyright (C) 2005 - 2020 Intel Corporation. */

#include <stdint.h>

typedef void *(*rawAllocType)(intptr_t pool_id, size_t *bytes);
typedef int (*rawFreeType)(intptr_t pool_id, void *raw_ptr, size_t raw_bytes);

struct MemPoolPolicy {
    rawAllocType pAlloc;
    rawFreeType pFree;
    size_t granularity;
    int version;
    unsigned fixedPool : 1, keepAllMemory : 1, reserved : 30;
};
