// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdlib.h>

#define VEC_INIT_SIZE (8)

#define VEC(name, type)                                                        \
    struct name {                                                              \
        type *buffer;                                                          \
        size_t size;                                                           \
        size_t capacity;                                                       \
    }

#define VEC_INITIALIZER                                                        \
    {                                                                          \
        NULL, 0, 0                                                             \
    }

static inline int vec_reserve(void *vec, size_t ncapacity, size_t s)
{
    size_t ncap = ncapacity == 0 ? VEC_INIT_SIZE : ncapacity;
    VEC(vvec, void) *vecp = (struct vvec *)vec;
    void *tbuf = realloc(vecp->buffer, s * ncap);
    if (tbuf == NULL) {
        return -1;
    }
    vecp->buffer = tbuf;
    vecp->capacity = ncap;
    return 0;
}

#define VEC_RESERVE(vec, ncapacity)                                            \
    (((vec)->size == 0 || (ncapacity) > (vec)->size)                           \
         ? vec_reserve((void *)vec, ncapacity, sizeof(*(vec)->buffer))         \
         : 0)

#define VEC_INSERT(vec, element) ((vec)->buffer[(vec)->size - 1] = (element), 0)

#define VEC_INC_SIZE(vec) (((vec)->size++), 0)

#define VEC_INC_BACK(vec)                                                      \
    ((vec)->capacity == (vec)->size                                            \
         ? (VEC_RESERVE((vec), ((vec)->capacity * 2)) == 0 ? VEC_INC_SIZE(vec) \
                                                           : -1)               \
         : VEC_INC_SIZE(vec))

#define VEC_PUSH_BACK(vec, element)                                            \
    (VEC_INC_BACK(vec) == 0 ? VEC_INSERT(vec, element) : -1)

#define VEC_FOREACH(el, vec)                                                   \
    size_t _vec_i;                                                             \
    for (_vec_i = 0;                                                           \
         _vec_i < (vec)->size && (((el) = (vec)->buffer[_vec_i]), 1);          \
         ++_vec_i)

#define VEC_SIZE(vec) ((vec)->size)

#define VEC_CAPACITY(vec) ((vec)->capacity)

#define VEC_GET(vec, id) (&(vec)->buffer[id])

#define VEC_CLEAR(vec)                                                         \
    do {                                                                       \
        (vec)->size = 0;                                                       \
    } while (0)

#define VEC_DELETE(vec)                                                        \
    do {                                                                       \
        free((vec)->buffer);                                                   \
        (vec)->buffer = NULL;                                                  \
        (vec)->size = 0;                                                       \
        (vec)->capacity = 0;                                                   \
    } while (0)

#ifdef __cplusplus
}
#endif
