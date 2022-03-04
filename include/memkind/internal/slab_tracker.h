// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "memkind/internal/fast_slab_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void SlabTracker;

extern void fast_slab_tracker_create(SlabTracker **slab_tracker);
extern void fast_slab_tracker_destroy(SlabTracker *slab_tracker);

extern void fast_slab_tracker_register(SlabTracker *slab_tracker,
                                       uintptr_t addr,
                                       FastSlabAllocator *fast_slab_allocator);
extern FastSlabAllocator *
fast_slab_tracker_get_fast_slab(SlabTracker *slab_tracker, uintptr_t addr);

#ifdef __cplusplus
}
#endif
