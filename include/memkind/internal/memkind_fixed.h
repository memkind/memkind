// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "memkind_arena.h"
#include "memkind_default.h"
#include <memkind.h>

#include <pthread.h>

int memkind_fixed_create(struct memkind *kind, struct memkind_ops *ops,
                         const char *name);
int memkind_fixed_destroy(struct memkind *kind);
int memkind_fixed_get_mmap_flags(struct memkind *kind, int *flags);
void *memkind_fixed_mmap(struct memkind *kind, void *addr, size_t size);
struct memkind_fixed {
    void *addr;
    size_t size;
    size_t current;
    pthread_mutex_t lock;
};

extern struct memkind_ops MEMKIND_FIXED_OPS;

#ifdef __cplusplus
}
#endif
