/*
 * Copyright (C) 2015 - 2017 Intel Corporation.
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

#include <memkind.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_arena.h>

#include <pthread.h>

/*
 *  The pmem memory memkind operations enable memory kinds built on memory-mapped
 *  files. These support traditional volatile memory allocation in a fashion
 *  similar to libvmem(3) library. It uses the mmap(2) system call to create
 *  a pool of volatile memory. Such memory may have different attributes,
 *  depending on the file system containing the memory-mapped files. (See also
 *  http://pmem.io/pmdk/libvmem).
 *
 *  The pmem memkinds are most useful when used with Direct Access storage (DAX),
 *  which is memory-addressable persistent storage that supports load/store access
 *  without being paged via the system page cache. A Persistent Memory-aware file
 *  system is typically used to provide this type of access.
 *
 *  The most convenient way to create pmem memkinds is to use memkind_create_pmem()
 *  (see memkind(3)).
 */

#define	MEMKIND_PMEM_MIN_SIZE (1024 * 1024 * 16)
#define MEMKIND_PMEM_CHUNK_SIZE (1ull << 21ull) // 2MB

/*
 * memkind_pmem_create() is an implementation of the memkind "create" operation
 * for file-backed memory kinds. This allocates a space for some pmem-specific
 * metadata, then calls memkind_arena_create() (see memkind_arena(3))
 */
int memkind_pmem_create(struct memkind *kind, struct memkind_ops *ops, const char *name);

/*
 * memkind_pmem_destroy() is an implementation of the memkind "destroy" operation
 * for file-backed memory kinds.  This releases all of the resources allocated
 * by memkind_pmem_create() and allows the file system space to be reclaimed.
 */
int memkind_pmem_destroy(struct memkind *kind);

/*
 * memkind_pmem_mmap() allocates the file system space for a block of size bytes
 * in the memory-mapped file associated with given kind. The addr hint is ignored.
 * The return value is the address of mapped memory region or MAP_FAILED in the
 * case of an error.
 */
void *memkind_pmem_mmap(struct memkind *kind, void *addr, size_t size);

/*
 * memkind_pmem_get_mmap_flags() sets flags to MAP_SHARED. See mmap(2) for more
 * information about these flags.
 */
int memkind_pmem_get_mmap_flags(struct memkind *kind, int *flags);

struct memkind_pmem {
    int fd;
    void *addr;
    off_t offset;
    size_t max_size;
    pthread_mutex_t pmem_lock;
};

extern struct memkind_ops MEMKIND_PMEM_OPS;

#ifdef __cplusplus
}
#endif
