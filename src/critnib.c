// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2022, Intel Corporation */

/*
 * critnib.c -- implementation of critnib tree
 *
 * It offers identity lookup (like a hashmap) and <= lookup (like a search
 * tree).  Unlike some hashing algorithms (cuckoo hash, perfect hashing) the
 * complexity isn't constant, but for data sizes we expect it's several
 * times as fast as cuckoo, and has no "stop the world" cases that would
 * cause latency (ie, better worst case behaviour).
 */

/*
 * STRUCTURE DESCRIPTION
 *
 * Critnib is a hybrid between a radix tree and DJ Bernstein's critbit:
 * it skips nodes for uninteresting radix nodes (ie, ones that would have
 * exactly one child), this requires adding to every node a field that
 * describes the slice (4-bit in our case) that this radix level is for.
 *
 * This implementation also stores each node's path (ie, bits that are
 * common to every key in that subtree) -- this doesn't help with lookups
 * at all (unused in == match, could be reconstructed at no cost in <=
 * after first dive) but simplifies inserts and removes.  If we ever want
 * that piece of memory it's easy to trim it down.
 */

/*
 * CONCURRENCY ISSUES
 *
 * Reads are completely lock-free sync-free, but only almost wait-free:
 * if for some reason a read thread gets pathologically stalled, it will
 * notice the data being stale and restart the work.  In usual cases,
 * the structure having been modified does _not_ cause a restart.
 *
 * Writes could be easily made lock-free as well (with only a cmpxchg
 * sync), but this leads to problems with removes.  A possible solution
 * would be doing removes by overwriting by NULL w/o freeing -- yet this
 * would lead to the structure growing without bounds.  Complex per-node
 * locks would increase concurrency but they slow down individual writes
 * enough that in practice a simple global write lock works faster.
 *
 * Removes are the only operation that can break reads.  The structure
 * can do local RCU well -- the problem being knowing when it's safe to
 * free.  Any synchronization with reads would kill their speed, thus
 * instead we have a remove count.  The grace period is DELETED_LIFE,
 * after which any read will notice staleness and restart its work.
 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "memkind/internal/critnib.h"

#define util_lssb_index64(x) ((unsigned char)__builtin_ctzll(x))
#define util_mssb_index64(x) ((unsigned char)(63 - __builtin_clzll(x)))

/*
 * A node that has been deleted is left untouched for this many delete
 * cycles.  Reads have guaranteed correctness if they took no longer than
 * DELETED_LIFE concurrent deletes, otherwise they notice something is
 * wrong and restart.  The memory of deleted nodes is never freed to
 * malloc nor their pointers lead anywhere wrong, thus a stale read will
 * (temporarily) get a wrong answer but won't crash.
 *
 * There's no need to count writes as they never interfere with reads.
 *
 * Allowing stale reads (of arbitrarily old writes or of deletes less than
 * DELETED_LIFE old) might sound counterintuitive, but it doesn't affect
 * semantics in any way: the thread could have been stalled just after
 * returning from our code.  Thus, the guarantee is: the result of get() or
 * find_le() is a value that was current at any point between the call
 * start and end.
 */

/*
 * atomic load
 */
static void load(void *src, void *dst)
{
    __atomic_load((word *)src, (word *)dst, memory_order_acquire);
}

static void load64(uint64_t *src, uint64_t *dst)
{
    __atomic_load(src, dst, memory_order_acquire);
}

/*
 * atomic store
 */
static void store(void *dst, void *src)
{
    __atomic_store_n((word *)dst, (word)src, memory_order_release);
}

/*
 * internal: is_leaf -- check tagged pointer for leafness
 */
static inline bool is_leaf(struct critnib_node *n)
{
    return (word)n & 1;
}

/*
 * internal: to_leaf -- untag a leaf pointer
 */
static inline struct critnib_leaf *to_leaf(struct critnib_node *n)
{
    return (void *)((word)n & ~1UL);
}

/*
 * internal: path_mask -- return bit mask of a path above a subtree [shift]
 * bits tall
 */
static inline word path_mask(sh_t shift)
{
    return ~NIB << shift;
}

/*
 * internal: slice_index -- return index of child at the given nib
 */
