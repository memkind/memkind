// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "../config.h"
#include <memkind/internal/memkind_memtier.h>

#include <tiering/ctl.h>
#include <tiering/memtier_log.h>

#include <pthread.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include <pthread.h>
#include <threads.h>
#include <execinfo.h>

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>
#define MEMKIND_ATOMIC _Atomic
#else
#define MEMKIND_ATOMIC
#endif


#define MEMTIER_EXPORT __attribute__((visibility("default")))
#define MEMTIER_INIT   __attribute__((constructor))
#define MEMTIER_FINI   __attribute__((destructor))

#define MEMTIER_LIKELY(x)   __builtin_expect(!!(x), 1)
#define MEMTIER_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define MT_SYMBOL2(a, b) a##b
#define MT_SYMBOL1(a, b) MT_SYMBOL2(a, b)
#define MT_SYMBOL(b)     MT_SYMBOL1(MEMTIER_ALLOC_PREFIX, b)

#define mt_malloc             MT_SYMBOL(malloc)
#define mt_calloc             MT_SYMBOL(calloc)
#define mt_realloc            MT_SYMBOL(realloc)
#define mt_free               MT_SYMBOL(free)
#define mt_posix_memalign     MT_SYMBOL(posix_memalign)
#define mt_malloc_usable_size MT_SYMBOL(malloc_usable_size)

#ifdef MEMKIND_DECORATION_ENABLED
#include <memkind/internal/memkind_private.h>
MEMTIER_EXPORT void memtier_kind_malloc_post(struct memkind *kind, size_t size,
                                             void **result)
{
    log_debug("kind: %s, malloc:(%zu) = %p", kind->name, size, *result);
}

MEMTIER_EXPORT void memtier_kind_calloc_post(memkind_t kind, size_t num,
                                             size_t size, void **result)
{
    log_debug("kind: %s, calloc:(%zu, %zu) = %p", kind->name, num, size,
              *result);
}

MEMTIER_EXPORT void memtier_kind_realloc_post(struct memkind *kind, void *ptr,
                                              size_t size, void **result)
{
    log_debug("kind: %s, realloc(%p, %zu) = %p", kind->name, ptr, size,
              *result);
}

MEMTIER_EXPORT void memtier_kind_posix_memalign_post(memkind_t kind,
                                                     void **memptr,
                                                     size_t alignment,
                                                     size_t size, int *err)
{
    log_debug("kind: %s, posix_memalign(%p, %zu, %zu) = %d", kind->name,
              *memptr, alignment, size, err);
}

MEMTIER_EXPORT void memtier_kind_free_pre(void **ptr)
{
    struct memkind *kind = memkind_detect_kind(*ptr);
    if (kind)
        log_debug("kind: %s, free(%p)", kind->name, *ptr);
    else
        log_debug("free(%p)", *ptr);
}

MEMTIER_EXPORT void memtier_kind_usable_size_post(void **ptr, size_t size)
{
    struct memkind *kind = memkind_detect_kind(*ptr);
    if (kind)
        log_debug("kind: %s, malloc_usable_size(%p) = %zu", kind->name, *ptr,
                  size);
    else
        log_debug("malloc_usable_size(%p) = %zu", *ptr, size);
}
#endif

static int destructed;

static struct memtier_memory *current_memory;

void *(*real_mmap)(void *, size_t, int, int, int, off_t);
int (*real_munmap)(void *, size_t);
static bool init = false;

// for initial mallocs
static thread_local unsigned char bytes[100000];
static thread_local size_t last_byte = 0;

static thread_local void* mmap_map[1000];
static thread_local int num_mmaps = 0;
extern thread_local bool dont_mmap;

void do_init()
{
    if (init) 
        return;
    init = true;

    real_mmap = (void * (*)(void *, size_t, int, int, int, off_t))dlsym(
        RTLD_NEXT , "mmap");
    real_munmap = (int (*)(void *, size_t))dlsym(RTLD_NEXT , "munmap");
    log_err("real_mmap %p", real_mmap);
    log_err("real_munmap %p", real_munmap);

    init = false;
}

MEMTIER_EXPORT void *malloc(size_t size)
{
    if (real_mmap == 0)
    {
        do_init();
        void* ret = (void*)&bytes[last_byte];
        last_byte += size;
        
        return ret;
    }

    if (MEMTIER_LIKELY(current_memory)) {
        return memtier_malloc(current_memory, size, 0);
    } else if (destructed == 0) {
        return memkind_malloc(MEMKIND_DEFAULT, size);
    }
    return NULL;
}

