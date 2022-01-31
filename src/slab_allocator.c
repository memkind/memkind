// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021-2022 Intel Corporation. */

#include "memkind/internal/slab_allocator.h"

#include "assert.h"
#include "pthread.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"

#include "stdatomic.h"

// -------- typedefs ----------------------------------------------------------

#ifndef MEMKIND_EXPORT
#define MEMKIND_EXPORT __attribute__((visibility("default")))
#endif

// -------- static functions --------------------------------------------------

static freelist_node_meta_t *slab_alloc_addr_to_node_meta_(void *addr)
{
    assert(addr);
    return (freelist_node_meta_t *)(((uint8_t *)addr) -
                                    sizeof(freelist_node_meta_t));
}

static void *slab_alloc_node_meta_to_addr_(freelist_node_meta_t *meta)
{
    assert(meta);
    return (void *)(((uint8_t *)meta) + sizeof(freelist_node_meta_t));
}

#ifdef USE_LOCKLESS

static void slab_alloc_glob_freelist_push_(void *addr)
{
    freelist_node_meta_t *meta = slab_alloc_addr_to_node_meta_(addr);
    slab_alloc_t *alloc = meta->allocator;
    do {
        meta->next = alloc->globFreelist.freelist;
    } while (false ==
             atomic_compare_exchange_weak(&alloc->globFreelist.freelist,
                                          &meta->next, meta));
}

static void *slab_alloc_glob_freelist_pop_(slab_alloc_t *alloc)
{
    freelist_node_meta_t *meta = NULL;
    do {
        meta = alloc->globFreelist.freelist;
        if (!meta)
            break;
    } while (false ==
             atomic_compare_exchange_weak(&alloc->globFreelist.freelist, &meta,
                                          meta->next));

    return meta ? slab_alloc_node_meta_to_addr_(meta) : NULL;
}

#else

static void slab_alloc_glob_freelist_lock_(slab_alloc_t *alloc)
{
    int ret = pthread_mutex_lock(&alloc->globFreelist.mutex);
    assert(ret == 0 && "mutex lock failed!");
}

static void slab_alloc_glob_freelist_unlock_(slab_alloc_t *alloc)
{
    int ret = pthread_mutex_unlock(&alloc->globFreelist.mutex);
    assert(ret == 0 && "mutex unlock failed!");
}

static void slab_alloc_glob_freelist_push_(void *addr)
{
    freelist_node_meta_t *meta = slab_alloc_addr_to_node_meta_(addr);
    slab_alloc_t *alloc = meta->allocator;
    slab_alloc_glob_freelist_lock_(alloc);
    meta->next = alloc->globFreelist.freelist;
    alloc->globFreelist.freelist = meta;
    slab_alloc_glob_freelist_unlock_(alloc);
}

static void *slab_alloc_glob_freelist_pop_(slab_alloc_t *alloc)
{
    slab_alloc_glob_freelist_lock_(alloc);
    freelist_node_meta_t *meta = alloc->globFreelist.freelist;
    if (meta)
        alloc->globFreelist.freelist = alloc->globFreelist.freelist->next;
    slab_alloc_glob_freelist_unlock_(alloc);

    return meta ? slab_alloc_node_meta_to_addr_(meta) : NULL;
}

#endif

static size_t slab_alloc_fetch_increment_used_(slab_alloc_t *alloc)
{
    // the value is never atomically decreased, only thing we need is
    return atomic_fetch_add_explicit(&alloc->used, 1u, memory_order_relaxed);
}

static freelist_node_meta_t *slab_alloc_create_meta_(slab_alloc_t *alloc)
{
    size_t free_idx = slab_alloc_fetch_increment_used_(alloc);
    size_t meta_offset = alloc->elementSize * free_idx;

    // TODO handle failure gracefully instead of die - in biary_alloc
    bigary_alloc(&alloc->mappedMemory, (free_idx + 1) * alloc->elementSize);
    void *ret_ = ((uint8_t *)alloc->mappedMemory.area) + meta_offset;
    freelist_node_meta_t *ret = ret_;
    ret->allocator = alloc;
    ret->next = NULL;

    return ret;
}

// -------- public functions --------------------------------------------------

MEMKIND_EXPORT int slab_alloc_init(slab_alloc_t *alloc, size_t element_size,
                                   size_t max_elements)
{
    // TODO handle failure gracefully instead of die - in biary_alloc
    alloc->elementSize = sizeof(freelist_node_meta_t) + element_size;
    size_t max_elements_size = max_elements * alloc->elementSize;
    bigary_init(&alloc->mappedMemory, BIGARY_DRAM, max_elements_size);
    alloc->used = 0u;

    int ret = pthread_mutex_init(&alloc->globFreelist.mutex, NULL);
    alloc->globFreelist.freelist = NULL;
    if (ret != 0)
        return ret;

    return ret;
}

MEMKIND_EXPORT void slab_alloc_destroy(slab_alloc_t *alloc)
{
    int ret = pthread_mutex_destroy(&alloc->globFreelist.mutex);
    bigary_destroy(&alloc->mappedMemory);
    assert(ret == 0 && "mutex destruction failed");
}

MEMKIND_EXPORT void *slab_alloc_malloc(slab_alloc_t *alloc)
{
    void *ret = slab_alloc_glob_freelist_pop_(alloc);
    if (!ret) {
        freelist_node_meta_t *meta = slab_alloc_create_meta_(alloc);
        if (meta) // defensive programming
            ret = slab_alloc_node_meta_to_addr_(meta);
    }
    return ret;
}

MEMKIND_EXPORT void slab_alloc_free(void *addr)
{
    assert(addr);
    slab_alloc_glob_freelist_push_(addr);
}
