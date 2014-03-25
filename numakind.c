#include <stdlib.h>
#include <stdio.h>
#include <numa.h>
#include <numaif.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <jemalloc/jemalloc.h>

#include "numakind.h"
#include "numakind_mcdram.h"

static int numakind_getarena(numakind_t kind, int *arena);

void numakind_error_message(int err, char *msg, size_t size)
{
    switch (err) {
        case NUMAKIND_ERROR_UNAVAILABLE:
            strncpy(msg, "<numakind> Requested numa kind is not available", size);
            break;
        case NUMAKIND_ERROR_MBIND:
            strncpy(msg, "<numakind> Call to mbind() failed", size);
            break;
        case NUMAKIND_ERROR_MEMALIGN:
            strncpy(msg, "<numakind> Call to posix_memalign() failed", size);
            break;
        case NUMAKIND_ERROR_MALLCTL:
            strncpy(msg, "<numakind> Call to je_mallctl() failed", size);
            break;
        case NUMAKIND_ERROR_MALLOC:
            strncpy(msg, "<numakind> Call to je_malloc() failed", size);
            break;
        case NUMAKIND_ERROR_GETCPU:
            strncpy(msg, "<numakind> Call to sched_getcpu() returned out of range", size);
            break;
        case NUMAKIND_ERROR_MCDRAM:
            strncpy(msg, "<numakind> Initializing for MCDRAM failed", size);
            break;
        case NUMAKIND_ERROR_PMTT:
            snprintf(msg, size, "<numakind> Unable to parse bandwidth table: %s", NUMAKIND_BANDWIDTH_PATH);
            break;
        case NUMAKIND_ERROR_TIEDISTANCE:
            strncpy(msg, "<numakind> Two NUMA memory nodes are equidistant from target cpu node", size);
            break;
        case NUMAKIND_ERROR_ALIGNMENT:
            strncpy(msg, "<numakind> Alignment must be a power of two and larger than sizeof(void *)", size);
            break;
        case NUMAKIND_ERROR_ALLOCM:
            strncpy(msg, "<numakind> Call to je_allocm() failed", size);
            break;
        default:
            snprintf(msg, size, "<numakind> Undefined error number: %i", err);
            break;
    }
    if (size > 0)
        msg[size-1] = '\0';
}

int numakind_isavail(int kind)
{
    int result;
    switch (kind) {
        case NUMAKIND_MCDRAM:
        case NUMAKIND_MCDRAM_HUGETLB:
            result = numakind_mcdram_isavail();
            break;
        case NUMAKIND_DEFAULT:
            result = 1;
            break;
        default:
            result = 0;
            fprintf(stderr, "WARNING: Unknown numakind_t (%i)\n", (int)kind);
            break;
    }
    return result;
}

int numakind_mmap_flags(int kind, int *flags)
{
    int err = 0;
    switch (kind) {
        case NUMAKIND_MCDRAM_HUGETLB:
            *flags = MAP_HUGETLB;
        case NUMAKIND_MCDRAM:
        case NUMAKIND_DEFAULT:
            *flags = 0;
            break;
        default:
            *flags = 0;
            err = NUMAKIND_ERROR_UNAVAILABLE;
            break;
    }
    return err;
}

int numakind_nodemask(int kind, unsigned long *nodemask, unsigned long maxnode)
{
    struct bitmask nodemask_bm = {maxnode, nodemask};
    int err = 0;

    switch (kind) {
        case NUMAKIND_MCDRAM:
            err = numakind_mcdram_nodemask(nodemask, maxnode);
            break;
        case NUMAKIND_DEFAULT:            numa_bitmask_clearall(&nodemask_bm);
            numa_bitmask_setbit(&nodemask_bm, numa_preferred());
            break;
        default:
            numa_bitmask_clearall(&nodemask_bm);
            err = NUMAKIND_ERROR_UNAVAILABLE;
            break;
    }
    return err;
}

int numakind_mbind(int kind, void *addr, size_t size)
{
    nodemask_t nodemask;
    int err = 0;
    if (!numakind_isavail(kind)) {
        err = NUMAKIND_ERROR_UNAVAILABLE;
    }
    else {
        numakind_nodemask(kind, nodemask.n, NUMA_NUM_NODES);
        err = mbind(addr, size, MPOL_BIND, nodemask.n, NUMA_NUM_NODES, 0);
        err = err ? NUMAKIND_ERROR_MBIND : 0;
    }
    return err;
}

void *numakind_malloc(numakind_t kind, size_t size)
{
    void *result = NULL;
    int err = 0;
    int arena;

    if (kind == NUMAKIND_DEFAULT) {
        result = je_malloc(size);
    }
    else {
        err = numakind_getarena(kind, &arena);
        if (!err) {
            err = je_allocm(&result, NULL, size, ALLOCM_ARENA(arena));
        }
        if (err) {
            result = NULL;
        }
    }
    return result;
}

