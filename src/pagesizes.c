// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/pagesizes.h"

static bool is_pow2(size_t val)
{
    return val > 0ul && ((val & (val - 1)) == 0ul);
}

size_t traced_pagesize_get_sysytem_pagesize(void)
{
    return sysconf(_SC_PAGESIZE);
}

bool traced_pagesize_check_correctness(void)
{
    size_t system_pagesize = traced_pagesize_get_sysytem_pagesize();

    bool alignment_correct = ((TRACED_PAGESIZE % system_pagesize) == 0u) &&
        ((BIGARY_PAGESIZE % system_pagesize) == 0u);
    bool relative_size_correct = BIGARY_PAGESIZE >= TRACED_PAGESIZE;
    bool size_gt_0 = TRACED_PAGESIZE > 0ul;
    bool pow2_correct = is_pow2(TRACED_PAGESIZE) && is_pow2(BIGARY_PAGESIZE);

    return alignment_correct && relative_size_correct && size_gt_0 &&
        pow2_correct;
}
