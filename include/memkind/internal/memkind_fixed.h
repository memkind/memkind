// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "memkind.h"
#include "memkind_arena.h"
#include "memkind_default.h"

#include "pthread.h"

/*
 * Header file for the operations on user-supplied memory.
 * More details in memkind_fixed(3) man page.
 *
 * Functionality defined in this header is a part of EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */

int memkind_fixed_create(struct memkind *kind, struct memkind_ops *ops,
                         const char *name);
int memkind_fixed_destroy(struct memkind *kind);
/// @warning In contrast to POSIX mmap, does **not** align memory
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
