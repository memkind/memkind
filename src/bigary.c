// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021-2022 Intel Corporation. */

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "memkind/internal/bigary.h"
// default max: 16 GB
// #define BIGARY_DEFAULT_MAX (16 * 1024 * 1024 * 1024ULL)
#define BIGARY_DEFAULT_MAX (4 * 1024 * 1024 * 1024ULL)
#define BIGARY_PAGESIZE    2097152

static void die(const char *fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    // TODO refactor/remove this exit function
    if (write(STDERR_FILENO, buf, len))
        ;
    exit(1);
}

/***************************/
/* initialize a new bigary */
/***************************/
void bigary_init(bigary *restrict ba, int fd, int flags, size_t max)
{
    if (!max)
        max = BIGARY_DEFAULT_MAX;
    // round *max* up to pagesize
    size_t last_page_size = max % BIGARY_PAGESIZE;
    if (last_page_size) {
        size_t pages = max / BIGARY_PAGESIZE + 1;
        max = pages * BIGARY_PAGESIZE;
    }
    int ret = pthread_mutex_init(&ba->enlargement, NULL);
    if (ret != 0)
        die("mutex init failed\n");
    ba->declared = max;
    ba->fd = fd;
    ba->flags = flags;
    if ((ba->area = mmap(0, max, PROT_NONE, flags, fd, 0)) == MAP_FAILED)
        die("mmapping bigary(%zd) failed: %m\n", max);
    if (mmap(ba->area, BIGARY_PAGESIZE, PROT_READ | PROT_WRITE,
             MAP_FIXED | flags, fd, 0) == MAP_FAILED) {
        die("bigary alloc of %zd failed: %m\n", BIGARY_PAGESIZE);
    }
    ba->top = BIGARY_PAGESIZE;
}

void bigary_destroy(bigary *restrict ba)
{
    int ret1 = pthread_mutex_destroy(&ba->enlargement);
    int ret2 = munmap(ba->area, ba->declared);
    assert(ret1 == 0 && "mutex destruction failed!");
    assert(ret2 == 0 && "unmap failed!");
}

/********************************************************************/
/* ensure there's at least X space allocated                        */
/* (you may want MAP_POPULATE to ensure the space is actually there */
/********************************************************************/
void bigary_alloc(bigary *restrict ba, size_t top)
{
    if (ba->top >= top)
        return;
    pthread_mutex_lock(&ba->enlargement);
    if (ba->top >= top) // re-check
        goto done;
    top = (top + BIGARY_PAGESIZE - 1) & ~(BIGARY_PAGESIZE - 1); // align up
    // printf("extending to %zd\n", top);
    if (top > ba->declared)
        die("bigary's max is %zd, %zd requested.\n", ba->declared, top);
    if (mmap(ba->area + ba->top, top - ba->top, PROT_READ | PROT_WRITE,
             MAP_FIXED | ba->flags, ba->fd, ba->top) == MAP_FAILED) {
        die("in-bigary alloc of %zd to %zd failed: %m\n", top - ba->top, top);
    }
    ba->top = top;
done:
    pthread_mutex_unlock(&ba->enlargement);
}
