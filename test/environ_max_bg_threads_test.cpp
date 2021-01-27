// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind.h>
#include <test/proc_stat.h>
#include <unistd.h>

#include "common.h"

int main()
{
    ProcStat proc_stat;
    const size_t alloc_size = 1 * MB;

    void *ptr = memkind_malloc(MEMKIND_REGULAR, alloc_size);
    if (ptr == nullptr) {
        printf("Error: allocation failed\n");
        return -1;
    }
    memset(ptr, 'a', alloc_size);

    if (memkind_set_bg_threads(false) & memkind_set_bg_threads(true)) {
        printf("Error: failed to set the state of background threads option\n");
        return -1;
    }
    sleep(2);
    unsigned threads_count = proc_stat.get_threads_count();

    memkind_free(MEMKIND_REGULAR, ptr);
    return threads_count;
}
