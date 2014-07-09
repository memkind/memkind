/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
 */

#ifndef numakind_default_include_h
#define numakind_default_include_h
#ifdef __cplusplus
extern "C" {
#endif

#include "numakind.h"

int numakind_default_create(struct numakind *kind, const struct numakind_ops *ops, const char *name);
int numakind_default_destroy(struct numakind *kind);
void *numakind_default_malloc(struct numakind *kind, size_t size);
void *numakind_default_calloc(struct numakind *kind, size_t num, size_t size);
int numakind_default_posix_memalign(struct numakind *kind, void **memptr, size_t alignment, size_t size);
void *numakind_default_realloc(struct numakind *kind, void *ptr, size_t size);
void numakind_default_free(struct numakind *kind, void *ptr);
int numakind_default_is_available(struct numakind *kind);
int numakind_default_mbind(struct numakind *kind, void *ptr, size_t len);
int numakind_default_get_mmap_flags(struct numakind *kind, int *flags);
int numakind_default_get_mbind_mode(struct numakind *kind, int *mode);
int numakind_default_get_mbind_nodemask(struct numakind *kind, unsigned long *nodemask, unsigned long maxnode);

static const struct numakind_ops NUMAKIND_DEFAULT_OPS = {
    .create = numakind_default_create,
    .destroy = numakind_default_destroy,
    .malloc = numakind_default_malloc,
    .calloc = numakind_default_calloc,
    .posix_memalign = numakind_default_posix_memalign,
    .realloc = numakind_default_realloc,
    .free = numakind_default_free,
    .is_available = numakind_default_is_available,
    .mbind = numakind_default_mbind,
    .get_mmap_flags = numakind_default_get_mmap_flags,
    .get_mbind_mode = numakind_default_get_mbind_mode,
    .get_mbind_nodemask = numakind_default_get_mbind_nodemask,
    .get_arena = NULL
};

#ifdef __cplusplus
}
#endif
#endif
