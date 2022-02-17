// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/pagesizes.h"

size_t traced_pagesize_get_sysytem_pagesize(void)
{
    return sysconf(_SC_PAGESIZE);
}

bool traced_pagesize_check_correctness(void)
{
    size_t system_pagesize = traced_pagesize_get_sysytem_pagesize();

    return ((TRACED_PAGESIZE % system_pagesize) == 0u) &&
        ((BIGARY_PAGESIZE % system_pagesize) == 0u);
}
