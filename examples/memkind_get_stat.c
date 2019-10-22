/*
 * Copyright (C) 2019 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

    fprintf(stdout,
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
    fprintf(stdout,
            "\n MEMKIND_REGULAR stats \nresident %zu \nactive %zu, \nallocated %zu \n",
            stats_kind_regular_resident, stats_kind_regular_active,
            stats_kind_regular_allocated);

    memkind_free(NULL, ptr_regular);

    return 0;
}
