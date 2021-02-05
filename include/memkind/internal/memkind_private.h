// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "memkind.h"

#include <pthread.h>
#include <stdbool.h>

#ifdef __GNUC__
#define MEMKIND_LIKELY(x) __builtin_expect(!!(x), 1)
#define MEMKIND_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define MEMKIND_LIKELY(x) (x)
#define MEMKIND_UNLIKELY(x) (x)
#endif

#ifndef MEMKIND_EXPORT
#define MEMKIND_EXPORT __attribute__((visibility("default")))
#endif

// This ladder call is required due to meanders of C's preprocessor logic.
// Without it, JE_PREFIX would be used directly (i.e. 'JE_PREFIX') and not
// substituted with defined value.
#define JE_SYMBOL2(a, b) a##b
#define JE_SYMBOL1(a, b) JE_SYMBOL2(a, b)
#define JE_SYMBOL(b) JE_SYMBOL1(JE_PREFIX, b)

// Redefine symbols
#define jemk_malloc JE_SYMBOL(malloc)
#define jemk_mallocx JE_SYMBOL(mallocx)
#define jemk_calloc JE_SYMBOL(calloc)
#define jemk_rallocx JE_SYMBOL(rallocx)
#define jemk_realloc JE_SYMBOL(realloc)
#define jemk_mallctl JE_SYMBOL(mallctl)
#define jemk_memalign JE_SYMBOL(memalign)
#define jemk_posix_memalign JE_SYMBOL(posix_memalign)
#define jemk_free JE_SYMBOL(free)
#define jemk_dallocx JE_SYMBOL(dallocx)
#define jemk_malloc_usable_size JE_SYMBOL(malloc_usable_size)
#define jemk_arenalookupx JE_SYMBOL(arenalookupx)
#define jemk_check_reallocatex JE_SYMBOL(check_reallocatex)

enum memkind_const_private
{
    MEMKIND_NAME_LENGTH_PRIV = 64
};

struct memkind_ops {
    int (*create)(struct memkind *kind, struct memkind_ops *ops,
                  const char *name);
    int (*destroy)(struct memkind *kind);
    void *(*malloc)(struct memkind *kind, size_t size);
    void *(*calloc)(struct memkind *kind, size_t num, size_t size);
    int (*posix_memalign)(struct memkind *kind, void **memptr, size_t alignment,
                          size_t size);
    void *(*realloc)(struct memkind *kind, void *ptr, size_t size);
    void (*free)(struct memkind *kind, void *ptr);
    void *(*mmap)(struct memkind *kind, void *addr, size_t size);
    int (*mbind)(struct memkind *kind, void *ptr, size_t size);
    int (*madvise)(struct memkind *kind, void *addr, size_t size);
    int (*get_mmap_flags)(struct memkind *kind, int *flags);
    int (*get_mbind_mode)(struct memkind *kind, int *mode);
    int (*get_mbind_nodemask)(struct memkind *kind, unsigned long *nodemask,
                              unsigned long maxnode);
    int (*get_arena)(struct memkind *kind, unsigned int *arena, size_t size);
    int (*check_available)(struct memkind *kind);
    void (*init_once)(void);
    int (*finalize)(struct memkind *kind);
    size_t (*malloc_usable_size)(struct memkind *kind, void *ptr);
    int (*update_memory_usage_policy)(struct memkind *kind,
                                      memkind_mem_usage_policy policy);
    int (*get_stat)(memkind_t kind, memkind_stat_type stat, size_t *value);
    void *(*defrag_reallocate)(struct memkind *kind, void *ptr);
};

struct memkind {
    struct memkind_ops *ops;
    unsigned int partition;
    char name[MEMKIND_NAME_LENGTH_PRIV];
    pthread_once_t init_once;
    unsigned int arena_map_len; // is power of 2
    unsigned int *arena_map;    // To be deleted beyond 1.2.0+
    pthread_key_t arena_key;
    void *priv;
    unsigned int arena_map_mask; // arena_map_len - 1 to optimize modulo
                                 // operation on arena_map_len
    unsigned int arena_zero;     // index first jemalloc arena of this kind
};

struct memkind_config {
    const char *pmem_dir;            // PMEM kind path
    size_t pmem_size;                // PMEM kind size
    memkind_mem_usage_policy policy; // kind memory usage policy
};

typedef enum memkind_node_variant_t
{
    NODE_VARIANT_MULTIPLE = 0,
    NODE_VARIANT_SINGLE = 1,
    NODE_VARIANT_MAX = 2,
    NODE_VARIANT_ALL = NODE_VARIANT_MAX,
    NODE_VARIANT_MAX_EXT = 3
} memkind_node_variant_t;

void memkind_init(memkind_t kind, bool check_numa);

void *kind_mmap(struct memkind *kind, void *addr, size_t size);

char *memkind_get_env(const char *name);

#ifdef __cplusplus
}
#endif
