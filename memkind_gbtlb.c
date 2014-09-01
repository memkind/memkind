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

#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>
#include <smmintrin.h>

#include "memkind_gbtlb.h"

#ifndef MAP_HUGE_1GB
# define MAP_HUGE_1GB (30 << 26)
#endif

typedef struct memkind_list_node_s {
    void *ptr;
    void *mmapptr;
    size_t size;
    struct memkind_list_node_s *next;
} memkind_list_node_t;

typedef struct {
    pthread_mutex_t mutex;
    memkind_list_node_t *list;
} memkind_table_node_t;

static int ptr_hash(void *ptr, int table_len);
static int memkind_store(void *ptr, void **mmapptr, size_t *size);
static int memkind_gb_mmap(void **result, memkind_t kind, size_t size);
static int memkind_gb_mbind(void *result, memkind_t kind, size_t size);


static int ptr_hash(void *ptr, int table_len)
{
    return _mm_crc32_u64(0, (size_t)ptr) % table_len;
}

static int memkind_gb_mmap(void **result, memkind_t kind, size_t size){
    
    void *addr=NULL;
    int ret = 0;
    int flags;
    
    ret = kind->ops->get_mmap_flags(kind, &flags);
    if (ret != 0) {
        return ret;
    }
    
    addr = mmap (NULL, size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | flags,
                 -1, 0);

    if (addr == MAP_FAILED){
        ret = MEMKIND_ERROR_MMAP;
    }
    
    *result  = addr;
    return ret;
}

static int memkind_gb_mbind(void *result, memkind_t kind, size_t size){

    int ret = 0;
    
    ret = kind->ops->mbind(kind, result, size);
    if (!ret){
        memkind_store(result, &result, &size);
    }
    else {
        munmap(result, size);
        ret = MEMKIND_ERROR_MBIND;
    }

    return ret;
}

int memkind_gbtlb_create(struct memkind *kind, const struct memkind_ops *ops, const char *name)
{
    int err = 0;

    kind->ops = ops;
    if (strlen(name) >= MEMKIND_NAME_LENGTH) {
        err = MEMKIND_ERROR_INVALID;
    }
    if (!err) {
        strcpy(kind->name, name);
    }
    return err;
}

int memkind_gbtlb_destroy(struct memkind *kind)
{
    return 0;
}

void *memkind_gbtlb_malloc(struct memkind *kind, size_t size)
{
    
    void *result = NULL;
    int err = 0;
    
    err = memkind_gb_mmap(&result, kind, size);
    if (err != 0){
        result = NULL;
        return result;
    }
    else{
        err = memkind_gb_mbind(result, kind, size);
        if (err != 0){
            result = NULL;
            return result;
        }
    }
    return result;
}

void *memkind_gbtlb_calloc(struct memkind *kind, size_t num, size_t size)
{
    void *result = NULL;
    int err = 0;
    size_t t_size;
    
    t_size = num * size;
    
    err = memkind_gb_mmap(&result, kind, t_size);
    if (err != 0){
        result = NULL;
        return result;
    }
    else {
        err = memkind_gb_mbind(result, kind, t_size);
        if (err != 0){
            result = NULL;
            return result;
        }
        else{
            memset(result, 0x0, t_size);
        }
    }
    return result;
}

int memkind_gbtlb_posix_memalign(struct memkind *kind, void **memptr, size_t alignment, size_t size)
{
    int err = 0;
    void *mmapptr;
    
    err = memkind_gb_mmap(&mmapptr, kind, size);
    if (err != 0){
        memptr = NULL;
        return err;
    }
    else {
        err = kind->ops->mbind(kind, mmapptr, size);
        if (!err) {
            if (alignment) {
                *memptr = (void *) ((char *)mmapptr + 
                                    (size_t)(mmapptr) % alignment);
            }
            else {
                *memptr = mmapptr;
            }
            memkind_store (*memptr, &mmapptr, &size);
        }
        else {
            munmap (mmapptr, size);
        }
    }
    return err;
}

