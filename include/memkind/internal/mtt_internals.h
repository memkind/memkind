// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "config.h"
#include "memkind/internal/fast_pool_allocator.h"
#include "memkind/internal/mmap_tracing_queue.h"
#include "memkind/internal/ranking.h"

#ifdef __cplusplus
#include <atomic>
#define _Atomic(x) std::atomic<x>
#define atomic_size_t _Atomic(size_t)
extern "C" {
#else

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#else
#define atomic_size_t size_t
#define _Atomic(x) x
#endif
#endif

/// Limits to be used for PMEM vs DRAM distribution
///
/// @pre The following property must be met lowLimit <= softLimit <= hardLimit
typedef struct MTTInternalsLimits {
    /// soft limit of used dram, data movement DRAM -> PMEM will occur when
    /// surpassed
    /// @pre must be a multiple of TRACED_PAGESIZE
    size_t softLimit;
    /// hard limit of used dram, this limit will not be surpassed by the
    /// allocator TODO @warning not implemented
    /// @pre must be a multiple of TRACED_PAGESIZE
    size_t hardLimit;
    /// value below which movement from PMEM -> DRAM occurs
    /// @pre must be a multiple of TRACED_PAGESIZE
    size_t lowLimit;
} MTTInternalsLimits;

typedef struct MttInternals {
    ranking_handle dramRanking;
    ranking_handle pmemRanking;
    /// Used as temporary variable for some internal operations
    metadata_handle tempMetadataHandle;
    FastPoolAllocator pool;
    uint64_t lastTimestamp;
    MTTInternalsLimits limits;
    MMapTracingQueue mmapTracingQueue;
    atomic_size_t usedDram;
} MttInternals;

extern int mtt_internals_create(MttInternals *internals, uint64_t timestamp,
                                const MTTInternalsLimits *limits,
                                MmapCallback *user_mmap);
extern void mtt_internals_destroy(MttInternals *internals);

extern void *mtt_internals_malloc(MttInternals *internals, size_t size,
                                  MmapCallback *user_mmap);
extern void *mtt_internals_realloc(MttInternals *internals, void *ptr,
                                   size_t size, MmapCallback *user_mmap);
extern void mtt_internals_free(MttInternals *internals, void *ptr);
extern size_t mtt_internals_usable_size(MttInternals *internals, void *ptr);

extern void mtt_internals_touch(MttInternals *internals, uintptr_t address);
/// @return size that could not be moved due to insufficient DRAM_RANKING size
extern size_t mtt_internals_ranking_balance(MttInternals *internals,
                                            atomic_size_t *used_dram,
                                            size_t dram_limit);
extern void mtt_internals_ranking_update(MttInternals *internals,
                                         uint64_t timestamp,
                                         atomic_size_t *used_dram);

#ifdef __cplusplus
}
#endif
