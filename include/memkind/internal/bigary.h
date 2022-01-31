// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021-2022 Intel Corporation. */

#pragma once
#include "pthread.h"
#include "stdlib.h" // size_t
#include "sys/mman.h"

#define BIGARY_DRAM -1, MAP_ANONYMOUS | MAP_PRIVATE

#ifdef __cplusplus
#define restrict __restrict__
#endif

struct bigary {
    void *area;
    size_t declared;
    size_t top;
    int fd;
    int flags;
    pthread_mutex_t enlargement;
};
typedef struct bigary bigary;

extern void bigary_init(bigary *restrict m_bigary, int fd, int flags,
                        size_t max);
extern void bigary_alloc(bigary *restrict m_bigary, size_t top);
extern void bigary_destroy(bigary *restrict m_bigary);