static inline unsigned slice_index(word key, sh_t shift)
{
    return (unsigned)((key >> shift) & NIB);
}

/*
 * critnib_new -- allocates a new critnib structure
 */
int critnib_create(critnib *__restrict c)
{
    assert(c && "critnib c cannot be NULL");
    (void)memset(c, 0, sizeof(critnib));
    pthread_mutex_init(&c->mutex, NULL);

    int ret1 = fast_slab_allocator_init(&c->node_alloc,
                                        sizeof(struct critnib_node), 0ul);
    int ret2 = fast_slab_allocator_init(&c->leaf_alloc,
                                        sizeof(struct critnib_leaf), 0ul);
    return (int)(ret1 == 0 && ret2 == 0) ? 0 : -1;
}

/*
 * internal: delete_node -- recursively free (to malloc) a subtree
 */
static void delete_node(struct critnib *c, struct critnib_node *__restrict n)
{
    if (!is_leaf(n)) {
        for (int i = 0; i < SLNODES; i++) {
            if (n->child[i])
                delete_node(c, n->child[i]);
        }

        fast_slab_allocator_free(&c->node_alloc, n);
    } else {
        fast_slab_allocator_free(&c->leaf_alloc, to_leaf(n));
    }
}

/*
 * critnib_destroy -- destroy and free a critnib struct
 */
void critnib_destroy(struct critnib *c)
{
    if (c->root)
        delete_node(c, c->root);

    pthread_mutex_destroy(&c->mutex);

    for (struct critnib_node *m = c->deleted_node; m;) {
        struct critnib_node *mm = m->child[0];
        fast_slab_allocator_free(&c->node_alloc, m);
        m = mm;
    }

    for (struct critnib_leaf *k = c->deleted_leaf; k;) {
        struct critnib_leaf *kk = k->value;
        fast_slab_allocator_free(&c->leaf_alloc, k);
        k = kk;
    }

    for (int i = 0; i < DELETED_LIFE; i++) {
        fast_slab_allocator_free(&c->node_alloc, c->pending_del_nodes[i]);
        fast_slab_allocator_free(&c->leaf_alloc, c->pending_del_leaves[i]);
    }
    fast_slab_allocator_destroy(&c->leaf_alloc);
    fast_slab_allocator_destroy(&c->node_alloc);
}

/*
 * internal: free_node -- free (to internal pool, not malloc) a node.
 *
 * We cannot free them to malloc as a stalled reader thread may still walk
 * through such nodes; it will notice the result being bogus but only after
 * completing the walk, thus we need to ensure any freed nodes still point
 * to within the critnib structure.
 */
static void free_node(struct critnib *__restrict c,
                      struct critnib_node *__restrict n)
{
    if (!n)
        return;

    assert(!is_leaf(n) && "leaf passed to free_node instead of free_leaf");
    n->child[0] = c->deleted_node;
    c->deleted_node = n;
}

/*
 * internal: alloc_node -- allocate a node from our pool or from malloc
 */
static struct critnib_node *alloc_node(struct critnib *__restrict c)
{
    if (!c->deleted_node)
        return fast_slab_allocator_malloc(&c->node_alloc);

    struct critnib_node *n = c->deleted_node;

    c->deleted_node = n->child[0];

    return n;
}

/*
 * internal: free_leaf -- free (to internal pool, not malloc) a leaf.
 *
 * See free_node().
 */
static void free_leaf(struct critnib *__restrict c,
                      struct critnib_leaf *__restrict k)
{
    if (!k)
        return;

    k->value = c->deleted_leaf;
    c->deleted_leaf = k;
}

/*
 * internal: alloc_leaf -- allocate a leaf from our pool or from malloc
 */
static struct critnib_leaf *alloc_leaf(struct critnib *__restrict c)
{
    if (!c->deleted_leaf)
        return fast_slab_allocator_malloc(&c->leaf_alloc);

    struct critnib_leaf *k = c->deleted_leaf;

    c->deleted_leaf = k->value;

    return k;
}

