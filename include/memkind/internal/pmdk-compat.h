#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef pthread_mutex_t os_mutex_t;

#define Malloc malloc
#define Free free

static void *Zalloc(size_t s)
{
	void *m = Malloc(s);
	if (m)
		memset(m, 0, s);
	return m;
}

#define util_mutex_init(x)	pthread_mutex_init(x, NULL)
#define util_mutex_destroy(x)	pthread_mutex_destroy(x)
#define util_mutex_lock(x)	pthread_mutex_lock(x)
#define util_mutex_unlock(x)	pthread_mutex_unlock(x)
#define util_lssb_index64(x)	((unsigned char)__builtin_ctzll(x))
#define util_mssb_index64(x)	((unsigned char)(63 - __builtin_clzll(x)))
#define util_lssb_index32(x)	((unsigned char)__builtin_ctzl(x))
#define util_mssb_index32(x)	((unsigned char)(31 - __builtin_clzl(x)))


#define NOFUNCTION do ; while(0)

// Make these an unthing for now...
#define ASSERT(x) NOFUNCTION
#define ASSERTne(x, y) ASSERT(x != y)
#define VALGRIND_ANNOTATE_NEW_MEMORY(p, s) NOFUNCTION
#define VALGRIND_HG_DRD_DISABLE_CHECKING(p, s) NOFUNCTION
