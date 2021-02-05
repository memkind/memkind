// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */

#include <memkind/internal/memkind_arena.h>
#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_pmem.h>
#include <memkind/internal/memkind_private.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_SYNC
#define MAP_SYNC 0x80000
#endif

#ifndef MAP_SHARED_VALIDATE
#define MAP_SHARED_VALIDATE 0x03
#endif

MEMKIND_EXPORT struct memkind_ops MEMKIND_PMEM_OPS = {
    .create = memkind_pmem_create,
    .destroy = memkind_pmem_destroy,
    .malloc = memkind_arena_malloc,
    .calloc = memkind_arena_calloc,
    .posix_memalign = memkind_arena_posix_memalign,
    .realloc = memkind_arena_realloc,
    .free = memkind_arena_free,
    .mmap = memkind_pmem_mmap,
    .get_mmap_flags = memkind_pmem_get_mmap_flags,
    .get_arena = memkind_thread_get_arena,
    .malloc_usable_size = memkind_default_malloc_usable_size,
    .finalize = memkind_pmem_destroy,
    .update_memory_usage_policy = memkind_arena_update_memory_usage_policy,
    .get_stat = memkind_arena_get_kind_stat,
    .defrag_reallocate = memkind_arena_defrag_reallocate};

void *pmem_extent_alloc(extent_hooks_t *extent_hooks, void *new_addr,
                        size_t size, size_t alignment, bool *zero, bool *commit,
                        unsigned arena_ind)
{
    int err;
    void *addr = NULL;

    if (new_addr != NULL) {
        /* not supported */
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

    addr = memkind_pmem_mmap(kind, new_addr, size);

    if (addr != MAP_FAILED) {
        *zero = true;
        *commit = true;

        /* XXX - check alignment */
    } else {
        addr = NULL;
    }
exit:
    return addr;
}

bool pmem_extent_dalloc(extent_hooks_t *extent_hooks, void *addr, size_t size,
                        bool committed, unsigned arena_ind)
{
    bool result = true;
    if (memkind_get_hog_memory()) {
        return result;
    }

    // if madvise fail, it means that addr isn't mapped shared (doesn't come
    // from pmem) and it should be also unmapped to avoid space exhaustion when
    // calling large number of operations like memkind_create_pmem and
    // memkind_destroy_kind
    errno = 0;
    int status = madvise(addr, size, MADV_REMOVE);
    if (!status) {
        struct memkind *kind = get_kind_by_arena(arena_ind);
        struct memkind_pmem *priv = kind->priv;
        assert(priv->current_size >= size);
        if (pthread_mutex_lock(&priv->pmem_lock) != 0)
            assert(0 && "failed to acquire mutex");
        priv->current_size -= size;
        if (pthread_mutex_unlock(&priv->pmem_lock) != 0)
            assert(0 && "failed to release mutex");
    } else {
        if (errno == EOPNOTSUPP) {
            log_fatal("Filesystem doesn't support FALLOC_FL_PUNCH_HOLE.");
            abort();
        }
    }
    if (munmap(addr, size) == -1) {
        log_err("munmap failed!");
    } else {
        result = false;
    }
    return result;
}

bool pmem_extent_commit(extent_hooks_t *extent_hooks, void *addr, size_t size,
                        size_t offset, size_t length, unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

bool pmem_extent_decommit(extent_hooks_t *extent_hooks, void *addr, size_t size,
                          size_t offset, size_t length, unsigned arena_ind)
{
    /* do nothing - report failure (opt-out) */
    return true;
}

bool pmem_extent_purge(extent_hooks_t *extent_hooks, void *addr, size_t size,
                       size_t offset, size_t length, unsigned arena_ind)
{
    /* do nothing - report failure (opt-out) */
    return true;
}

bool pmem_extent_split(extent_hooks_t *extent_hooks, void *addr, size_t size,
                       size_t size_a, size_t size_b, bool committed,
                       unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

bool pmem_extent_merge(extent_hooks_t *extent_hooks, void *addr_a,
                       size_t size_a, void *addr_b, size_t size_b,
                       bool committed, unsigned arena_ind)
{
    /* do nothing - report success */
    return false;
}

void pmem_extent_destroy(extent_hooks_t *extent_hooks, void *addr, size_t size,
                         bool committed, unsigned arena_ind)
{
    if (munmap(addr, size) == -1) {
        log_err("munmap failed!");
    }
}

static extent_hooks_t pmem_extent_hooks = {.alloc = pmem_extent_alloc,
                                           .dalloc = pmem_extent_dalloc,
                                           .commit = pmem_extent_commit,
                                           .decommit = pmem_extent_decommit,
                                           .purge_lazy = pmem_extent_purge,
                                           .split = pmem_extent_split,
                                           .merge = pmem_extent_merge,
                                           .destroy = pmem_extent_destroy};

int memkind_pmem_create_tmpfile(const char *dir, int *fd)
{
    static char template[] = "/memkind.XXXXXX";
    int err = MEMKIND_SUCCESS;
    int oerrno;
    int dir_len = strlen(dir);

    if (dir_len > PATH_MAX) {
        log_err("Could not create temporary file: too long path.");
        return MEMKIND_ERROR_INVALID;
    }

    char fullname[dir_len + sizeof(template)];
    (void)strcpy(fullname, dir);
    (void)strcat(fullname, template);

    sigset_t set, oldset;
    sigfillset(&set);
    (void)sigprocmask(SIG_BLOCK, &set, &oldset);

    if ((*fd = mkstemp(fullname)) < 0) {
        log_err("Could not create temporary file: errno=%d.", errno);
        err = MEMKIND_ERROR_INVALID;
        goto exit;
    }

    (void)unlink(fullname);
    (void)sigprocmask(SIG_SETMASK, &oldset, NULL);

    return err;

exit:
    oerrno = errno;
    (void)sigprocmask(SIG_SETMASK, &oldset, NULL);
    if (*fd != -1) {
        (void)close(*fd);
    }
    *fd = -1;
    errno = oerrno;
    return err;
}

MEMKIND_EXPORT int memkind_pmem_create(struct memkind *kind,
                                       struct memkind_ops *ops,
                                       const char *name)
{
    struct memkind_pmem *priv;
    int err;

    priv = (struct memkind_pmem *)jemk_malloc(sizeof(struct memkind_pmem));
    if (!priv) {
        log_err("malloc() failed.");
        return MEMKIND_ERROR_MALLOC;
    }

    if (pthread_mutex_init(&priv->pmem_lock, NULL) != 0) {
        err = MEMKIND_ERROR_RUNTIME;
        goto exit;
    }

    err = memkind_default_create(kind, ops, name);
    if (err) {
        goto exit;
    }

    err = memkind_arena_create_map(kind, &pmem_extent_hooks);
    if (err) {
        goto exit;
    }

    kind->priv = priv;
    return 0;

exit:
    /* err is set, please don't overwrite it with result of
     * pthread_mutex_destroy */
    pthread_mutex_destroy(&priv->pmem_lock);
    jemk_free(priv);
    return err;
}

MEMKIND_EXPORT int memkind_pmem_destroy(struct memkind *kind)
{
    struct memkind_pmem *priv = kind->priv;

    memkind_arena_destroy(kind);

    pthread_mutex_destroy(&priv->pmem_lock);

    (void)close(priv->fd);
    jemk_free(priv->dir);
    jemk_free(priv);

    return 0;
}

static int memkind_pmem_recreate_file(struct memkind_pmem *priv, size_t size)
{
    int status = -1;
    int fd = -1;
    int err = memkind_pmem_create_tmpfile(priv->dir, &fd);
    if (err) {
        goto exit;
    }
    if ((errno = posix_fallocate(fd, 0, (off_t)size)) != 0) {
        goto exit;
    }
    close(priv->fd);
    priv->fd = fd;
    priv->offset = 0;
    status = 0;
exit:
    return status;
}

MEMKIND_EXPORT void *memkind_pmem_mmap(struct memkind *kind, void *addr,
                                       size_t size)
{
    struct memkind_pmem *priv = kind->priv;
    void *result;

    if (pthread_mutex_lock(&priv->pmem_lock) != 0)
        assert(0 && "failed to acquire mutex");

    if (priv->max_size != 0 && priv->current_size + size > priv->max_size) {
        if (pthread_mutex_unlock(&priv->pmem_lock) != 0)
            assert(0 && "failed to release mutex");
        return MAP_FAILED;
    }

    if ((errno = posix_fallocate(priv->fd, priv->offset, (off_t)size)) != 0) {
        if (errno == EFBIG) {
            if (memkind_pmem_recreate_file(priv, size) != 0) {
                if (pthread_mutex_unlock(&priv->pmem_lock) != 0)
                    assert(0 && "failed to release mutex");
                return MAP_FAILED;
            }
        } else {
            if (pthread_mutex_unlock(&priv->pmem_lock) != 0)
                assert(0 && "failed to release mutex");
            return MAP_FAILED;
        }
    }

    if ((result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, priv->fd,
                       priv->offset)) != MAP_FAILED) {
        priv->offset += size;
        priv->current_size += size;
    }

    if (pthread_mutex_unlock(&priv->pmem_lock) != 0)
        assert(0 && "failed to release mutex");

    return result;
}

MEMKIND_EXPORT int memkind_pmem_get_mmap_flags(struct memkind *kind, int *flags)
{
    *flags = MAP_SHARED;
    return 0;
}

int memkind_pmem_validate_dir(const char *dir)
{
    int ret = MEMKIND_SUCCESS;
    int fd = -1;
    const size_t size = sysconf(_SC_PAGESIZE);

    int err = memkind_pmem_create_tmpfile(dir, &fd);

    if (err) {
        return MEMKIND_ERROR_RUNTIME;
    }

    if (ftruncate(fd, size) != 0) {
        ret = MEMKIND_ERROR_RUNTIME;
        goto end;
    }

    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_SHARED_VALIDATE | MAP_SYNC, fd, 0);
    if (addr == MAP_FAILED) {
        ret = MEMKIND_ERROR_MMAP;
        goto end;
    }
    munmap(addr, size);

end:
    (void)close(fd);
    return ret;
}