void *memkind_gbtlb_realloc(struct memkind *kind, void *ptr, size_t size)
{
   
    size_t tmp_size;
    void *tmp_ptr = NULL;

    tmp_size = 0;
    
    if (NULL == ptr){
        kind->ops->malloc(kind, size);
    }
    else {
        memkind_store (ptr, &tmp_ptr, &tmp_size);
        if (NULL == tmp_ptr){
            ptr = tmp_ptr;
        }
        else {
            tmp_ptr = NULL;
            tmp_size  += size;
            tmp_ptr = kind->ops->malloc(kind, tmp_size);
            memcpy(tmp_ptr, ptr, size);
            munmap(ptr, size);
            ptr = tmp_ptr;
        }
    }
    return ptr;
}

void memkind_gbtlb_free(struct memkind *kind, void *ptr)
{
    int err;
    void *mmapptr = NULL;
    size_t size = 0;
    
    err = memkind_store(ptr, &mmapptr, &size);
    if (!err) {
        munmap(mmapptr, size);
    }
}

int memkind_gbtlb_mbind(struct memkind *kind, void *ptr, size_t len)
{
    nodemask_t nodemask;
    int err = 0;
    int mode;

    err = kind->ops->get_mbind_nodemask(kind, nodemask.n, NUMA_NUM_NODES);
    if (!err) {
        err = kind->ops->get_mbind_mode(kind, &mode);
    }
    if (!err) {
        err = mbind(ptr, len, mode, nodemask.n, NUMA_NUM_NODES, 0);
        err = err ? MEMKIND_ERROR_MBIND : 0;
    }
    return err;
}

int memkind_gbtlb_get_mmap_flags(struct memkind *kind, int *flags)
{
    *flags = MAP_PRIVATE | MAP_HUGETLB | MAP_HUGE_1GB | MAP_ANONYMOUS;
    return 0;
}

int memkind_gbtlb_get_mbind_mode(struct memkind *kind, int *mode)
{
    *mode = MPOL_BIND;
    return 0;
}

int memkind_gbtlb_preferred_get_mbind_mode(struct memkind *kind, int *mode)
{
    *mode = MPOL_PREFERRED;
    return 0;
}

static int memkind_store(void *memptr, void **mmapptr, size_t *size)
{
    static int table_len = 0;
    static int is_init = 0;
    static memkind_table_node_t *table = NULL;
    static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
    int err = 0;
    int hash, i;
    memkind_list_node_t *storeptr, *lastptr;

    if (!is_init && *mmapptr == NULL) {
        return -1;
    }

    if (!is_init) {
        pthread_mutex_lock(&init_mutex);
        if (!is_init) {
            table_len = numa_num_configured_cpus();
            table = malloc(sizeof(memkind_table_node_t) * table_len);
            for (i = 0; i < table_len; ++i) {
                pthread_mutex_init(&(table[i].mutex), NULL);
                table[i].list = NULL;
            }
            is_init = 1;
        }
        pthread_mutex_unlock(&init_mutex);
    }

    hash = ptr_hash(memptr, table_len);
    pthread_mutex_lock(&(table[hash].mutex));
    if (*mmapptr == NULL) { /* memkind_store() call is a query */
        storeptr = table[hash].list;
        lastptr = NULL;
        while (storeptr && storeptr->ptr != memptr) {
            lastptr = storeptr;
            storeptr = storeptr->next;
        }
        if (storeptr) {
            *mmapptr = storeptr->mmapptr;
            *size = storeptr->size;
            if (lastptr) {
                lastptr->next = storeptr->next;
            }
            else {
                table[hash].list = storeptr->next;
            }
            free(storeptr);
        }
        else {
            err = -2;
        }
    }
    else { /* memkind_store() call is a store */
        storeptr = table[hash].list;
        table[hash].list = (memkind_list_node_t*)malloc(sizeof(memkind_list_node_t));
        table[hash].list->ptr = memptr;
        table[hash].list->mmapptr = *mmapptr;
        table[hash].list->size = *size;
        table[hash].list->next = storeptr;
    }
    pthread_mutex_unlock(&(table[hash].mutex));
    return err;
}

