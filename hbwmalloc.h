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

#ifndef hbw_malloc_include_h
#define hbw_malloc_include_h
#ifdef __cplusplus
extern "C" {
#endif
/*!
 *  \file hbwmalloc.h
 *  \brief Header file for the high bandwidth memory interface.
 *
 *  #include <hbwmalloc.h>
 *
 *  Link with -lnuma -lnumakind
 *
 *  This file defines the external API's and enumerations for the
 *  hbwmalloc library.  These interfaces define a heap manager that
 *  targets the high bandwidth memory numa nodes.
 *
 *  \section ENVIRONMENT
 *  \subsection NUMAKIND_HBW_NODES
 *  Comma separated list of NUMA nodes that are treated as high
 *  bandwidth.  Can be used if pmtt file is not present.  Uses same
 *  parser as numactl, so logic defined in numactl man pages applies:
 *  e.g 1-3,5 is a valid setting.
 */

/*!
 *  \brief Fallback policy.
 *
 *  Policy that determines behavior when there is not enough free high
 *  bandwidth memory to satisfy a user request.  This enum is used with
 *  hbw_get_policy() and hbw_set_policy().
 */
typedef enum {
    /*!
     *  If insufficient high bandwidth memory pages are available seg
     *  fault when memory is touched (default).
     */
    HBW_POLICY_BIND = 1,
    /*!
     *  If insufficient high bandwidth memory pages are available fall
     *  back on standard memory pages.
     */
    HBW_POLICY_PREFERRED = 2
} hbw_policy_t;

/*!
 *  \brief Page size selection.
 *
 *  The hbw_allocate_memalign_psize() API gives the user the option to
 *  select the page size from this enumerated list.
 */
typedef enum {
    /*!
     *  The 4 kilobyte page size option, this enables the same behavior
     *  from hbw_allocate_memalign_psize() as the hbw_allocate_memalign()
     *  API.  Note with transparent huge pages enabled, these allocations
     *  may be promoted by the operating system to 2 megabyte pages.
     */
    HBW_PAGESIZE_4KB = 1,
    HBW_PAGESIZE_2MB = 2
} hbw_pagesize_t;

int hbw_get_policy(void);
void hbw_set_policy(int mode);
int hbw_is_available(void);
void *hbw_malloc(size_t size);
void *hbw_calloc(size_t num, size_t size);
int hbw_allocate_memalign(void **memptr, size_t alignment, size_t size);
int hbw_allocate_memalign_psize(void **memptr, size_t alignment, size_t size,
                                int pagesize);
void *hbw_realloc(void *ptr, size_t size);
void hbw_free(void *ptr);

#ifdef __cplusplus
}
#endif
#endif
