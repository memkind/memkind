/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
 */

#ifndef numakind_arena_include_h
#define numakind_arena_include_h
#ifdef __cplusplus
extern "C" {
#endif

#include "numakind.h"

int numakind_arena_create(struct numakind *kind, const struct numakind_ops *ops, const char *name);
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
