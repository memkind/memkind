// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "config.h"

#include <memkind/internal/hasher.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

#ifdef HAVE_THREADS_H
#include <threads.h>
#else
#ifndef __cplusplus
#define thread_local __thread
#endif
#endif

static thread_local void *stack_bottom = NULL;
static thread_local bool stack_bottom_initialized = false;

#define LSB_SET_9 (0b111111111)
#define MSB_SET_9 (0b1111111110000000)
#define LSB_SET_7 (0b1111111)

/// @brief Initialize data describing stack of the calling thread
static void initialize_stack_bottom(void)
{
    size_t stack_size = 0; // value irrelevant
    pthread_attr_t attr;

    // the stack_bottom_initialized flag is used instead of pthread_once
    // due to issues with pthread_once context
    if (!stack_bottom_initialized) {
        // this code should only be executed once per each calling thread
        stack_bottom_initialized = true;

        int ret = pthread_getattr_np(pthread_self(), &attr); // initialize attr
        assert(ret == 0 && "pthread get attr failed!");

        ret = pthread_attr_getstack(&attr, &stack_bottom, &stack_size);
        assert(ret == 0 && "pthread get stack failed!");

        ret = pthread_attr_destroy(&attr); // initialized in pthread_getattr_np
        assert(ret == 0 && "pthread attr destroy failed!");
    }
}

static uint16_t calculate_stack_size_hash_9_bit(void)
{
    void *foo = NULL; // value irrelevant
    initialize_stack_bottom();
    // should work fine also when foo < stack bottom
    // in such case, diff is two's compliment representation
    // msb is ignored, a change in stack size of has the same effect:
    // least significant bytes change first
    size_t diff = (void *)&foo - stack_bottom;
    // return 9 least significant bits
    // this should be enough for our requirements
    return (uint16_t)(diff & LSB_SET_9);
}

MEMKIND_EXPORT uint16_t hasher_calculate_hash(size_t size_rank)
{
    uint16_t stack_hash = calculate_stack_size_hash_9_bit();
    // expression are split due to clang formatting issues
    uint32_t ret = (((stack_hash << 7) & MSB_SET_9) | (size_rank & LSB_SET_7));

    return (uint16_t)(0xFFFF & ret);
}
