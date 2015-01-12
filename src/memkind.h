/*
 * Copyright (C) 2014, 2015 Intel Corporation.
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

#ifndef memkind_include_h
#define memkind_include_h
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/types.h>

enum memkind_const {
    MEMKIND_MAX_KIND = 512,
    MEMKIND_ERROR_MESSAGE_SIZE = 128,
    MEMKIND_NAME_LENGTH = 64
};

enum memkind_error {
    MEMKIND_ERROR_UNAVAILABLE = -1,
    MEMKIND_ERROR_MBIND = -2,
    MEMKIND_ERROR_MMAP  = -3,
    MEMKIND_ERROR_MEMALIGN = -4,
    MEMKIND_ERROR_MALLCTL = -5,
    MEMKIND_ERROR_MALLOC = -6,
    MEMKIND_ERROR_GETCPU = -7,
    MEMKIND_ERROR_PMTT = -8,
    MEMKIND_ERROR_TIEDISTANCE = -9,
    MEMKIND_ERROR_ALIGNMENT = -10,
    MEMKIND_ERROR_MALLOCX = -11,
    MEMKIND_ERROR_ENVIRON = -12,
    MEMKIND_ERROR_INVALID = -13,
    MEMKIND_ERROR_REPNAME = -14,
    MEMKIND_ERROR_TOOMANY = -15,
    MEMKIND_ERROR_PTHREAD = -16,
    MEMKIND_ERROR_BADOPS = -17,
    MEMKIND_ERROR_RUNTIME = -255
};

enum memkind_base_partition {
    MEMKIND_PARTITION_DEFAULT = 0,
    MEMKIND_PARTITION_HBW = 1,
    MEMKIND_PARTITION_HBW_HUGETLB = 2,
    MEMKIND_PARTITION_HBW_PREFERRED = 3,
    MEMKIND_PARTITION_HBW_PREFERRED_HUGETLB = 4,
    MEMKIND_PARTITION_HUGETLB = 5,
    MEMKIND_PARTITION_HBW_GBTLB = 6,
    MEMKIND_PARTITION_HBW_PREFERRED_GBTLB = 7,
    MEMKIND_PARTITION_GBTLB = 8,
    MEMKIND_PARTITION_HBW_INTERLEAVE = 9,
    MEMKIND_NUM_BASE_KIND
};

struct memkind_ops;

struct memkind {
    const struct memkind_ops *ops;
    int partition;
    char name[MEMKIND_NAME_LENGTH];
    pthread_once_t init_once;
    int arena_map_len;
    unsigned int *arena_map;
#ifndef MEMKIND_TLS
    pthread_key_t arena_key;
#endif
    void *priv;
};

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
    int (* get_arena)(struct memkind *kind, unsigned int *arena);
    int (* get_size)(struct memkind *kind, size_t *total, size_t *free);
    int (* check_available)(struct memkind *kind);
    int (* check_addr)(struct memkind *kind, void *addr);
    void (*init_once)(void);
};

typedef struct memkind * memkind_t;

extern memkind_t MEMKIND_DEFAULT;
extern memkind_t MEMKIND_HUGETLB;
extern memkind_t MEMKIND_HBW;
extern memkind_t MEMKIND_HBW_PREFERRED;
extern memkind_t MEMKIND_HBW_HUGETLB;
extern memkind_t MEMKIND_HBW_PREFERRED_HUGETLB;
extern memkind_t MEMKIND_HBW_GBTLB;
extern memkind_t MEMKIND_HBW_PREFERRED_GBTLB;
extern memkind_t MEMKIND_GBTLB;
extern memkind_t MEMKIND_HBW_INTERLEAVE;


/* Convert error number into an error message */
void memkind_error_message(int err, char *msg, size_t size);

/* Free all resources allocated by the library (must be last call to library by the process) */
int memkind_finalize(void);

/* KIND MANAGEMENT INTERFACE */

/* Create a new kind */
int memkind_create(const struct memkind_ops *ops, const char *name, memkind_t *kind);

/* Create a new PMEM (file-backed) kind of given size on top of a temporary file */
int memkind_create_pmem(const char *dir, size_t max_size, memkind_t *kind);

/* Query the number of kinds instantiated */
int memkind_get_num_kind(int *num_kind);

/* Get kind associated with a partition (index from 0 to num_kind - 1) */
int memkind_get_kind_by_partition(int partition, memkind_t *kind);

/* Get kind given the name of the kind */
int memkind_get_kind_by_name(const char *name, memkind_t *kind);

/* Get the amount in bytes of total and free memory of the NUMA nodes associated with the kind */
int memkind_get_size(memkind_t kind, size_t *total, size_t *free);

/* returns 0 if memory kind is available else returns error code */
int memkind_check_available(memkind_t kind);

/* HEAP MANAGEMENT INTERFACE */

/* malloc from the numa nodes of the specified kind */
void *memkind_malloc(memkind_t kind, size_t size);

/* calloc from the numa nodes of the specified kind */
void *memkind_calloc(memkind_t kind, size_t num, size_t size);

/* posix_memalign from the numa nodes of the specified kind */
int memkind_posix_memalign(memkind_t kind, void **memptr, size_t alignment,
                           size_t size);

/* realloc from the numa nodes of the specified kind */
void *memkind_realloc(memkind_t kind, void *ptr, size_t size);

/* Free memory allocated with the memkind API */
void memkind_free(memkind_t kind, void *ptr);

/* ALLOCATOR CALLBACK FUNCTION */
void *memkind_partition_mmap(int partition, void *addr, size_t size);

#ifdef __cplusplus
}
#endif
#endif
