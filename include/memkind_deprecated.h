// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2021 Intel Corporation. */

/*
 * !!!!!!!!!!!!!!!!!!!
 * !!!   WARNING   !!!
 * !!! PLEASE READ !!!
 * !!!!!!!!!!!!!!!!!!!
 *
 * This header file contains all memkind deprecated symbols.
 *
 * Please avoid usage of this API in newly developed code, as
 * eventually this code is subject for removal.
 */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#ifndef MEMKIND_DEPRECATED

#ifdef __GNUC__
#define MEMKIND_DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define MEMKIND_DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement MEMKIND_DEPRECATED for this compiler")
#define MEMKIND_DEPRECATED(func) func
#endif

#endif

/*
 * Symbols related to GBTLB that are no longer supported
 */
extern memkind_t MEMKIND_HBW_GBTLB;
extern memkind_t MEMKIND_HBW_PREFERRED_GBTLB;
extern memkind_t MEMKIND_GBTLB;

int MEMKIND_DEPRECATED(memkind_get_kind_by_partition(int partition,
                                                     memkind_t *kind));

enum memkind_base_partition {
    MEMKIND_PARTITION_DEFAULT = 0,
    MEMKIND_PARTITION_HBW = 1,
    MEMKIND_PARTITION_HBW_HUGETLB = 2,
    MEMKIND_PARTITION_HBW_PREFERRED = 3,
    MEMKIND_PARTITION_HBW_PREFERRED_HUGETLB = 4,
    MEMKIND_PARTITION_HUGETLB = 5,
    MEMKIND_PARTITION_HBW_GBTLB = 6,
    MEMKIND_PARTITION_HBW_PREFERRED_GBTLB = 7,
    MEMKIND_PARTITION_GBTLB = 8,
    MEMKIND_PARTITION_HBW_INTERLEAVE = 9,
    MEMKIND_PARTITION_INTERLEAVE = 10,
    MEMKIND_PARTITION_REGULAR = 11,
    MEMKIND_PARTITION_HBW_ALL = 12,
    MEMKIND_PARTITION_HBW_ALL_HUGETLB = 13,
    MEMKIND_PARTITION_DAX_KMEM = 14,
    MEMKIND_PARTITION_DAX_KMEM_ALL = 15,
    MEMKIND_PARTITION_DAX_KMEM_PREFERRED = 16,
    MEMKIND_PARTITION_DAX_KMEM_INTERLEAVE = 17,
    MEMKIND_PARTITION_HIGHEST_CAPACITY = 18,
    MEMKIND_PARTITION_HIGHEST_CAPACITY_PREFERRED = 19,
    MEMKIND_PARTITION_HIGHEST_CAPACITY_LOCAL = 20,
    MEMKIND_NUM_BASE_KIND
};

#ifdef __cplusplus
}
#endif
