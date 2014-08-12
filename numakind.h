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

#ifndef numakind_include_h
#define numakind_include_h
#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  \mainpage
 *
 *  The numakind library extends libnuma with the ability to
 *  categorize groups of numa nodes into different "kinds" of
 *  memory. It provides a low level interface for generating inputs to
 *  mbind() and mmap(), and a high level interface for heap
 *  management.  The heap management is implemented with an extension
 *  to the jemalloc library which dedicates "arenas" to each CPU and
 *  kind of memory.  To use numakind, jemalloc must be compiled with
 *  the --enable-numakind option.
 */

enum numakind_const {
    NUMAKIND_MAX_KIND = 512,
    NUMAKIND_NUM_BASE_KIND = 6,
    NUMAKIND_ERROR_MESSAGE_SIZE = 128,
    NUMAKIND_NAME_LENGTH = 64
};

enum numakind_error {
    NUMAKIND_ERROR_UNAVAILABLE = -1,
    NUMAKIND_ERROR_MBIND = -2,
    NUMAKIND_ERROR_MEMALIGN = -3,
    NUMAKIND_ERROR_MALLCTL = -4,
    NUMAKIND_ERROR_MALLOC = -5,
    NUMAKIND_ERROR_GETCPU = -6,
    NUMAKIND_ERROR_PMTT = -7,
    NUMAKIND_ERROR_TIEDISTANCE = -8,
    NUMAKIND_ERROR_ALIGNMENT = -9,
    NUMAKIND_ERROR_MALLOCX = -10,
    NUMAKIND_ERROR_ENVIRON = -11,
    NUMAKIND_ERROR_INVALID = -12,
    NUMAKIND_ERROR_REPNAME = -13,
    NUMAKIND_ERROR_TOOMANY = -14,
    NUMAKIND_ERROR_PTHREAD = -15,
    NUMAKIND_ERROR_RUNTIME = -255
};

enum numakind_base_partition {
    NUMAKIND_PARTITION_DEFAULT = 0,
    NUMAKIND_PARTITION_HUGETLB = 1,
    NUMAKIND_PARTITION_HBW = 2,
    NUMAKIND_PARTITION_HBW_HUGETLB = 3,
    NUMAKIND_PARTITION_HBW_PREFERRED = 4,
    NUMAKIND_PARTITION_HBW_PREFERRED_HUGETLB = 5
};

struct numakind_ops;

struct numakind {
    const struct numakind_ops *ops;
    int partition;
    char name[NUMAKIND_NAME_LENGTH];
    pthread_once_t init_once;
    int arena_map_len;
    unsigned int *arena_map;
};

struct numakind_ops {
    int (* create)(struct numakind *kind, const struct numakind_ops *ops, const char *name);
    int (* destroy)(struct numakind *kind);
    void *(* malloc)(struct numakind *kind, size_t size);
    void *(* calloc)(struct numakind *kind, size_t num, size_t size);
    int (* posix_memalign)(struct numakind *kind, void **memptr, size_t alignment, size_t size);
    void *(* realloc)(struct numakind *kind, void *ptr, size_t size);
    void (* free)(struct numakind *kind, void *ptr);
    int (* is_available)(struct numakind *kind);
    int (* mbind)(struct numakind *kind, void *ptr, size_t len);
    int (* get_mmap_flags)(struct numakind *kind, int *flags);
    int (* get_mbind_mode)(struct numakind *kind, int *mode);
    int (* get_mbind_nodemask)(struct numakind *kind, unsigned long *nodemask, unsigned long maxnode);
    int (* get_arena) (struct numakind *kind, unsigned int *arena);
    int (* get_size) (struct numakind *kind, size_t *total, size_t *free);
    void (*init_once)(void);
};

typedef struct numakind * numakind_t;

extern numakind_t NUMAKIND_DEFAULT;
extern numakind_t NUMAKIND_HUGETLB;
extern numakind_t NUMAKIND_HBW;
extern numakind_t NUMAKIND_HBW_PREFERRED;
extern numakind_t NUMAKIND_HBW_HUGETLB;
extern numakind_t NUMAKIND_HBW_PREFERRED_HUGETLB;

/* Convert error number into an error message */
void numakind_error_message(int err, char *msg, size_t size);

/* Free all resources allocated by the library (must be last call to library by the process) */
int numakind_finalize(void);

/* KIND MANAGEMENT INTERFACE */

/* Create a new kind */
int numakind_create(const struct numakind_ops *ops, const char *name);

/* Query the number of kinds instantiated */
int numakind_get_num_kind(int *num_kind);

/* Get kind associated with a partition (index from 0 to num_kind - 1) */
int numakind_get_kind_by_partition(int partition, numakind_t *kind);

/* Get kind given the name of the kind */
int numakind_get_kind_by_name(const char *name, numakind_t *kind);

/* Get the amount in bytes of total and free memory of the NUMA nodes assciated with the kind */
int numakind_get_size(numakind_t kind, size_t *total, size_t *free);

/* returns 1 if numa kind is availble else 0 */
int numakind_is_available(numakind_t kind);

/* HEAP MANAGEMENT INTERFACE */

/* malloc from the numa nodes of the specified kind */
void *numakind_malloc(numakind_t kind, size_t size);

/* calloc from the numa nodes of the specified kind */
void *numakind_calloc(numakind_t kind, size_t num, size_t size);

/* posix_memalign from the numa nodes of the specified kind */
int numakind_posix_memalign(numakind_t kind, void **memptr, size_t alignment,
                            size_t size);

/* realloc from the numa nodes of the specified kind */
void *numakind_realloc(numakind_t kind, void *ptr, size_t size);

/* Free memory allocated with the numakind API */
void numakind_free(numakind_t kind, void *ptr);

/* ALLOCATOR CALLBACK FUNCTIONS */

/* returns 1 if numa kind associated with the partition is availble else 0 */
int numakind_partition_is_available(int partition);

/* get flags for call to mmap for the numa kind associated with the partition */
int numakind_partition_get_mmap_flags(int partition, int *flags);

/* mbind to the nearest numa node of the numa kind associated with the partition */
int numakind_partition_mbind(int partition, void *addr, size_t len);

#ifdef __cplusplus
}
#endif
#endif