MEMTIER_EXPORT void *calloc(size_t num, size_t size)
{
    if (real_mmap == 0)
    {
        do_init();
        void* ret = (void*)&bytes[last_byte];
        last_byte += size;
        return ret;
    }

    if (MEMTIER_LIKELY(current_memory)) {
        return memtier_calloc(current_memory, num, size);
    } else if (destructed == 0) {
        return memkind_calloc(MEMKIND_DEFAULT, num, size);
    }
    return NULL;
}

MEMTIER_EXPORT void *realloc(void *ptr, size_t size)
{
    if ((ptr >= (void*)bytes) && ptr < (void*)(bytes + last_byte))
    {
        do_init();
        void* ret = (void*)&bytes[last_byte];
        last_byte += size;
        return ret;
    }

    if (MEMTIER_LIKELY(current_memory)) {
        return memtier_realloc(current_memory, ptr, size);
    } else if (destructed == 0) {
        return memkind_realloc(MEMKIND_DEFAULT, ptr, size);
    }
    return NULL;
}

// clang-format off
MEMTIER_EXPORT int posix_memalign(void **memptr, size_t alignment, size_t size)
{
    if (MEMTIER_LIKELY(current_memory)) {
        return memtier_posix_memalign(current_memory, memptr, alignment,
                                          size);
    } else if (destructed == 0) {
        return memkind_posix_memalign(MEMKIND_DEFAULT, memptr, alignment,
                                     size);
    }
    return 0;
}
// clang-format on

MEMTIER_EXPORT void free(void *ptr)
{
    if ((ptr >= (void*)bytes) && ptr < (void*)(bytes + last_byte)) {
        // ignore
        return;
    }

    if (MEMTIER_LIKELY(current_memory)) {
        memtier_realloc(current_memory, ptr, 0);
    } else if (destructed == 0) {
        memkind_free(MEMKIND_DEFAULT, ptr);
    }
}

MEMTIER_EXPORT size_t malloc_usable_size(void *ptr)
{
    return memtier_usable_size(ptr);
}

MEMTIER_EXPORT void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    if (!real_mmap) {
        do_init();
    }

    if ((current_memory == 0) || (
        (addr == NULL) &&
        (prot == (PROT_READ | PROT_WRITE)) && 
        (flags == (MAP_ANONYMOUS | MAP_PRIVATE)))) 
    {
        //log_err("mmap: start:%p, length:%lu, prot:%d, flags:%d, fd:%d, offset:%ld", 
        //    addr, length, prot, flags, fd, offset);

        mmap_map[num_mmaps] = addr;
        num_mmaps++;

        return memtier_mmap((void*)real_mmap, current_memory, addr, length, prot, flags, fd, offset);
    }

    return real_mmap(addr, length, prot, flags, fd, offset);
}

MEMTIER_EXPORT int munmap(void *addr, size_t length)
{
    if (!real_mmap) {
        do_init();
    }

    int i;
    for(i = 0; i < num_mmaps; i++)
    {
        if (mmap_map[i] == addr) {
            //log_err("munmap: start:%p, length:%lu", addr, length);
            memtier_munmap(addr);
            return 0;
        }
    }

    return real_munmap(addr, length);
}

static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static MEMTIER_INIT void memtier_init(void)
{

    pthread_once(&init_once, log_init_once);
    log_info("Memkind memtier lib loaded!");

    //orig_mmap = (void* (*)(void*, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap");

    char *env_var = utils_get_env("MEMKIND_MEM_TIERS");
    if (env_var) {
        current_memory = ctl_create_tier_memory_from_env(env_var);
        if (current_memory) {
            return;
        }
        log_err("Error with parsing MEMKIND_MEM_TIERS");
    } else {
        log_err("Missing MEMKIND_MEM_TIERS env var");
    }
    abort();
}

static MEMTIER_FINI void memtier_fini(void)
{
    log_info("Unloading memkind memtier lib!");

    ctl_destroy_tier_memory(current_memory);
    current_memory = NULL;

    destructed = 1;
}

MEMTIER_EXPORT void *mt_malloc(size_t size)
{
    return malloc(size);
}

MEMTIER_EXPORT void *mt_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

MEMTIER_EXPORT void *mt_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

MEMTIER_EXPORT void mt_free(void *ptr)
{
    free(ptr);
}

MEMTIER_EXPORT int mt_posix_memalign(void **memptr, size_t alignment,
                                     size_t size)
{
    return posix_memalign(memptr, alignment, size);
}

MEMTIER_EXPORT size_t mt_malloc_usable_size(void *ptr)
{
    return malloc_usable_size(ptr);
}
