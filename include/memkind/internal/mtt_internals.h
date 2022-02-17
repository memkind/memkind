// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "config.h"
#include "memkind/internal/pool_allocator.h"
#include "memkind/internal/ranking.h"

typedef struct MTTInternalsLimits {
    /// soft limit of used dram, data movement will occur when surpassed
    size_t softLimit;
    /// hard limit of used dram, this limit will not be surpassed by the
    /// allocator TODO @warning not implemented
    size_t hardLimit;
    /// value below which movement from pmem to dram occurs
    size_t lowLimit;
} MTTInternalsLimits;

typedef struct MttInternals {
    ranking_handle dramRanking;
    ranking_handle pmemRanking;
    PoolAllocator pool;
    uint64_t lastTimestamp;
    MTTInternalsLimits limits;
} MttInternals;

extern int mtt_internals_create(MttInternals *internals, uint64_t timestamp,
                                MTTInternalsLimits *limits);
extern void mtt_internals_destroy(MttInternals *internals);

extern void *mtt_internals_malloc(MttInternals *internals, size_t size);
extern void *mtt_internals_realloc(MttInternals *internals, void *ptr,
                                   size_t size);
extern void mtt_internals_free(void *ptr);

extern void mtt_internals_touch(MttInternals *internals, uintptr_t address);
extern void mtt_internals_ranking_update(MttInternals *internals,
                                         uint64_t timestamp);