/*
 * crinib_insert -- write a key:value pair to the critnib structure
 *
 * Returns:
 *  • 0 on success
 *  • EEXIST if such a key already exists
 *  • ENOMEM if we're out of memory
 *
 * Takes a global write lock but doesn't stall any readers.
 */
int critnib_insert(struct critnib *c, word key, void *value, int update)
{
    pthread_mutex_lock(&c->mutex);

    struct critnib_leaf *k = alloc_leaf(c);
    if (!k) {
        pthread_mutex_unlock(&c->mutex);

        return ENOMEM;
    }

    k->key = key;
    k->value = value;

    struct critnib_node *kn = (void *)((word)k | 1);

    struct critnib_node *n = c->root;
    if (!n) {
        c->root = kn;

        pthread_mutex_unlock(&c->mutex);

        return 0;
    }

    struct critnib_node **parent = &c->root;
    struct critnib_node *prev = c->root;

    while (n && !is_leaf(n) && (key & path_mask(n->shift)) == n->path) {
        prev = n;
        parent = &n->child[slice_index(key, n->shift)];
        n = *parent;
    }

    if (!n) {
        n = prev;
        store(&n->child[slice_index(key, n->shift)], kn);

        pthread_mutex_unlock(&c->mutex);

        return 0;
    }

    word path = is_leaf(n) ? to_leaf(n)->key : n->path;
    /* Find where the path differs from our key. */
    word at = path ^ key;
    if (!at) {
        assert(is_leaf(n));
        free_leaf(c, to_leaf(kn));

        if (update) {
            to_leaf(n)->value = value;
            pthread_mutex_unlock(&c->mutex);
            return 0;
        } else {
            pthread_mutex_unlock(&c->mutex);
            return EEXIST;
        }
    }

    /* and convert that to an index. */
#if __SIZEOF_SIZE_T__ != 8
#error "only 64-bit machines are supported"
#endif
    sh_t sh = util_mssb_index64(at) & (sh_t) ~(SLICE - 1);

    struct critnib_node *m = alloc_node(c);
    if (!m) {
        free_leaf(c, to_leaf(kn));

        pthread_mutex_unlock(&c->mutex);

        return ENOMEM;
    }

    for (int i = 0; i < SLNODES; i++)
        m->child[i] = NULL;

    m->child[slice_index(key, sh)] = kn;
    m->child[slice_index(path, sh)] = n;
    m->shift = sh;
    m->path = key & path_mask(sh);
    store(parent, m);

    pthread_mutex_unlock(&c->mutex);

    return 0;
}

/*
 * critnib_remove -- delete a key from the critnib structure, return its value
 */
void *critnib_remove(struct critnib *c, word key)
{
    struct critnib_leaf *k;
    void *value = NULL;

    pthread_mutex_lock(&c->mutex);

    struct critnib_node *n = c->root;
    if (!n)
        goto not_found;

    word del = __atomic_fetch_add(&c->remove_count, 1, __ATOMIC_ACQ_REL) %
        DELETED_LIFE;
    free_node(c, c->pending_del_nodes[del]);
    free_leaf(c, c->pending_del_leaves[del]);
    c->pending_del_nodes[del] = NULL;
    c->pending_del_leaves[del] = NULL;

    if (is_leaf(n)) {
        k = to_leaf(n);
        if (k->key == key) {
            store(&c->root, NULL);
            goto del_leaf;
        }

        goto not_found;
    }
    /*
     * n and k are a parent:child pair (after the first iteration); k is the
     * leaf that holds the key we're deleting.
     */
    struct critnib_node **k_parent = &c->root;
    struct critnib_node **n_parent = &c->root;
    struct critnib_node *kn = n;

    while (!is_leaf(kn)) {
        n_parent = k_parent;
        n = kn;
        k_parent = &kn->child[slice_index(key, kn->shift)];
        kn = *k_parent;

        if (!kn)
            goto not_found;
    }

    k = to_leaf(kn);
    if (k->key != key)
        goto not_found;

    store(&n->child[slice_index(key, n->shift)], NULL);

    /* Remove the node if there's only one remaining child. */
    int ochild = -1;
    for (int i = 0; i < SLNODES; i++) {
        if (n->child[i]) {
            if (ochild != -1)
                goto del_leaf;

            ochild = i;
        }
    }

    assert(ochild != -1);

    store(n_parent, n->child[ochild]);
    c->pending_del_nodes[del] = n;

del_leaf:
    value = k->value;
    c->pending_del_leaves[del] = k;

not_found:
    pthread_mutex_unlock(&c->mutex);
    return value;
}

