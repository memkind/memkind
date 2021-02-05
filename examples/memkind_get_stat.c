// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include <memkind.h>
#include <stdio.h>

int main()
{
    const size_t size = 1024;

    size_t stats_active;
    size_t stats_resident;
    size_t stats_allocated;
    size_t stats_kind_regular_active;
    size_t stats_kind_regular_resident;
    size_t stats_kind_regular_allocated;

    char *ptr_regular = (char *)memkind_malloc(MEMKIND_REGULAR, size);

    fprintf(
        stdout,
        "This example shows how to use memkind API to retrieve information about allocation stats.\n");

    snprintf(ptr_regular, size,
             "Hello world from regular kind memory - ptr_regular.\n");

    memkind_update_cached_stats();
    memkind_get_stat(NULL, MEMKIND_STAT_TYPE_RESIDENT, &stats_resident);
    memkind_get_stat(NULL, MEMKIND_STAT_TYPE_ACTIVE, &stats_active);
    memkind_get_stat(NULL, MEMKIND_STAT_TYPE_ALLOCATED, &stats_allocated);
    memkind_get_stat(MEMKIND_REGULAR, MEMKIND_STAT_TYPE_RESIDENT,
                     &stats_kind_regular_resident);
    memkind_get_stat(MEMKIND_REGULAR, MEMKIND_STAT_TYPE_ACTIVE,
                     &stats_kind_regular_active);
    memkind_get_stat(MEMKIND_REGULAR, MEMKIND_STAT_TYPE_ALLOCATED,
                     &stats_kind_regular_allocated);

    fprintf(stdout,
            "\n Global stats \nresident %zu \nactive %zu, \nallocated %zu \n",
            stats_resident, stats_active, stats_allocated);
    fprintf(
        stdout,
        "\n MEMKIND_REGULAR stats \nresident %zu \nactive %zu, \nallocated %zu \n",
        stats_kind_regular_resident, stats_kind_regular_active,
        stats_kind_regular_allocated);

    memkind_free(NULL, ptr_regular);

    return 0;
}