void *numakind_realloc(numakind_t kind, void *ptr, size_t size)
{
    int err = 0;
    int arena;

    if (kind == NUMAKIND_DEFAULT) {
        ptr = je_realloc(ptr, size);
    }
    else {
        err = numakind_getarena(kind, &arena);
        if (!err) {
            if (ptr == NULL) {
                err = je_allocm(&ptr, NULL, size, ALLOCM_ARENA(arena));
            }
            else {
                err = je_rallocm(&ptr, NULL, size, 0, ALLOCM_ARENA(arena));
            }
        }
        if (err) {
            ptr = NULL;
        }
    }
    return ptr;
}

void *numakind_calloc(numakind_t kind, size_t num, size_t size)
{
    void *result;
    result = numakind_malloc(kind, num * size);
    if (result) {
        memset(result, 0, num * size);
    }
    return result;
}

int numakind_posix_memalign(numakind_t kind, void **memptr, size_t alignment,
        size_t size)
{
    int err = 0;
    int arena;

    if (kind == NUMAKIND_DEFAULT) {
        err = je_posix_memalign(memptr, alignment, size);
        err = err ? NUMAKIND_ERROR_MEMALIGN : 0;
    }
    else {
         err = numakind_getarena(kind, &arena);
         if (!err) {
             if ( (alignment < sizeof(void*)) ||
                  (((alignment - 1) & alignment) != 0) ){
                  err = NUMAKIND_ERROR_ALIGNMENT;
             }
         }
         if (!err) {
              err = je_allocm(memptr, NULL, size,
                              ALLOCM_ALIGN(alignment) | ALLOCM_ARENA(arena));
              err = err ? NUMAKIND_ERROR_ALLOCM : 0;
         }
    }
    if (err) {
        *memptr = NULL;
    }
    return err;
}

void numakind_free(numakind_t kind, void *ptr)
{
    je_free(ptr);
}


static int numakind_getarena(numakind_t kind, int *arena)
{
    static int init_err = 0;
    static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_mutex_t destroy_mutex = PTHREAD_MUTEX_INITIALIZER;
    static int is_init = 0;
    static int num_cpu = 0;
    static unsigned int map_len = 0;
    static unsigned int *arena_map = NULL;
    static size_t unsigned_size = sizeof(unsigned int);
    int err = 0;
    int i, cpu_node, arena_index;
    unsigned int kind_select;

    if (!is_init && !init_err) {
        /* Initialize arena_map */
        pthread_mutex_lock(&init_mutex);
        if (!is_init && !init_err) {
            num_cpu = numa_num_configured_cpus();
            map_len = (NUMAKIND_NUM_KIND - 1) * num_cpu;
            arena_map = je_malloc(sizeof(unsigned int) * map_len);
            if (arena_map) {
                for (kind_select = 1;
                     kind_select < NUMAKIND_NUM_KIND && !init_err;
                     ++kind_select) {
                    for (i = 0; i < num_cpu && !init_err; ++i) {
                        arena_map[i] = kind_select;
                        init_err = je_mallctl("arenas.extendk", arena_map + i,
                                              &unsigned_size, NULL, 0);
                    }
                }
                init_err = init_err ? NUMAKIND_ERROR_MALLCTL : 0;
            }
            else {
                init_err = NUMAKIND_ERROR_MALLOC;
            }
            if (init_err) {
                is_init = 0;
                num_cpu = 0;
                map_len = 0;
                if (arena_map) {
                    je_free(arena_map);
                    arena_map = NULL;
                }
            }
            else {
                is_init = 1;
            }
            pthread_mutex_unlock(&init_mutex);
        }
        if (init_err)
            return init_err;
    }
    if (kind < 0 && !init_err) {
        /* Destroy arena_map */
        if (map_len) {
            pthread_mutex_lock(&destroy_mutex);
            if (map_len) {
                for (i = 0; i < map_len && !init_err; ++i) {
                    init_err = je_mallctl("arenas.purge", arena_map + i,
                                          &unsigned_size, NULL, 0);
                }
                je_free(arena_map);
                arena_map = NULL;
                map_len = 0;
                num_cpu = 0;
                is_init = 0;
                pthread_mutex_unlock(&destroy_mutex);
            }
        }
        init_err = init_err ? NUMAKIND_ERROR_MALLCTL : 0;
        return init_err;
    }
    else if (!init_err) {
        cpu_node = numa_node_of_cpu(sched_getcpu());
        if (cpu_node >= num_cpu) {
            err = NUMAKIND_ERROR_GETCPU;
        }
        else {
            arena_index = ((int)kind - 1) * num_cpu + cpu_node;
            *arena = arena_map[arena_index];
        }
    }
    else {
        err = init_err;
    }
    return err;
}
