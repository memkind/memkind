#ifndef numakind_include_h
#define numakind_include_h
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NUMAKIND_DEFAULT = 0,
    NUMAKIND_MCDRAM,
    NUMAKIND_MCDRAM_HUGETLB,
    NUMAKIND_NUM_KIND
} numakind_t;

typedef enum {
    NUMAKIND_ERROR_UNAVAILABLE = -1,
    NUMAKIND_ERROR_MBIND = -2,
    NUMAKIND_ERROR_MEMALIGN = -3,
    NUMAKIND_ERROR_MALLCTL = -4,
    NUMAKIND_ERROR_MALLOC = -5,
    NUMAKIND_ERROR_GETCPU = -6,
    NUMAKIND_ERROR_MCDRAM = -7,
    NUMAKIND_ERROR_PMTT = -8,
    NUMAKIND_ERROR_TIEDISTANCE = -9,
    NUMAKIND_ERROR_ALIGNMENT = -10,
    NUMAKIND_ERROR_ALLOCM = -11
} numakind_error_t;

static const size_t NUMAKIND_ERROR_MESSAGE_SIZE = 128;

/* Convert error number into an error message */
void numakind_error_message(int err, char *msg, size_t size);

/* returns 1 if numa kind is availble else 0 */
int numakind_isavail(int kind);

/* sets nodemask for the nearest numa node for the specified numa kind */
int numakind_nodemask(int kind, unsigned long *nodemask, unsigned long maxnode);

/* set flags for call to mmap */
int numakind_mmap_flags(int kind, int *flags);

/* mbind to the nearest numa node of the specified kind */
int numakind_mbind(int kind, void *addr, size_t len);

/* malloc from the nearest numa node of the specified kind */
void *numakind_malloc(numakind_t kind, size_t size);

/* calloc from the nearest numa node of the specified kind */
void *numakind_calloc(numakind_t kind, size_t num, size_t size);

/* posix_memalign from the nearest numa node of the specified kind */
int numakind_posix_memalign(numakind_t kind, void **memptr, size_t alignment,
        size_t size);

/* realloc from the nearest numa node of the specified kind */
void *numakind_realloc(numakind_t kind, void *ptr, size_t size);

/* Free memory allocated with the numakind API */
void numakind_free(numakind_t kind, void *ptr);

#ifdef __cplusplus
}
#endif
#endif
