/*
 * Copyright (C) 2014 Intel Corperation.
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

#ifndef numakind_arena_include_h
#define numakind_arena_include_h
#ifdef __cplusplus
extern "C" {
#endif

#include "numakind.h"

int numakind_arena_create(struct numakind *kind, const struct numakind_ops *ops, const char *name);
int numakind_arena_create_map(struct numakind *kind);
int numakind_arena_destroy(struct numakind *kind);
void *numakind_arena_malloc(struct numakind *kind, size_t size);
void *numakind_arena_calloc(struct numakind *kind, size_t num, size_t size);
int numakind_arena_posix_memalign(struct numakind *kind, void **memptr, size_t alignment, size_t size);
void *numakind_arena_realloc(struct numakind *kind, void *ptr, size_t size);
void numakind_arena_free(struct numakind *kind, void *ptr);
int numakind_cpu_get_arena(struct numakind *kind, unsigned int *arena);
int numakind_bijective_get_arena(struct numakind *kind, unsigned int *arena);


#ifdef __cplusplus
}
#endif
#endif
