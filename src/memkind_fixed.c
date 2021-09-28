// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2021 Intel Corporation. */

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_fixed.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_private.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

extern void memtier_reset_size(unsigned id);

MEMKIND_EXPORT struct memkind_ops MEMKIND_FIXED_OPS = {
    .create = memkind_fixed_create,
    .destroy = memkind_fixed_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .mmap = memkind_fixed_mmap,
    .get_mmap_flags = NULL,
    .get_arena = memkind_thread_get_arena,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_fixed_destroy,
    .update_memory_usage_policy = memkind_arena_update_memory_usage_policy,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate,
};

/// @pre @p size is not equal zero
static void *memkind_fixed_mmap_aligned(struct memkind *kind, size_t alignment,
                                        size_t size)
{
    assert(size != 0 && "memkind_fixed_mmap_aligned does not accept size == 0");
    struct memkind_fixed *priv = kind->priv;
    void *result;

    if (pthread_mutex_lock(&priv->lock) != 0)
        assert(0 && "failed to acquire mutex");

    // calculate alignment offset
    size_t align_offset = 0u;
    size_t alignment_modulo = ((uintptr_t)priv->addr) % alignment;
    if (alignment_modulo != 0) {
        align_offset = alignment - alignment_modulo;
    }

    if (priv->current + size + align_offset > priv->size) {
        if (pthread_mutex_unlock(&priv->lock) != 0)
            assert(0 && "failed to release mutex");
        return MAP_FAILED;
    }

    priv->addr += align_offset;
    result = priv->addr;
    priv->addr += size;
    priv->current += size + align_offset;

    if (pthread_mutex_unlock(&priv->lock) != 0)
        assert(0 && "failed to release mutex");

    return result;
}

static void *fixed_extent_alloc(extent_hooks_t *extent_hooks, void *new_addr,
                                size_t size, size_t alignment, bool *zero,
                                bool *commit, unsigned arena_ind)
{
    int err;
    void *addr = NULL;

    if (new_addr != NULL) {
        /* not supported */
        log_err("fixed_extent_alloc only supports new_addr == NULL");
        goto exit;
    }

    struct memkind *kind = get_kind_by_arena(arena_ind);
    if (kind == NULL) {
        return NULL;
    }

    err = memkind_check_available(kind);
    if (err) {
        goto exit;
    }

    addr = memkind_fixed_mmap_aligned(kind, alignment, size);

    if (addr != MAP_FAILED) {
        *zero = true;
        *commit = true;
    } else {
        addr = NULL;
    }
exit:
    return addr;
}

static bool fixed_extent_dalloc(extent_hooks_t *extent_hooks, void *addr,
                                size_t size, bool committed, unsigned arena_ind)
{
    return true;
}

static bool fixed_extent_commit(extent_hooks_t *extent_hooks, void *addr,
                                size_t size, size_t offset, size_t length,
                                unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

static bool fixed_extent_decommit(extent_hooks_t *extent_hooks, void *addr,
                                  size_t size, size_t offset, size_t length,
                                  unsigned arena_ind)
{
    /* do nothing - report failure (opt-out) */
    return true;
}

static bool fixed_extent_purge(extent_hooks_t *extent_hooks, void *addr,
                               size_t size, size_t offset, size_t length,
                               unsigned arena_ind)
{
    /* do nothing - report failure (opt-out) */
    return true;
}

static bool fixed_extent_split(extent_hooks_t *extent_hooks, void *addr,
                               size_t size, size_t size_a, size_t size_b,
                               bool committed, unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

static bool fixed_extent_merge(extent_hooks_t *extent_hooks, void *addr_a,
                               size_t size_a, void *addr_b, size_t size_b,
                               bool committed, unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

static void fixed_extent_destroy(extent_hooks_t *extent_hooks, void *addr,
                                 size_t size, bool committed,
                                 unsigned arena_ind)
{
    if (munmap(addr, size) == -1) {
        log_err("munmap failed!");
    }
}

// clang-format off
static extent_hooks_t fixed_extent_hooks = {
    .alloc = fixed_extent_alloc,
    .dalloc = fixed_extent_dalloc,
    .commit = fixed_extent_commit,
    .decommit = fixed_extent_decommit,
    .purge_lazy = fixed_extent_purge,
    .split = fixed_extent_split,
    .merge = fixed_extent_merge,
    .destroy = fixed_extent_destroy
};
// clang-format on

MEMKIND_EXPORT int memkind_fixed_create(struct memkind *kind,
                                        struct memkind_ops *ops,
                                        const char *name)
{
    struct memkind_fixed *priv;
    int err;

    priv = (struct memkind_fixed *)jemk_malloc(sizeof(struct memkind_fixed));
    if (!priv) {
        log_err("malloc() failed.");
        return MEMKIND_ERROR_MALLOC;
    }

    if (pthread_mutex_init(&priv->lock, NULL) != 0) {
        err = MEMKIND_ERROR_RUNTIME;
        goto exit;
    }

    err = memkind_default_create(kind, ops, name);
    if (err) {
        goto exit;
    }

    err = memkind_arena_create_map(kind, &fixed_extent_hooks, false);
    if (err) {
        goto exit;
    }

    kind->priv = priv;
    return 0;

exit:
    /* err is set, please don't overwrite it with result of
     * pthread_mutex_destroy */
    pthread_mutex_destroy(&priv->lock);
    jemk_free(priv);
    return err;
}

MEMKIND_EXPORT int memkind_fixed_destroy(struct memkind *kind)
{
    struct memkind_fixed *priv = kind->priv;

    memkind_arena_destroy(kind);
    memtier_reset_size(kind->partition);
    pthread_mutex_destroy(&priv->lock);

    jemk_free(priv);

    return 0;
}

MEMKIND_EXPORT void *memkind_fixed_mmap(struct memkind *kind, void *addr,
                                        size_t size)
{
    struct memkind_fixed *priv = kind->priv;
    void *result;

    if (pthread_mutex_lock(&priv->lock) != 0)
        assert(0 && "failed to acquire mutex");

    if (priv->size != 0 && priv->current + size > priv->size) {
        if (pthread_mutex_unlock(&priv->lock) != 0)
            assert(0 && "failed to release mutex");
        return MAP_FAILED;
    }

    result = priv->addr;
    priv->addr += size;
    priv->current += size;

    if (pthread_mutex_unlock(&priv->lock) != 0)
        assert(0 && "failed to release mutex");

    return result;
}
