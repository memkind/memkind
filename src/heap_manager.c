// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2021 Intel Corporation. */

#include <memkind/internal/heap_manager.h>
#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/tbb_wrapper.h>

#include <pthread.h>
#include <stdio.h>
#include <string.h>

static struct heap_manager_ops *heap_manager_g;

static pthread_once_t heap_manager_init_once_g = PTHREAD_ONCE_INIT;

struct heap_manager_ops {
    void (*init)(struct memkind *kind);
    size_t (*heap_manager_malloc_usable_size)(void *ptr);
    void (*heap_manager_free)(void *ptr);
    void *(*heap_manager_realloc)(void *ptr, size_t size);
    struct memkind *(*heap_manager_detect_kind)(void *ptr);
    int (*heap_manager_update_cached_stats)(void);
    int (*heap_manager_get_stat)(memkind_stat_type stat, size_t *value);
    void *(*heap_manager_defrag_reallocate)(void *ptr);
    int (*heap_manager_set_bg_threads)(bool state);
};

static struct heap_manager_ops arena_heap_manager_g = {
    .init = memkind_arena_init,
    .heap_manager_malloc_usable_size = memkind_arena_malloc_usable_size,
    .heap_manager_free = memkind_arena_free_with_kind_detect,
    .heap_manager_realloc = memkind_arena_realloc_with_kind_detect,
    .heap_manager_detect_kind = memkind_arena_detect_kind,
    .heap_manager_update_cached_stats = memkind_arena_update_cached_stats,
    .heap_manager_get_stat = memkind_arena_get_global_stat,
    .heap_manager_defrag_reallocate =
        memkind_arena_defrag_reallocate_with_kind_detect,
    .heap_manager_set_bg_threads = memkind_arena_set_bg_threads};

static struct heap_manager_ops tbb_heap_manager_g = {
    .init = tbb_initialize,
    .heap_manager_malloc_usable_size =
        tbb_pool_malloc_usable_size_with_kind_detect,
    .heap_manager_free = tbb_pool_free_with_kind_detect,
    .heap_manager_realloc = tbb_pool_realloc_with_kind_detect,
    .heap_manager_detect_kind = tbb_detect_kind,
    .heap_manager_update_cached_stats = tbb_update_cached_stats,
    .heap_manager_get_stat = tbb_get_global_stat,
    .heap_manager_defrag_reallocate =
        tbb_pool_defrag_reallocate_with_kind_detect,
    .heap_manager_set_bg_threads = tbb_set_bg_threads};

static void set_heap_manager()
{
    heap_manager_g = &arena_heap_manager_g;
    const char *env = memkind_get_env("MEMKIND_HEAP_MANAGER");
    if (env && strcmp(env, "TBB") == 0) {
        heap_manager_g = &tbb_heap_manager_g;
    }
}

static inline struct heap_manager_ops *get_heap_manager()
{
    pthread_once(&heap_manager_init_once_g, set_heap_manager);
    return heap_manager_g;
}

void heap_manager_init(struct memkind *kind)
{
    get_heap_manager()->init(kind);
}

size_t heap_manager_malloc_usable_size(void *ptr)
{
    return get_heap_manager()->heap_manager_malloc_usable_size(ptr);
}

void heap_manager_free(void *ptr)
{
    get_heap_manager()->heap_manager_free(ptr);
}

void *heap_manager_realloc(void *ptr, size_t size)
{
    return get_heap_manager()->heap_manager_realloc(ptr, size);
}

struct memkind *heap_manager_detect_kind(void *ptr)
{
    return get_heap_manager()->heap_manager_detect_kind(ptr);
}

int heap_manager_update_cached_stats(void)
{
    return get_heap_manager()->heap_manager_update_cached_stats();
}

int heap_manager_get_stat(memkind_stat_type stat, size_t *value)
{
    return get_heap_manager()->heap_manager_get_stat(stat, value);
}

void *heap_manager_defrag_reallocate(void *ptr)
{
    return get_heap_manager()->heap_manager_defrag_reallocate(ptr);
}

int heap_manager_set_bg_threads(bool state)
{
    return get_heap_manager()->heap_manager_set_bg_threads(state);
}
