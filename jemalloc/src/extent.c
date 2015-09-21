#define	JEMALLOC_EXTENT_C_
#include "jemalloc/internal/jemalloc_internal.h"

/******************************************************************************/

static inline int
extent_szad_comp(extent_node_t *a, extent_node_t *b)
{
	int ret;
#ifdef JEMALLOC_ENABLE_MEMKIND
        unsigned a_partition = a->partition;
        unsigned b_partition = b->partition;

        ret = (a_partition > b_partition) - (a_partition < b_partition);

	if (ret == 0) {
#endif
		size_t a_size = a->size;
		size_t b_size = b->size;

		ret = (a_size > b_size) - (a_size < b_size);
		if (ret == 0) {
			uintptr_t a_addr = (uintptr_t)a->addr;
			uintptr_t b_addr = (uintptr_t)b->addr;

			ret = (a_addr > b_addr) - (a_addr < b_addr);
		}
#ifdef JEMALLOC_ENABLE_MEMKIND
	}
#endif

	return (ret);
}

/* Generate red-black tree functions. */
rb_gen(, extent_tree_szad_, extent_tree_t, extent_node_t, link_szad,
    extent_szad_comp)

static inline int
extent_ad_comp(extent_node_t *a, extent_node_t *b)
{
	int ret;
#ifdef JEMALLOC_ENABLE_MEMKIND
        unsigned a_partition = a->partition;
        unsigned b_partition = b->partition;

        ret = (a_partition > b_partition) - (a_partition < b_partition);

	if (ret == 0) {
#endif
		uintptr_t a_addr = (uintptr_t)a->addr;
		uintptr_t b_addr = (uintptr_t)b->addr;

		ret = (a_addr > b_addr) - (a_addr < b_addr);
#ifdef JEMALLOC_ENABLE_MEMKIND
	}
#endif

	return (ret);
}

/* Generate red-black tree functions. */
rb_gen(, extent_tree_ad_, extent_tree_t, extent_node_t, link_ad,
    extent_ad_comp)
