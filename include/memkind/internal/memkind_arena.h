// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <jemalloc/jemalloc.h>
#include <memkind.h>
#include <memkind/internal/memkind_private.h>

/*
 * Header file for the jemalloc arena allocation memkind operations.
 * More details in memkind_arena(3) man page.
 *
 * Functionality defined in this header is considered as EXPERIMENTAL API.
 * API standards are described in memkind(3) man page.
 */

struct memkind *get_kind_by_arena(unsigned arena_ind);
struct memkind *memkind_arena_detect_kind(void *ptr);
int memkind_arena_create(struct memkind *kind, struct memkind_ops *ops,
                         const char *name);
int memkind_arena_create_map(struct memkind *kind, extent_hooks_t *hooks);
int memkind_arena_destroy(struct memkind *kind);
void *memkind_arena_malloc(struct memkind *kind, size_t size);
void *memkind_arena_calloc(struct memkind *kind, size_t num, size_t size);
int memkind_arena_posix_memalign(struct memkind *kind, void **memptr,
                                 size_t alignment, size_t size);
void *memkind_arena_realloc(struct memkind *kind, void *ptr, size_t size);
void *memkind_arena_realloc_with_kind_detect(void *ptr, size_t size);
int memkind_bijective_get_arena(struct memkind *kind, unsigned int *arena,
                                size_t size);
int memkind_thread_get_arena(struct memkind *kind, unsigned int *arena,
                             size_t size);
int memkind_arena_finalize(struct memkind *kind);
void memkind_arena_init(struct memkind *kind);
void memkind_arena_free(struct memkind *kind, void *ptr);
void memkind_arena_free_with_kind_detect(void *ptr);
size_t memkind_arena_malloc_usable_size(void *ptr);
int memkind_arena_update_memory_usage_policy(struct memkind *kind,
                                             memkind_mem_usage_policy policy);
int memkind_arena_set_max_bg_threads(size_t threads_limit);
int memkind_arena_set_bg_threads(bool state);
int memkind_arena_update_cached_stats(void);
int memkind_arena_get_kind_stat(struct memkind *kind,
                                memkind_stat_type stat_type, size_t *stat);
int memkind_arena_get_stat_with_check_init(struct memkind *kind,
                                           memkind_stat_type stat,
                                           bool check_init, size_t *value);
int memkind_arena_get_global_stat(memkind_stat_type stat_type, size_t *stat);
void *memkind_arena_defrag_reallocate(struct memkind *kind, void *ptr);
void *memkind_arena_defrag_reallocate_with_kind_detect(void *ptr);
bool memkind_get_hog_memory(void);
#ifdef __cplusplus
}
#endif
