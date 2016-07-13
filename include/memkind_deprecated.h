/*
 * Copyright (C) 2016 Intel Corporation.
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

/*
 * !!!!!!!!!!!!!!!!!!!
 * !!!   WARNING   !!!
 * !!! PLEASE READ !!!
 * !!!!!!!!!!!!!!!!!!!
 *
 * This header file contains all memkind deprecated symbols.
 *
 * Please avoid usage of this API in newly developed code, as
 * eventually this code is subject for removal.
 */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

struct memkind_ops {
    int (* create)(struct memkind *kind, const struct memkind_ops *ops, const char *name);
    int (* destroy)(struct memkind *kind);
    void *(* malloc)(struct memkind *kind, size_t size);
    void *(* calloc)(struct memkind *kind, size_t num, size_t size);
    int (* posix_memalign)(struct memkind *kind, void **memptr, size_t alignment, size_t size);
    void *(* realloc)(struct memkind *kind, void *ptr, size_t size);
    void (* free)(struct memkind *kind, void *ptr);
    void *(* mmap)(struct memkind *kind, void *addr, size_t size);
    int (* mbind)(struct memkind *kind, void *ptr, size_t size);
    int (* madvise)(struct memkind *kind, void *addr, size_t size);
    int (* get_mmap_flags)(struct memkind *kind, int *flags);
    int (* get_mbind_mode)(struct memkind *kind, int *mode);
    int (* get_mbind_nodemask)(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
    int (* get_arena)(struct memkind *kind, unsigned int *arena, size_t size);
    int (* get_size)(struct memkind *kind, size_t *total, size_t *free);
    int (* check_available)(struct memkind *kind);
    int (* check_addr)(struct memkind *kind, void *addr);
    void (*init_once)(void);
};

/*
 * There is work in progress to implement new functionality that allows
 * for dynamic kind creation.
 */
/* Create a new kind */
int DEPRECATED(memkind_create(const struct memkind_ops *ops, const char *name, memkind_t *kind));

#ifdef __cplusplus
}
#endif
