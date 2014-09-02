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

#ifndef memkind_gbtlb_include_h
#define memkind_gbtlb_include_h
#ifdef __cplusplus
extern "C" {
#endif

#include "memkind.h"
#include "memkind_hbw.h"
#include "memkind_default.h"

int memkind_gbtlb_create(struct memkind *kind, const struct memkind_ops *ops, const char *name);
int memkind_gbtlb_destroy(struct memkind *kind);
void *memkind_gbtlb_malloc(struct memkind *kind, size_t size);
void *memkind_gbtlb_calloc(struct memkind *kind, size_t num, size_t size);
int memkind_gbtlb_posix_memalign(struct memkind *kind, void **memptr, size_t alignment, size_t size);
void *memkind_gbtlb_realloc(struct memkind *kind, void *ptr, size_t size);
void memkind_gbtlb_free(struct memkind *kind, void *ptr);
int memkind_gbtlb_mbind(struct memkind *kind, void *ptr, size_t len);
int memkind_gbtlb_get_mmap_flags(struct memkind *kind, int *flags);
int memkind_gbtlb_get_mbind_mode(struct memkind *kind, int *mode);
int memkind_gbtlb_preferred_get_mbind_mode(struct memkind *kind, int *mode);
size_t memkind_gbro_set_size(size_t size);
size_t memkind_noop_set_size(size_t size);

static const struct memkind_ops MEMKIND_HBW_GBTLB_OPS = {
    .create = memkind_gbtlb_create,
    .destroy = memkind_gbtlb_destroy,
    .malloc = memkind_gbtlb_malloc,
    .calloc = memkind_gbtlb_calloc,
    .posix_memalign = memkind_gbtlb_posix_memalign,
    .realloc = memkind_gbtlb_realloc,
    .free = memkind_gbtlb_free,
    .mbind = memkind_gbtlb_mbind,
    .get_mmap_flags = memkind_gbtlb_get_mmap_flags,
    .get_mbind_mode = memkind_gbtlb_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_size = memkind_default_get_size,
    .set_size = memkind_noop_set_size
};

static const struct memkind_ops MEMKIND_HBW_GBRO_OPS = {
    .create = memkind_gbtlb_create,
    .destroy = memkind_gbtlb_destroy,
    .malloc = memkind_gbtlb_malloc,
    .calloc = memkind_gbtlb_calloc,
    .posix_memalign = memkind_gbtlb_posix_memalign,
    .realloc = memkind_gbtlb_realloc,
    .free = memkind_gbtlb_free,
    .mbind = memkind_gbtlb_mbind,
    .get_mmap_flags = memkind_gbtlb_get_mmap_flags,
    .get_mbind_mode = memkind_gbtlb_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_size = memkind_default_get_size,
    .set_size = memkind_gbro_set_size
};

static const struct memkind_ops MEMKIND_HBW_PREFERRED_GBRO_OPS = {
    .create = memkind_gbtlb_create,
    .destroy = memkind_gbtlb_destroy,
    .malloc = memkind_gbtlb_malloc,
    .calloc = memkind_gbtlb_calloc,
    .posix_memalign = memkind_gbtlb_posix_memalign,
    .realloc = memkind_gbtlb_realloc,
    .free = memkind_gbtlb_free,
    .mbind = memkind_gbtlb_mbind,
    .get_mmap_flags = memkind_gbtlb_get_mmap_flags,
    .get_mbind_mode = memkind_gbtlb_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_size = memkind_default_get_size,
    .set_size = memkind_gbro_set_size
};

static const struct memkind_ops MEMKIND_HBW_PREFERRED_GBTLB_OPS = {
    .create = memkind_gbtlb_create,
    .destroy = memkind_gbtlb_destroy,
    .malloc = memkind_gbtlb_malloc,
    .calloc = memkind_gbtlb_calloc,
    .posix_memalign = memkind_gbtlb_posix_memalign,
    .realloc = memkind_gbtlb_realloc,
    .free = memkind_gbtlb_free,
    .mbind = memkind_gbtlb_mbind,
    .get_mmap_flags = memkind_gbtlb_get_mmap_flags,
    .get_mbind_mode = memkind_gbtlb_preferred_get_mbind_mode,
    .get_mbind_nodemask = memkind_hbw_get_mbind_nodemask,
    .get_size = memkind_default_get_size,
    .set_size = memkind_noop_set_size
};



#ifdef __cplusplus
}
#endif
#endif
