/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
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

#if __STDC_VERSION__ < 199901L
#error "<numakind> requires ISO C99 or greater"
#endif

static const int NUMAKIND_MAX_KIND = 512;
static const int NUMAKIND_NUM_BASE_KIND = 5;
static const size_t NUMAKIND_ERROR_MESSAGE_SIZE = 128;

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
    NUMAKIND_ERROR_ALLOCM = -10,
    NUMAKIND_ERROR_ENVIRON = -11,
    NUMAKIND_ERROR_RUNTIME = -255
};

enum numakind_base_partition {
    NUMAKIND_PARTITION_DEFAULT = 0,
    NUMAKIND_PARTITION_HBW = 1,
    NUMAKIND_PARTITION_HBW_HUGETLB = 2,
    NUMAKIND_PARTITION_HBW_PREFERRED = 3,
    NUMAKIND_PARTITION_HBW_PREFERRED_HUGETLB = 4
};

enum numakind_arena_mode {
    NUMAKIND_ARENA_MODE_NONE,
    NUMAKIND_ARENA_MODE_CPU,
    NUMAKIND_ARENA_MODE_BIJECTIVE
};

struct numakind_ops;

struct numakind {
    struct numakind_ops *ops;
    int partition;
    char *name;
    int arena_mode;
    int arena_map_len;
    unsigned int *arena_map;
};

struct numakind_ops {
    int (* create)(struct numakind *kind, struct numakind_ops *ops, char *name, int arena_mode);
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
};

typedef struct numakind * numakind_t;

/* Convert error number into an error message */
void numakind_error_message(int err, char *msg, size_t size);

/* Query the number of kinds availble */
int numakind_get_num_kind(int *num_kind);

/* Get kind associated with a partition (index from 0 to num_kind - 1) */
int numakind_get_kind_by_partition(int partition, numakind_t *kind);

/* Get kind given the name of the kind */
int numakind_get_kind_by_name(const char *name, numakind_t *kind);

/* Create a new kind */
int numakind_create(const struct numakind_ops *ops, const char *name, int arena_mode);

/* returns 1 if numa kind is availble else 0 */
int numakind_is_available(numakind_t kind);

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
