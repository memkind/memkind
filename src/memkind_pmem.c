/*
 * Copyright (C) 2015 - 2016 Intel Corporation.
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
#include <memkind/internal/memkind_private.h>

MEMKIND_EXPORT const struct memkind_ops MEMKIND_PMEM_OPS = {
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
};

MEMKIND_EXPORT int memkind_pmem_create(struct memkind *kind, const struct memkind_ops *ops,
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

    /* mark this kind/partition as file-mapped */
    kind->partition |= JEMALLOC_MEMKIND_FILE_MAPPED;

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

MEMKIND_EXPORT int memkind_pmem_destroy(struct memkind *kind)
{
    struct memkind_pmem *priv = kind->priv;

    memkind_arena_destroy(kind);

    pthread_mutex_destroy(&priv->pmem_lock);
    (void) munmap(priv->addr, priv->max_size);
    (void) close(priv->fd);
    jemk_free(priv);

    return 0;
}

MEMKIND_EXPORT void *memkind_pmem_mmap(struct memkind *kind, void *addr, size_t size)
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

MEMKIND_EXPORT int memkind_pmem_get_mmap_flags(struct memkind *kind, int *flags)
{
    *flags = MAP_SHARED;
    return 0;
}

MEMKIND_EXPORT int memkind_pmem_get_size(struct memkind *kind, size_t *total, size_t *free)
{
    struct memkind_pmem *priv = kind->priv;

    *total = priv->max_size;
    *free = priv->max_size - priv->offset; /* rough estimation */

    return 0;
}
