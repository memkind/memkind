/*
 * In this example we try to create an optimal allocator for use on a
 * NUMA system with threading.  We assume that by the time a thread
 * makes an allocation call it is being run on the CPU that it will
 * remain on for the thread lifetime.  If this is not the case, and
 * the thread is migrated between allocation calls, there is no way 
 * to insure that the thread's data is local anyway.  
 *
 * We will create one heap partition for each NUMA node to which the
 * partition will be bound.  There will be one arena per CPU, and the
 * index for the arena will be kept in thread local storage.  
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <numa.h>
#include <numaif.h>
#include <jemalloc/jemalloc.h>
#include <memkind.h>
#include <memkind_default.h>
#include <memkind_arena.h>

pthread_key_t numakind_key_g;
pthread_once_t numakind_init_once_g = PTHREAD_ONCE_INIT;
int numakind_init_err_g = 0;
int numakind_zero_partition_g;

enum { NUMAKIND_MAX = 2048 };

#define NUMAKIND_GET_MBIND_NODEMASK_MACRO(NODE)                   \
int get_mbind_nodemask_numa_##NODE(struct memkind *kind,          \
    unsigned long *nodemask, unsigned long maxnode)               \
{                                                                 \
    unsigned int node_id = NODE;                                  \
    struct bitmask nodemask_bm = {maxnode, nodemask};             \
    numa_bitmask_clearall(&nodemask_bm);                          \
    numa_bitmask_setbit(&nodemask_bm, node_id);                   \
    return 0;                                                     \
}

#define NUMAKIND_OPS_MACRO(NODE)                                  \
{                                                                 \
    .create = memkind_arena_create,                               \
    .destroy = memkind_arena_destroy,                             \
    .malloc = memkind_arena_malloc,                               \
    .calloc = memkind_arena_calloc,                               \
    .posix_memalign = memkind_arena_posix_memalign,               \
    .realloc = memkind_arena_realloc,                             \
    .free = memkind_default_free,                                 \
    .get_arena = memkind_thread_get_arena,                        \
    .get_size = memkind_default_get_size,                         \
    .get_mbind_nodemask = get_mbind_nodemask_numa_##NODE          \
}

#include "numakind_helper.h"

void numakind_init(void)
{
    int err = 0;
    char numakind_name[64];
    int i, name_length, num_nodes;
    memkind_t numakind;

    pthread_key_create(&numakind_key_g, je_free);

    num_nodes = numa_num_configured_nodes();
    if (num_nodes > NUMAKIND_MAX) {
        err = MEMKIND_ERROR_TOOMANY;
    }
    
    for (i = 0; !err && i < num_nodes; ++i) {
        name_length = snprintf(numakind_name, 63, "numakind_%.4d", i);
        if (name_length == 63) {
            err = MEMKIND_ERROR_INVALID;
        }
        if (!err) {
            err = memkind_create(NUMAKIND_OPS + i, numakind_name, &numakind);
        }
        if (!err) {
            if (i == 0) {
                numakind_zero_partition_g = numakind->partition;
            }
            else if (numakind->partition != numakind_zero_partition_g + i) {
                err = -1;
            }
        }
    }
    if (err) {
        numakind_init_err_g = err;
    }
}


void *numakind_malloc(size_t size)
{
    int err = 0;
    void *result = NULL;
    memkind_t *numakind;
    char numakind_name[64];
    int name_length;
    unsigned int thread_numa_node;

    pthread_once(&numakind_init_once_g, numakind_init);
    if (numakind_init_err_g) {
        return NULL;
    }

    numakind = pthread_getspecific(numakind_key_g);
    if (numakind == NULL) {
        numakind = (memkind_t *)je_malloc(sizeof(memkind_t));
        if (numakind == NULL) {
            err = MEMKIND_ERROR_MALLOC;
        }
        if (!err) {
            thread_numa_node = numa_node_of_cpu(sched_getcpu());
            err = memkind_get_kind_by_partition(numakind_zero_partition_g + thread_numa_node, numakind);
            if (!err) {
                err = pthread_setspecific(numakind_key_g, numakind) ?
                      MEMKIND_ERROR_PTHREAD : 0;
            }
        }
    }
    if (!err) {
        result = (void *)memkind_malloc(*numakind, size);
    }
    return result;
}

void numakind_free(void *ptr)
{
    memkind_default_free(MEMKIND_DEFAULT, ptr);
}


int main(int argc, char **argv)
{
#pragma omp parallel
{
    char *data;
    int status, pref;

    data = numakind_malloc(1024);
    if (!data) {
        fprintf(stderr, "ERROR: %d\n", numakind_init_err_g);
    }
    else {
        data[0] = '\0';
        move_pages(0, 1, (void **)&data, NULL, &status, MPOL_MF_MOVE);
        fprintf(stdout, "omp_thread: %.4d numa_node: %.4d\n", omp_get_thread_num(), status);
    }
    numakind_free(data);
}
    return numakind_init_err_g;
}
