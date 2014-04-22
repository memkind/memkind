/* FILE LICENSE TAG: intel */
/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
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
 *  ##include <hbwmalloc.h>
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

typedef enum {
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
