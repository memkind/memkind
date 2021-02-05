// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind.h>
#include <memkind/internal/memkind_private.h>

/*
 * Header file for the default implementations for memkind operations.
 * More details in memkind_default(3) man page.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */

int memkind_default_create(struct memkind *kind, struct memkind_ops *ops,
                           const char *name);
int memkind_default_destroy(struct memkind *kind);
void *memkind_default_malloc(struct memkind *kind, size_t size);
void *memkind_default_calloc(struct memkind *kind, size_t num, size_t size);
int memkind_default_posix_memalign(struct memkind *kind, void **memptr,
                                   size_t alignment, size_t size);
void *memkind_default_realloc(struct memkind *kind, void *ptr, size_t size);
void memkind_default_free(struct memkind *kind, void *ptr);
void *memkind_default_mmap(struct memkind *kind, void *addr, size_t size);
int memkind_default_mbind(struct memkind *kind, void *ptr, size_t size);
int memkind_default_get_mmap_flags(struct memkind *kind, int *flags);
int memkind_default_get_mbind_mode(struct memkind *kind, int *mode);
int memkind_default_get_mbind_nodemask(struct memkind *kind,
                                       unsigned long *nodemask,
                                       unsigned long maxnode);
int memkind_preferred_get_mbind_mode(struct memkind *kind, int *mode);
int memkind_interleave_get_mbind_mode(struct memkind *kind, int *mode);
int memkind_nohugepage_madvise(struct memkind *kind, void *addr, size_t size);
int memkind_posix_check_alignment(struct memkind *kind, size_t alignment);
void memkind_default_init_once(void);
size_t memkind_default_malloc_usable_size(struct memkind *kind, void *ptr);
static inline bool size_out_of_bounds(size_t size)
{
    return !size;
}

extern struct memkind_ops MEMKIND_DEFAULT_OPS;

#ifdef __cplusplus
}
#endif
