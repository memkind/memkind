/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2018, Intel Corporation */

#ifndef CRITNIB_H
#define CRITNIB_H 1

#include "memkind/internal/fast_slab_allocator.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DELETED_LIFE 16

#define SLICE   4
#define NIB     ((1UL << SLICE) - 1)
#define SLNODES (1 << SLICE)

typedef uintptr_t word;
typedef unsigned char sh_t;

struct critnib_node {
    /*
     * path is the part of a tree that's already traversed (be it through
     * explicit nodes or collapsed links) -- ie, any subtree below has all
     * those bits set to this value.
     *
     * nib is a 4-bit slice that's an index into the node's children.
     *
     * shift is the length (in bits) of the part of the key below this node.
     *
     *            nib
     * |XXXXXXXXXX|?|*****|
     *    path      ^
     *              +-----+
     *               shift
     */
    struct critnib_node *child[SLNODES];
    word path;
    sh_t shift;
};

struct critnib_leaf {
    word key;
    void *value;
};

struct critnib {
    struct critnib_node *root;

    /* pool of freed nodes: singly linked list, next at child[0] */
    struct critnib_node *deleted_node;
    struct critnib_leaf *deleted_leaf;

    /* nodes removed but not yet eligible for reuse */
    struct critnib_node *pending_del_nodes[DELETED_LIFE];
    struct critnib_leaf *pending_del_leaves[DELETED_LIFE];

    FastSlabAllocator node_alloc;
    FastSlabAllocator leaf_alloc;
    uint64_t remove_count;

    pthread_mutex_t mutex; /* writes/removes */
};

struct critnib;
typedef struct critnib critnib;

enum find_dir_t
{
    FIND_L = -2,
    FIND_LE = -1,
    FIND_EQ = 0,
    FIND_GE = +1,
    FIND_G = +2,
};

int critnib_create(critnib *c);
void critnib_destroy(critnib *c);

int critnib_insert(critnib *c, uintptr_t key, void *value, int update);
void *critnib_remove(critnib *c, uintptr_t key);
void *critnib_get(critnib *c, uintptr_t key);
void *critnib_find_le(critnib *c, uintptr_t key);
int critnib_find(critnib *c, uintptr_t key, enum find_dir_t dir,
                 uintptr_t *rkey, void **rvalue);
void critnib_iter(critnib *c, uintptr_t min, uintptr_t max,
                  int (*func)(uintptr_t key, void *value, void *privdata),
                  void *privdata);

#ifdef __cplusplus
}
#endif

#endif
