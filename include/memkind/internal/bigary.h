// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021-2022 Intel Corporation. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pthread.h"
#include "stdint.h"
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

typedef void *(mmap_wrapper)(void *arg, void *__addr, size_t __len, int __prot,
                             int __flags, int __fd, __off_t __offset);

typedef struct mmap_callback {
    void *arg;
    mmap_wrapper *wrapped_mmap;
} MmapCallback;

static void *wrapped_stdmmap(void *arg, void *__addr, size_t __len, int __prot,
                             int __flags, int __fd, __off_t __offset)
{
    (void)arg;
    return mmap(__addr, __len, __prot, __flags, __fd, __offset);
}

static const MmapCallback gStandardMmapCallback = {
    .arg = NULL,
    .wrapped_mmap = wrapped_stdmmap,
};

extern void bigary_init(bigary *restrict m_bigary, int fd, int flags,
                        size_t max);
extern void bigary_init_pages(bigary *restrict m_bigary, int fd, int flags,
                              size_t max, uintptr_t *address, size_t *nof_pages,
                              const MmapCallback *m_mmap);
extern void bigary_alloc(bigary *restrict m_bigary, size_t top);
extern void bigary_alloc_pages(bigary *restrict m_bigary, size_t top,
                               uintptr_t *address, size_t *nof_pages,
                               const MmapCallback *m_mmap);
extern void bigary_destroy(bigary *restrict m_bigary);

#ifdef __cplusplus
}
#endif
