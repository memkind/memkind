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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdlib.h>

#define VEC_INIT_SIZE (8)

#define VEC(name, type)\
struct name {\
    type *buffer;\
    size_t size;\
    size_t capacity;\
}

static inline int
vec_reserve(void *vec, size_t ncapacity, size_t s)
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

#define VEC_RESERVE(vec, ncapacity)\
(((vec)->size == 0 || (ncapacity) > (vec)->size) ?\
    vec_reserve((void *)vec, ncapacity, sizeof(*(vec)->buffer)) :\
    0)

#define VEC_INSERT(vec, element)\
((vec)->buffer[(vec)->size - 1] = (element), 0)

#define VEC_INC_SIZE(vec)\
(((vec)->size++), 0)

#define VEC_INC_BACK(vec)\
((vec)->capacity == (vec)->size ?\
    (VEC_RESERVE((vec), ((vec)->capacity * 2)) == 0 ?\
        VEC_INC_SIZE(vec) : -1) :\
    VEC_INC_SIZE(vec))

#define VEC_PUSH_BACK(vec, element)\
(VEC_INC_BACK(vec) == 0? VEC_INSERT(vec, element) : -1)

#define VEC_FOREACH(el, vec)\
size_t _vec_i; \
for (_vec_i = 0;\
    _vec_i < (vec)->size && (((el) = (vec)->buffer[_vec_i]), 1);\
    ++_vec_i)

#define VEC_SIZE(vec)\
((vec)->size)

#define VEC_GET(vec, id)\
(&(vec)->buffer[id])

#define VEC_CLEAR(vec) do {\
    (vec)->size = 0;\
} while (0)

#define VEC_DELETE(vec) do {\
    free((vec)->buffer);\
    (vec)->buffer = NULL;\
    (vec)->size = 0;\
    (vec)->capacity = 0;\
} while (0)

#ifdef __cplusplus
}
#endif
