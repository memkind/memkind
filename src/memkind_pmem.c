/*
 * Copyright (C) 2015 Intel Corporation.
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

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <jemalloc/jemalloc.h>

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_pmem.h>


void *pmem_chunk_alloc(void *chunk, size_t size, size_t alignment,
                       bool *zero, bool *commit, unsigned arena_ind)
{
    int err;
    void *addr = NULL;

    if (chunk != NULL) {
        /* not supported */
        goto exit;
    }

    struct memkind *kind;
    err = memkind_get_kind_by_arena(arena_ind, &kind);
    if (err) {
        goto exit;
    }

    err = memkind_check_available(kind);
    if (err) {
        goto exit;
    }

    addr = memkind_pmem_mmap(kind, chunk, size);

    if (addr != MAP_FAILED) {
        *zero = true;
        *commit = true;

        /* XXX - check alignment */
    } else {
        addr = NULL;
    }

exit:
    //printf("%s(chunk=%p size=%zu align=%zu zero=%d commit=%d arena=%u) = %p\n",
    //   __func__, chunk, size, alignment, *zero, *commit, arena_ind, addr);
    return addr;
}

bool pmem_chunk_dalloc(void *chunk, size_t size, bool commited,
                        unsigned arena_ind)
{
    /* do nothing - report failure (opt-out) */
    return true;
}

bool pmem_chunk_commit(void *chunk, size_t size, size_t offset, size_t length,
                        unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

bool pmem_chunk_decommit(void *chunk, size_t size, size_t offset, size_t length,
                          unsigned arena_ind)
{
    /* do nothing - report failure (opt-out) */
    return true;
}

bool pmem_chunk_purge(void *chunk, size_t size, size_t offset, size_t length,
                       unsigned arena_ind)
{
    /* do nothing - report failure (opt-out) */
    return true;
}

bool pmem_chunk_split(void *chunk, size_t size, size_t size_a, size_t size_b,
                       bool commited, unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

bool pmem_chunk_merge(void *chunk_a, size_t size_a, void *chunk_b,
                       size_t size_b, bool commited, unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

static chunk_hooks_t pmem_chunk_hooks = {
    pmem_chunk_alloc,
    pmem_chunk_dalloc,
    pmem_chunk_commit,
    pmem_chunk_decommit,
    pmem_chunk_purge,
    pmem_chunk_split,
    pmem_chunk_merge
};


const struct memkind_ops MEMKIND_PMEM_OPS = {
    .create = memkind_pmem_create,
    .destroy = memkind_pmem_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_default_free,
    .mmap = memkind_pmem_mmap,
    .get_mmap_flags = memkind_pmem_get_mmap_flags,
    .get_arena = memkind_thread_get_arena,
    .get_size = memkind_pmem_get_size,
    .chunk_hooks = &pmem_chunk_hooks
};


int memkind_pmem_create(struct memkind *kind, const struct memkind_ops *ops,
                        const char *name)
{
    struct memkind_pmem *priv;
    int err;

    priv = (struct memkind_pmem *)jemk_malloc(sizeof(struct memkind_pmem));
    if (!priv) {
        return MEMKIND_ERROR_MALLOC;
    }

    if (pthread_mutex_init(&priv->pmem_lock, NULL) != 0) {
        err = MEMKIND_ERROR_PTHREAD;
        goto exit;
    }

    err = memkind_arena_create(kind, ops, name);
    if (err) {
        goto exit;
    }

    kind->priv = priv;
    return 0;

exit:
    if (priv) {
        pthread_mutex_destroy(&priv->pmem_lock);
        jemk_free(priv);
    }
    return err;
}

int memkind_pmem_destroy(struct memkind *kind)
{
    struct memkind_pmem *priv = kind->priv;

    memkind_arena_destroy(kind);

    pthread_mutex_destroy(&priv->pmem_lock);
    (void) munmap(priv->addr, priv->max_size);
    (void) close(priv->fd);
    jemk_free(priv);

    return 0;
}

void *memkind_pmem_mmap(struct memkind *kind, void *addr, size_t size)
{
    struct memkind_pmem *priv = kind->priv;
    void *result;

    if (pthread_mutex_lock(&priv->pmem_lock)) {
        return MAP_FAILED;
    }

    if (priv->offset + size > priv->max_size) {
        pthread_mutex_unlock(&priv->pmem_lock);
        return MAP_FAILED;
    }

    if ((errno = posix_fallocate(priv->fd, priv->offset, size)) != 0) {
        pthread_mutex_unlock(&priv->pmem_lock);
        return MAP_FAILED;
    }

    result = priv->addr + priv->offset;
    priv->offset += size;

    pthread_mutex_unlock(&priv->pmem_lock);

    return result;
}

int memkind_pmem_get_mmap_flags(struct memkind *kind, int *flags)
{
    *flags = MAP_SHARED;
    return 0;
}

int memkind_pmem_get_size(struct memkind *kind, size_t *total, size_t *free)
{
    struct memkind_pmem *priv = kind->priv;

    *total = priv->max_size;
    *free = priv->max_size - priv->offset; /* rough estimation */

    return 0;
}
