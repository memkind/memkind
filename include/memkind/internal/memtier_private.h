// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once
#include <memkind_memtier.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include <assert.h>

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#define MEMKIND_ATOMIC _Atomic
#else
#define MEMKIND_ATOMIC
#endif

#if defined(MEMKIND_ATOMIC_C11_SUPPORT)
#define memkind_atomic_increment(counter, val)                                 \
    atomic_fetch_add_explicit(&counter, val, memory_order_relaxed)
#define memkind_atomic_decrement(counter, val)                                 \
    atomic_fetch_sub_explicit(&counter, val, memory_order_relaxed)
#elif defined(MEMKIND_ATOMIC_BUILTINS_SUPPORT)
#define memkind_atomic_increment(counter, val)                                 \
    __atomic_add_fetch(&counter, val, __ATOMIC_RELAXED)
#define memkind_atomic_decrement(counter, val)                                 \
    __atomic_sub_fetch(&counter, val, __ATOMIC_RELAXED)
#elif defined(MEMKIND_ATOMIC_SYNC_SUPPORT)
#define memkind_atomic_increment(counter, val)                                 \
    __sync_add_and_fetch(&counter, val)
#define memkind_atomic_decrement(counter, val)                                 \
    __sync_sub_and_fetch(&counter, val)
#else
#error "Missing atomic implementation."
#endif

struct memtier_tier {
    memkind_t kind;                   // Memory kind
    MEMKIND_ATOMIC size_t alloc_size; // Allocated size
};

struct memtier_tier_cfg {
    struct memtier_tier *tier; // Memory tier
    unsigned tier_ratio;       // Memory tier ratio
};

struct memtier_builder {
    unsigned size;                 // Number of memory tiers
    struct memtier_policy *policy; // Tiering policy
    struct memtier_tier_cfg *cfg;  // Memory Tier configuration
};

struct memtier_kind {
    struct memtier_builder *builder; // Tiering kind configuration
};

#define POLICY_NAME_LENGTH 64

struct memtier_policy {
    int (*create)(struct memtier_builder *builder);
    struct memtier_tier *(*get_tier)(struct memtier_kind *tier_kind);
    char name[POLICY_NAME_LENGTH];
    void *priv;
};

extern struct memtier_policy MEMTIER_POLICY_CIRCULAR_OBJ;

#ifdef __cplusplus
}
#endif