/*
 * critnib_get -- query for a key ("==" match), returns value or NULL
 *
 * Doesn't need a lock but if many deletes happened while our thread was
 * somehow stalled the query is restarted (as freed nodes remain unused only
 * for a grace period).
 *
 * Counterintuitively, it's pointless to return the most current answer,
 * we need only one that was valid at any point after the call started.
 */
void *critnib_get(struct critnib *c, word key)
{
    uint64_t wrs1, wrs2;
    void *res;

    do {
        struct critnib_node *n;

        load64(&c->remove_count, &wrs1);
        load(&c->root, &n);

        /*
         * critbit algorithm: dive into the tree, looking at nothing but
         * each node's critical bit^H^H^Hnibble.  This means we risk
         * going wrong way if our path is missing, but that's ok...
         */
        while (n && !is_leaf(n))
            load(&n->child[slice_index(key, n->shift)], &n);

        /* ... as we check it at the end. */
        struct critnib_leaf *k = to_leaf(n);
        res = (n && k->key == key) ? k->value : NULL;
        load64(&c->remove_count, &wrs2);
    } while (wrs1 + DELETED_LIFE <= wrs2);

    return res;
}

/*
 * internal: find_predecessor -- return the rightmost leaf in a subtree
 */
static struct critnib_leaf *find_predecessor(struct critnib_node *__restrict n)
{
    while (1) {
        int nib;
        for (nib = NIB; nib >= 0; nib--)
            if (n->child[nib])
                break;

        if (nib < 0)
            return NULL;

        n = n->child[nib];
        if (is_leaf(n))
            return to_leaf(n);
    }
}

/*
 * internal: find_le -- recursively search <= in a subtree
 */
static struct critnib_leaf *find_le(struct critnib_node *__restrict n, word key)
{
    if (!n)
        return NULL;

    if (is_leaf(n)) {
        struct critnib_leaf *k = to_leaf(n);
        return (k->key <= key) ? k : NULL;
    }

    /*
     * is our key outside the subtree we're in?
     *
     * If we're inside, all bits above the nib will be identical; note
     * that shift points at the nib's lower rather than upper edge, so it
     * needs to be masked away as well.
     */
    if ((key ^ n->path) >> (n->shift) & ~NIB) {
        /*
         * subtree is too far to the left?
         * -> its rightmost value is good
         */
        if (n->path < key)
            return find_predecessor(n);

        /*
         * subtree is too far to the right?
         * -> it has nothing of interest to us
         */
        return NULL;
    }

    unsigned nib = slice_index(key, n->shift);
    /* recursive call: follow the path */
    {
        struct critnib_node *m;
        load(&n->child[nib], &m);
        struct critnib_leaf *k = find_le(m, key);
        if (k)
            return k;
    }

    /*
     * nothing in that subtree?  We strayed from the path at this point,
     * thus need to search every subtree to our left in this node.  No
     * need to dive into any but the first non-null, though.
     */
    for (; nib > 0; nib--) {
        struct critnib_node *m;
        load(&n->child[nib - 1], &m);
        if (m) {
            n = m;
            if (is_leaf(n))
                return to_leaf(n);

            return find_predecessor(n);
        }
    }

    return NULL;
}

/*
 * critnib_find_le -- query for a key ("<=" match), returns value or NULL
 *
 * Same guarantees as critnib_get().
 */
void *critnib_find_le(struct critnib *c, word key)
{
    uint64_t wrs1, wrs2;
    void *res;

    do {
        load64(&c->remove_count, &wrs1);
        struct critnib_node *n; /* avoid a subtle TOCTOU */
        load(&c->root, &n);
        struct critnib_leaf *k = n ? find_le(n, key) : NULL;
        res = k ? k->value : NULL;
        load64(&c->remove_count, &wrs2);
    } while (wrs1 + DELETED_LIFE <= wrs2);

    return res;
}

