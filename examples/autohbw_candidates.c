// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */

///////////////////////////////////////////////////////////////////////////
// File   : autohbw_candidates.c
// Purpose: Shows which functions are interposed by AutoHBW library.
//        : These functions can be used for testing purposes
// Author : Ruchira Sasanka (ruchira.sasanka AT intel.com)
// Date   : Sept 10, 2015
///////////////////////////////////////////////////////////////////////////

#include <memkind.h>

#include <stdio.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////
// This function contains an example case for each heap allocation function
// intercepted by the AutoHBW library
///////////////////////////////////////////////////////////////////////////

// volatile is needed to prevent optimizing out below hooks
volatile int memkind_called_g;

void memkind_malloc_post(struct memkind *kind, size_t size, void **result)
{
    memkind_called_g = 1;
}
void memkind_calloc_post(struct memkind *kind, size_t nmemb, size_t size,
                         void **result)
{
    memkind_called_g = 1;
}
void memkind_posix_memalign_post(struct memkind *kind, void **memptr,
                                 size_t alignment, size_t size, int *err)
{
    memkind_called_g = 1;
}
void memkind_realloc_post(struct memkind *kind, void *ptr, size_t size,
                          void **result)
{
    memkind_called_g = 1;
}
void memkind_free_pre(struct memkind **kind, void **ptr)
{
    memkind_called_g = 1;
}

void finish_testcase(int fail_condition, const char *fail_message, int *err)
{

    if (memkind_called_g != 1 || fail_condition) {
        printf("%s\n", fail_message);
        *err = -1;
    }
    memkind_called_g = 0;
}

int main()
{
    int err = 0;
    const size_t size = 1024 * 1024; // 1M of data

    void *buf = NULL;
    memkind_called_g = 0;

    // Test 1: Test malloc and free
    buf = malloc(size);
    finish_testcase(buf == NULL, "Malloc failed!", &err);

    free(buf);
    finish_testcase(0, "Free after malloc failed!", &err);

    // Test 2: Test calloc and free
    buf = calloc(size, 1);
    finish_testcase(buf == NULL, "Calloc failed!", &err);

    free(buf);
    finish_testcase(0, "Free after calloc failed!", &err);

    // Test 3: Test realloc and free
    buf = malloc(size);
    finish_testcase(buf == NULL, "Malloc before realloc failed!", &err);

    buf = realloc(buf, size * 2);
    finish_testcase(buf == NULL, "Realloc failed!", &err);

    free(buf);
    finish_testcase(0, "Free after realloc failed!", &err);
    buf = NULL;

    // Test 4: Test posix_memalign and free
    int ret = posix_memalign(&buf, 64, size);
    finish_testcase(ret, "Posix_memalign failed!", &err);

    free(buf);
    finish_testcase(0, "Free after posix_memalign failed!", &err);

    return err;
}
