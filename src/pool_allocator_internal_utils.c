// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/pool_allocator_internal_utils.h"

#include "stdbool.h"

/// \note must be > 0
#define MIN_RANK_SIZE_POW_2 (4u) // 2^4 == 16u

/// @brief Return the index of the most significant bit with value "1"
static uint64_t msb_set(uint64_t n)
{
    uint64_t idx;
    asm("bsr %1, %0" : "=r"(idx) : "r"(n));
    return idx;
}

/// @brief Return the index of the least significant bit with value "1"
static uint64_t lsb_set(uint64_t n)
{
    uint64_t idx;
    asm("bsf %1, %0" : "=r"(idx) : "r"(n));
    return idx;
}

size_t size_to_rank_size(size_t size)
{
    // find leftmost bit and rightmost bits
    uint64_t msb = msb_set(size);
    uint64_t lsb = lsb_set(size);

    // can be cleaned up/further optimised; good enough for POC
    if (msb == lsb) {
        bool msb_calculations = msb > MIN_RANK_SIZE_POW_2;
        uint64_t pow = msb - MIN_RANK_SIZE_POW_2;
        return msb_calculations * (pow << 1ul);
    } else {
        size_t msb_full = msb + 1ul;
        if (msb_full < MIN_RANK_SIZE_POW_2)
            return 0;
        bool mid_is_enough = (!(size & (1ul << (msb - 1ul))) ||
                              (lsb == msb - 1ul)); // second msb is set
        size_t ranks_to_add = !mid_is_enough; // 0 if is enough, 1 otherwise
        return ((msb - MIN_RANK_SIZE_POW_2) << 1ul) + 1ul + ranks_to_add;
    }
    // not reachable, all paths should have already returned
}

size_t rank_size_to_size(size_t size_rank)
{
    // calculate hash without any conditional statements
    size_t min_pow2 = (size_rank >> 1ul) + MIN_RANK_SIZE_POW_2;
    size_t min_size = (1ul) << min_pow2;
    bool rank_is_pow_2 = (size_rank & 1ul) == 0ul;
    // cast bool to int directly - write optimised, branchless code
    size_t size_to_add_pow2 = !rank_is_pow_2; // rank is pow 2 -> nothing to add
    size_t size_to_add = size_to_add_pow2 << (min_pow2 - 1ul);
    size_t size = min_size + size_to_add;

    return size;
}
