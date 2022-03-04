// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/slab_tracker.h"
#include "memkind/internal/memkind_private.h"

#include <cassert>
#include <unordered_map>

struct SlabTrackerInternals {
    std::unordered_map<uintptr_t, FastSlabAllocator *> addrToSlab;
};

MEMKIND_EXPORT void fast_slab_tracker_create(SlabTracker **slab_tracker)
{
    *slab_tracker = new SlabTrackerInternals();
}

MEMKIND_EXPORT void fast_slab_tracker_destroy(SlabTracker *slab_tracker)
{
    delete static_cast<SlabTrackerInternals *>(slab_tracker);
}

MEMKIND_EXPORT void
fast_slab_tracker_register(SlabTracker *slab_tracker, uintptr_t addr,
                           FastSlabAllocator *fast_slab_allocator)
{
    assert(fast_slab_allocator && "fast_slab_allocator cannot be NULL!");
    static_cast<SlabTrackerInternals *>(slab_tracker)->addrToSlab[addr] =
        fast_slab_allocator;
}

MEMKIND_EXPORT FastSlabAllocator *
fast_slab_tracker_get_fast_slab(SlabTracker *slab_tracker, uintptr_t addr)
{
    return static_cast<SlabTrackerInternals *>(slab_tracker)->addrToSlab[addr];
}
