/*
 * Copyright (C) 2015 Intel Corporation.
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

///////////////////////////////////////////////////////////////////////////
// File   : autohbw.c
// Purpose: Library to automatically allocate HBW (MCDRAM)
// Author : Ruchira Sasanka (ruchira.sasanka@intel.com)
// Date   : Jan 30, 2015
///////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <memkind.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <hbwmalloc.h>

#define FALSE 0
#define TRUE 1
#define BOOL int


// Debug level. Increase the debug level to print more messages
// 1: Info only
// 2: Messages about allocation/failures
//
static int DbgLevel = 0;

// Log level:
// 1: Logs only failures
// 2: Logs failures and allocations
//
static int LogLevel = 1;
//
enum {LOG_NONE, LOG_ALLOC, LOG_ALL};

// The low limit for automatically promoting an allocation to HBW. Allocations
// of size greater than the following will ber promoted.
//
static long int HBWLowLimit = 1 * 1024 * 1024;

// If there is a high limit specified, allocations larger than this limit
// will not be allocated in HBW. By default, it is -1, indicating no
// high limit
//
static long int HBWHighLimit = -1;

// Whether we have initialized HBW arena of memkind library -- by making
// a dummy call to it. HBW arena (and hence any memkind_* call with kind
// HBW) must NOT be used until this flag is set true.
//
static int MemkindInitDone = FALSE;

// Following is the type of HBW memory that is allocated using memkind.
// By changing this type, this library can be used to allocate other
// types of memory types (e.g., MEMKIND_HUGETLB, MEMKIND_GBTLB,
// MEMKIND_HBW_HUGETLB etc.)
//
static memkind_t HBW_Type;

// Include helper file
//
#include "autohbw_helper.h"


////////////////////////////////////////////////////////////////////////////
// This function is executed at library load time.
// Initilize HBW arena by making a dummy allocation/free at library load
// time. Until HBW initialization is complete, we must not call any
// allocation routines with HBW as kind.
////////////////////////////////////////////////////////////////////////////
void __attribute__ ((constructor)) autohbw_load(void)
{

    // First set the default memory type this library allocates. This can
    // be overridden by env variable
    // Note: 'memkind_hbw_preferred' will allow falling back to DDR but
    //       'memkind_hbw will not'
    //
    int ret = memkind_get_kind_by_name("memkind_hbw_preferred", &HBW_Type);
    assert(!ret && "FATAL: Could not find default memory type\n");

    // Read any env variables. This has to be done first because DbgLevel
    // is set using env variables and debug printing is used below
    //
    setEnvValues();                // read any env variables

    DBG(1) printf("INFO: autohbw.so loaded!\n");

    // dummy HBW call to initialize HBW arena
    //
    void *pp = memkind_malloc(HBW_Type, 16);
    //
    if (pp) {

        // We have successfully initilized HBW arena
        //
        DBG(2) printf("\t-HBW int call succeeded\n");
        memkind_free(0, pp);

        MemkindInitDone = TRUE;        // enable HBW allocation

    }
    else {
        errPrn("\t-HBW init call FAILED\n");
        assert(0 && "HBW/memkind initialization faild");
    }
}


////////////////////////////////////////////////////////////////////////////
// My malloc implementation calling memkind_malloc.
////////////////////////////////////////////////////////////////////////////
void *myMemkindMalloc(size_t size)
{

    DBG(2) printf("In my memkind malloc sz:%ld .. ", size);

    void *pp;

    // if we have not initialized memkind HBW arena yet, call default kind
    // Similarly, if the hueristic decides not to alloc in HBW, use default
    //
    if (!MemkindInitDone || !isAllocInHBW(size))
        pp = memkind_malloc(MEMKIND_DEFAULT, size);
    else {
        DBG(2) printf("\tHBW");
        pp =  memkind_malloc(HBW_Type, size);
        logHBW(pp, size);
    }

    DBG(2) printf("\tptr:%p\n", pp);

    return pp;

}


////////////////////////////////////////////////////////////////////////////
// My calloc implementation calling memkind_calloc.
////////////////////////////////////////////////////////////////////////////
void *myMemkindCalloc(size_t nmemb, size_t size)
{

    DBG(2) printf("In my memkind calloc sz:%ld ..", size*nmemb);

    //setLibcSymbols();

    void *pp;

    // if we have not initialized memkind HBW arena yet, call default kind
    //
    if (!MemkindInitDone || !isAllocInHBW(size*nmemb))
        pp = memkind_calloc(MEMKIND_DEFAULT, nmemb, size);
    else {
        DBG(2) printf("\tHBW");
        pp = memkind_calloc(HBW_Type, nmemb, size);
        logHBW(pp, size*nmemb);
    }

    DBG(2) printf("\tptr:%p\n", pp);

    return pp;

}


////////////////////////////////////////////////////////////////////////////
// My realloc implementation calling memkind_realloc
////////////////////////////////////////////////////////////////////////////
void *myMemkindRealloc(void *ptr, size_t size)
{

    DBG(2) printf("In my memkind realloc sz:%ld, p1:%p ..", size, ptr);

    void *pp;

    // if we have not initialized memkind HBW arena yet, call default kind
    //
    if (!MemkindInitDone  || !isAllocInHBW(size))
        pp = memkind_realloc(MEMKIND_DEFAULT, ptr, size);
    else {
        DBG(2) printf("\tHBW");
        pp = memkind_realloc(HBW_Type, ptr, size);
        logHBW(pp, size);
    }

    DBG(2) printf("\tptr=%p\n", pp);

    return pp;
}


////////////////////////////////////////////////////////////////////////////
// Posix alignment
////////////////////////////////////////////////////////////////////////////
int myMemkindAlign(void **memptr, size_t alignment, size_t size)
{

    DBG(2) printf("In my memkind align sz:%ld .. ", size);

    int ret;

    // if we have not initialized memkind HBW arena yet, call default kind
    // Similarly, if the hueristic decides not to alloc in HBW, use default
    //
    if (!MemkindInitDone || !isAllocInHBW(size))

        ret = memkind_posix_memalign(MEMKIND_DEFAULT, memptr, alignment, size);
    else {
        DBG(2) printf("\tHBW");
        ret = memkind_posix_memalign(HBW_Type, memptr, alignment, size);
        logHBW(*memptr, size);
    }
    DBG(2) printf("\tptr:%p\n", *memptr);

    return ret;

}


////////////////////////////////////////////////////////////////////////////
// My memkind free function calling memkind_free (with Kind=0).
// Note: memkind_free does not need the exact kind, if kind is 0. Then
//       the library can figure out the proper kind itself.
////////////////////////////////////////////////////////////////////////////
void myMemkindFree(void *ptr)
{

    DBG(2) printf("In my memkind free, ptr:%p\n", ptr);

    memkind_free(0, ptr);
}


//--------------------------------------------------------------------------
// ------------------ Section B: Interposer Functions ----------------------
//--------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
// My interposer for malloc
////////////////////////////////////////////////////////////////////////////
void *malloc(size_t size)
{

    return myMemkindMalloc(size);
}

////////////////////////////////////////////////////////////////////////////
// My interposer for calloc
////////////////////////////////////////////////////////////////////////////
void *calloc(size_t nmemb, size_t size)
{

    return myMemkindCalloc(nmemb, size);
}

////////////////////////////////////////////////////////////////////////////
// My interposer for realloc
////////////////////////////////////////////////////////////////////////////
void *realloc(void *ptr, size_t size)
{

    return myMemkindRealloc(ptr, size);
}


////////////////////////////////////////////////////////////////////////////
// My interposer for posix_memalign
////////////////////////////////////////////////////////////////////////////
int posix_memalign(void **memptr, size_t alignment, size_t size)
{

    return myMemkindAlign(memptr, alignment, size);
}


////////////////////////////////////////////////////////////////////////////
// Capture deprecated valloc. We don't allow this use of this func beause
// (1) A ptr obtained by this can be passed to free()
// (2) To warn the use of a deprecated function.
// However, if really needed we can support this method using posix_memalign
////////////////////////////////////////////////////////////////////////////
void *valloc(size_t size)
{

    fprintf(stderr, "use of deprecated valloc. Use posix_memalign instead\n");
    assert(0 && "valloc is deprecated. Not supported by this library");
}


////////////////////////////////////////////////////////////////////////////
// Capture deprecated memalign. We don't allow this use of this func beause
// (1) A ptr obtained by this can be passed to free()
// (2) To warn the use of a deprecated function.
// However, if really needed we can support this method using posix_memalign
////////////////////////////////////////////////////////////////////////////
void *memalign(size_t boundary, size_t size)
{

    fprintf(stderr, "use of deprecated memalign. Use posix_memalign instead\n");
    assert(0 && "memalign is deprecated. Not supported by this library");

}

////////////////////////////////////////////////////////////////////////////
// My interposer for free
////////////////////////////////////////////////////////////////////////////
void free(void *ptr)
{

    if (ptr)
        return myMemkindFree(ptr);
}