/*
 * internal: find_successor -- return the rightmost leaf in a subtree
 */
static struct critnib_leaf *find_successor(struct critnib_node *__restrict n)
{
    while (1) {
        unsigned nib;
        for (nib = 0; nib <= NIB; nib++)
            if (n->child[nib])
                break;

        if (nib > NIB)
            return NULL;

        n = n->child[nib];
        if (is_leaf(n))
            return to_leaf(n);
    }
}

/*
 * internal: find_ge -- recursively search >= in a subtree
 */
static struct critnib_leaf *find_ge(struct critnib_node *__restrict n, word key)
{
    if (!n)
        return NULL;

    if (is_leaf(n)) {
        struct critnib_leaf *k = to_leaf(n);
        return (k->key >= key) ? k : NULL;
    }

    if ((key ^ n->path) >> (n->shift) & ~NIB) {
        if (n->path > key)
            return find_successor(n);

        return NULL;
    }

    unsigned nib = slice_index(key, n->shift);
    {
        struct critnib_node *m;
        load(&n->child[nib], &m);
        struct critnib_leaf *k = find_ge(m, key);
        if (k)
            return k;
    }

    for (; nib < NIB; nib++) {
        struct critnib_node *m;
        load(&n->child[nib + 1], &m);
        if (m) {
            n = m;
            if (is_leaf(n))
                return to_leaf(n);

            return find_successor(n);
        }
    }

    return NULL;
}

/*
 * critnib_find -- parametrized query, returns 1 if found
 */
int critnib_find(struct critnib *c, uintptr_t key, enum find_dir_t dir,
                 uintptr_t *rkey, void **rvalue)
{
    uint64_t wrs1, wrs2;
    struct critnib_leaf *k;
    uintptr_t _rkey;
    void **_rvalue;

    /* <42 ≡ ≤41 */
    if (dir < -1) {
        if (!key)
            return 0; /* no key is <0 */
        key--;
    } else if (dir > +1) {
        if (key == (uintptr_t)-1)
            return 0; /* no key is >(unsigned)∞ */
        key++;
    }

    do {
        load64(&c->remove_count, &wrs1);
        struct critnib_node *n;
        load(&c->root, &n);

        if (dir < 0)
            k = find_le(n, key);
        else if (dir > 0)
            k = find_ge(n, key);
        else {
            while (n && !is_leaf(n))
                load(&n->child[slice_index(key, n->shift)], &n);

            struct critnib_leaf *kk = to_leaf(n);
            k = (n && kk->key == key) ? kk : NULL;
        }
        if (k) {
            _rkey = k->key;
            _rvalue = k->value;
        }
        load64(&c->remove_count, &wrs2);
    } while (wrs1 + DELETED_LIFE <= wrs2);

    if (k) {
        if (rkey)
            *rkey = _rkey;
        if (rvalue)
            *rvalue = _rvalue;
        return 1;
    }

    return 0;
}

/*
 * critnib_iter -- iterator, [min..max], calls func(key, value, privdata)
 *
 * If func() returns non-zero, the search is aborted.
 */
static int iter(struct critnib_node *__restrict n, word min, word max,
                int (*func)(word key, void *value, void *privdata),
                void *privdata)
{
    if (is_leaf(n)) {
        word k = to_leaf(n)->key;
        if (k >= min && k <= max)
            return func(to_leaf(n)->key, to_leaf(n)->value, privdata);
        return 0;
    }

    if (n->path > max)
        return 1;
    if ((n->path | path_mask(n->shift)) < min)
        return 0;

    for (int i = 0; i < SLNODES; i++) {
        struct critnib_node *__restrict m = n->child[i];
        if (m && iter(m, min, max, func, privdata))
            return 1;
    }

    return 0;
}

void critnib_iter(critnib *c, uintptr_t min, uintptr_t max,
                  int (*func)(uintptr_t key, void *value, void *privdata),
                  void *privdata)
{
    pthread_mutex_lock(&c->mutex);
    if (c->root)
        iter(c->root, min, max, func, privdata);
    pthread_mutex_unlock(&c->mutex);
}
