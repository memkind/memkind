/*
 * Copyright (C) 2014 - 2016 Intel Corporation.
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

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/*
 * Header file for the memkind heap manager.
 * More details in memkind(3) man page.
 *
 * API standards are described in memkind(3) man page.
 */

/*EXPERIMENTAL API*/
enum memkind_const {
    MEMKIND_MAX_KIND = 512,
    MEMKIND_ERROR_MESSAGE_SIZE = 128
};

/*EXPERIMENTAL API*/
enum memkind_error {
    MEMKIND_ERROR_UNAVAILABLE = -1,
    MEMKIND_ERROR_MBIND = -2,
    MEMKIND_ERROR_MMAP  = -3,
    MEMKIND_ERROR_MALLOC = -6,
    MEMKIND_ERROR_ENVIRON = -12,
    MEMKIND_ERROR_INVALID = -13,
    MEMKIND_ERROR_TOOMANY = -15,
    MEMKIND_ERROR_BADOPS = -17,
    MEMKIND_ERROR_HUGETLB = -18,
    MEMKIND_ERROR_RUNTIME = -255
};

/*EXPERIMENTAL API*/
struct memkind;

typedef struct memkind * memkind_t;

#include "memkind_deprecated.h"

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_DEFAULT;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_HUGETLB;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_HBW;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_HBW_PREFERRED;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_HBW_HUGETLB;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_HBW_PREFERRED_HUGETLB;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_HBW_GBTLB;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_HBW_PREFERRED_GBTLB;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_GBTLB;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_HBW_INTERLEAVE;

/*EXPERIMENTAL API*/
extern memkind_t MEMKIND_INTERLEAVE;


/*STANDARD API*/
/* API versioning */
int memkind_get_version();


/*EXPERIMENTAL API*/
/* Convert error number into an error message */
void memkind_error_message(int err, char *msg, size_t size);


/* KIND MANAGEMENT INTERFACE */

/*EXPERIMENTAL API*/
/* Create a new PMEM (file-backed) kind of given size on top of a temporary file */
int memkind_create_pmem(const char *dir, size_t max_size, memkind_t *kind);

/*EXPERIMENTAL API*/
/* returns 0 if memory kind is available else returns error code */
int memkind_check_available(memkind_t kind);

/* HEAP MANAGEMENT INTERFACE */

/*EXPERIMENTAL API*/
/* malloc from the numa nodes of the specified kind */
void *memkind_malloc(memkind_t kind, size_t size);

/*EXPERIMENTAL API*/
/* calloc from the numa nodes of the specified kind */
void *memkind_calloc(memkind_t kind, size_t num, size_t size);

/*EXPERIMENTAL API*/
/* posix_memalign from the numa nodes of the specified kind */
int memkind_posix_memalign(memkind_t kind, void **memptr, size_t alignment,
                           size_t size);

/*EXPERIMENTAL API*/
/* realloc from the numa nodes of the specified kind */
void *memkind_realloc(memkind_t kind, void *ptr, size_t size);

/*EXPERIMENTAL API*/
/* Free memory allocated with the memkind API */
void memkind_free(memkind_t kind, void *ptr);

#ifdef __cplusplus
}
#endif
