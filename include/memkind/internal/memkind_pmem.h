// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "memkind_arena.h"
#include "memkind_default.h"
#include <memkind.h>

#include <pthread.h>

/*
 * Header file for the file-backed memory memkind operations.
 * More details in memkind_pmem(3) man page.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */

#define MEMKIND_PMEM_CHUNK_SIZE (1ull << 21ull) // 2MB

int memkind_pmem_create(struct memkind *kind, struct memkind_ops *ops,
                        const char *name);
int memkind_pmem_destroy(struct memkind *kind);
void *memkind_pmem_mmap(struct memkind *kind, void *addr, size_t size);
int memkind_pmem_get_mmap_flags(struct memkind *kind, int *flags);
int memkind_pmem_create_tmpfile(const char *dir, int *fd);
int memkind_pmem_validate_dir(const char *dir);

struct memkind_pmem {
    int fd;
    off_t offset;
    size_t max_size;
    pthread_mutex_t pmem_lock;
    size_t current_size;
    char *dir;
};

extern struct memkind_ops MEMKIND_PMEM_OPS;

#ifdef __cplusplus
}
#endif
