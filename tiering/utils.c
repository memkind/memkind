// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <stdio.h>

#define UTILS_EXPORT __attribute__((visibility("default")))
#define UTILS_INIT   __attribute__((constructor))
#define UTILS_FINI   __attribute__((destructor))

static void UTILS_INIT utils_init(void)
{
    printf("tiering utils: hello world\n");
}

static void UTILS_FINI utils_fini(void)
{
    printf("tiering utils: bye\n");
}

UTILS_EXPORT void *test(size_t size)
{
    printf("tiering utils: test\n");
    return 0;
}

#ifndef __MALLOC_HOOK_VOLATILE
#define __MALLOC_HOOK_VOLATILE
#endif

void *(*__MALLOC_HOOK_VOLATILE __test_hook)(size_t size,
                                            const void *caller) = (void *)test;
