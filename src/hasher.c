// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/hasher.h>
#include <memkind/internal/memkind_log.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "assert.h"
#include "pthread.h"
#include "stdbool.h"
#include "threads.h"

static thread_local void *stack_bottom = NULL;
static thread_local bool stack_bottom_initialized = NULL;

#define LSB_SET_9 (0b111111111)
#define MSB_SET_9 (0b1111111110000000)
#define LSB_SET_7 (0b1111111)

#ifndef MEMKIND_EXPORT
#define MEMKIND_EXPORT __attribute__((visibility("default")))
#endif

static void initialize_stack_bottom(void)
{
    size_t stack_size = 0; // value irrelevant
    pthread_attr_t attr;

    if (!stack_bottom_initialized) {
        stack_bottom_initialized = true;

        int ret = pthread_getattr_np(pthread_self(), &attr);
        assert(ret == 0 && "pthread get stack failed!");
        if (ret) {
            log_fatal("pthread get stack failed!");
        }

        pthread_attr_getstack(&attr, &stack_bottom, &stack_size);
        pthread_attr_destroy(&attr);
    }
}

static uint16_t calculate_stack_size_hash_9_bit(void)
{
    void *foo = NULL; // value irrelevant
    initialize_stack_bottom();
    // should work fine also when foo > stack bottom
    size_t diff = (void *)&foo - stack_bottom;
    // return 9 least significant bits
    // this should be enough for our requirements
    return (uint16_t)(diff & LSB_SET_9);
}

MEMKIND_EXPORT uint16_t hasher_calculate_hash(size_t size_rank)
{
    uint16_t stack_hash = calculate_stack_size_hash_9_bit();
    return (
        uint16_t)(0xFFFF &
                  (((stack_hash << 7) & MSB_SET_9) | (size_rank & LSB_SET_7)));
}
