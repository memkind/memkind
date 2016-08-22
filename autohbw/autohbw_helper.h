/*
 * Copyright (C) 2015 - 2016 Intel Corporation.
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
// File   : autohbw_helper.h
// Purpose: Helper functions (e.g., logging, env read) for AutoHBW
// Author : Ruchira Sasanka (ruchira DOT sasanka AT intel DOT com)
// Date   : Jan 30, 2015
///////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Logging helper macro
////////////////////////////////////////////////////////////////////////////
#define LOG(x) if (LogLevel >= (x))

////////////////////////////////////////////////////////////////////////////
// Error printing routine
////////////////////////////////////////////////////////////////////////////
void errPrn(const char *msg)
{
    fprintf(stderr, "%s", msg);
}

static BOOL isAutoHBWEnabled = TRUE;

////////////////////////////////////////////////////////////////////////////
// API functions to enable and disable AutoHBW
////////////////////////////////////////////////////////////////////////////
AUTOHBW_EXPORT void enableAutoHBW()
{
    isAutoHBWEnabled = TRUE;
    LOG(LOG_INFO) fprintf(LogF,
        "INFO: HBW allocations enabled by application "
        "(for this rank)\n");
}

AUTOHBW_EXPORT void disableAutoHBW()
{
    isAutoHBWEnabled = FALSE;
    LOG(LOG_INFO) fprintf(LogF,
        "INFO: HBW allocations disabled by application "
        "(for this rank)\n");
}

////////////////////////////////////////////////////////////////////////////
// Define the hueristic here.
////////////////////////////////////////////////////////////////////////////
BOOL isAllocInHBW(size_t size)
{
    if (!isAutoHBWEnabled)
        return FALSE;

    // if an upper limit is specified, and size is grearter than this upper
    // limit, return false
    //
    if ((HBWHighLimit > 0) && (size > HBWHighLimit))
        return FALSE;

    // if size is greater than the lower limit, return true
    //
    if (size > HBWLowLimit)
        return TRUE;

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
// Returns the limit in bytes using a limit value and a multiplier
// character like K, M, G
////////////////////////////////////////////////////////////////////////////
long getLimit(const unsigned long limit, const char lchar)
{
    LOG(LOG_ALL) fprintf(LogF, "limit w/o moidifier: %lu\n", limit);

    // Now read the trailing character (e.g., K, M, G)
    // Based on the character, determine the multiplier
    //
    if ((limit > 0) && isalpha(lchar)) {
        long mult = 1;              // default multiplier

        if (toupper(lchar) == 'K') mult = 1024;
        else if (toupper(lchar) == 'M') mult = 1024 * 1024;
        else if (toupper(lchar) == 'G') mult = 1024 * 1024 * 1024;

        return limit * mult;
    }
    // on error, return -1
    //
    return -1;
}

////////////////////////////////////////////////////////////////////////////
// Once HBWLowLimit (and HBWHighLimit) are set, call this method to
// inform the user about the size range of arrays that will be allocated
// in HBW
////////////////////////////////////////////////////////////////////////////
void printLimits()
{
    // Inform accoriding to the limits set
    //
    if ((HBWLowLimit >= 0) && (HBWHighLimit >= 0)) {
        // if both high and low limits are specified, we use a range
        //
        fprintf(LogF,
            "INFO: Allocations between %ldK - %ldK will be allocated in "
            "HBW. Set AUTO_HBW_SIZE=X:Y to change this limit.\n",
            HBWLowLimit/1024, HBWHighLimit/1024);
    }
    else if (HBWLowLimit >= 0) {
        // if only a low limit is provided, use that
        //
        fprintf(LogF,
            "INFO: Allocations greater than %ldK will be allocated in HBW."
            " Set AUTO_HBW_SIZE=X:Y to change this limit.\n",
            HBWLowLimit/1024);
    }
    else if (HBWHighLimit >= 0) {
        // if only a high limit is provided, use that
        //
        fprintf(LogF,
            "INFO: Allocations smaller than %ldK will be allocated in HBW. "
            "Set AUTO_HBW_SIZE=X:Y to change this limit.\n",
            HBWHighLimit/1024);
    }
    else {
      fprintf(LogF,
            "INFO: ERROR: No threshold set. This case should not happen\n");
    }
}

////////////////////////////////////////////////////////////////////////////
// Read from the enviornment and sets global variables
// Env variables are:
//   AUTO_HBW_SIZE = gives the size for auto HBW allocation
//   AUTO_HBW_LOG = gives logging level
////////////////////////////////////////////////////////////////////////////
void setEnvValues()
{
    // STEP: Read the log level from the env variable. Do this early because
    //       priting depends on this
    //
    char *log_str = getenv("AUTO_HBW_LOG");
    if (log_str && strlen(log_str)) {
        int level = atoi(log_str);
        if (level >= -1) {             // LogLevel can be negative
            LogLevel = level;
            fprintf(LogF, "INFO: Setting log level to %d\n", LogLevel);
        }
    }

    if (LogLevel == LOG_INFO) {
        fprintf(LogF, "INFO: HBW allocation stats will not be printed. "
            "Set AUTO_HBW_LOG to enable.\n");
    }
    else if (LogLevel == LOG_ALLOC) {
        fprintf(LogF,
            "INFO: Only HBW allocations will be printed. "
            "Set AUTO_HBW_LOG to disable/enable.\n");
    }
    else if (LogLevel == LOG_ALL) {
        fprintf(LogF,
            "INFO: HBW allocation with backtrace info will be printed. "
            "Set AUTO_HBW_LOG to disable.\n");
    }

    // Set the memory type allocated by this library. By default, it is
    // MEMKIND_HBW, but we can use this library to allocate other memory
    // types
    //
    const char *memtype_str = getenv("AUTO_HBW_MEM_TYPE");
    struct list_of_kind_struct {
        memkind_t   handle;
        char* name;
    } ;

    struct list_of_kind_struct kinds_list[] = {
        { MEMKIND_DEFAULT, "memkind_default" },
        { MEMKIND_HBW, "memkind_hbw" },
        { MEMKIND_HBW_HUGETLB, "memkind_hbw_hugetlb" },
        { MEMKIND_HBW_PREFERRED, "memkind_hbw_preferred" },
        { MEMKIND_HBW_PREFERRED_HUGETLB, "memkind_hbw_preferred_hugetlb" },
        { MEMKIND_HUGETLB, "memkind_hugetlb" },
        { MEMKIND_HBW_GBTLB, "memkind_hbw_gbtlb" },
        { MEMKIND_HBW_PREFERRED_GBTLB, "memkind_hbw_preferred_gbtlb" },
        { MEMKIND_GBTLB, "memkind_gbtlb" },
        { MEMKIND_HBW_INTERLEAVE, "memkind_hbw_interleave" },
        { MEMKIND_INTERLEAVE, "memkind_interleave" },
    };

    if (memtype_str && strlen(memtype_str)) {
        const int BLEN=80, slen = strlen(memtype_str);
        char buf[BLEN];
        //
        // Convert to all lower case because memkind API names user lower case
        //
        int i=0;
        for (i=0; (i < BLEN-1) && (i < slen); i++) {
            buf[i] = tolower(memtype_str[i]);
        }
        buf[i] = 0;
        //
        // Find the memkind_t using the name the user has provided in the env
        // variable
        //
        memkind_t mty = NULL;
        for (i=0; i < (sizeof(kinds_list)/sizeof(kinds_list[0])); i++) {
            if (strcmp(buf, kinds_list[i].name) == 0) {
                mty = kinds_list[i].handle;
                break;
            }
        }
        if (mty != NULL) {
            HBW_Type = mty;
            LOG(LOG_INFO) fprintf(LogF,
                "INFO: Setting HBW memory type to %s\n",
                memtype_str);
        }
        else {
            fprintf(LogF,
                "WARN: Memory type %s not recognized. Using default type\n",
                memtype_str);
        }
    }

    // STEP: Set the size limits (thresholds) for HBW allocation
    //
    // Reads the enviornment variable
    //
    const char *size_str = getenv("AUTO_HBW_SIZE");
    long lowlim = -1, highlim = -1;
    char lowC = 'K', highC = 'K';

    if (size_str && strlen(size_str)) {
        sscanf(size_str, "%ld%c:%ld%c", &lowlim, &lowC, &highlim, &highC);
        LOG(LOG_ALLOC) fprintf(LogF,
            "INFO:lowlim=%ld(%c), highlim=%ld(%c)\n",
            lowlim, lowC, highlim, highC);
    }
    if (lowlim == 0 || highlim == 0) {
        fprintf(LogF,
            "WARN: In AUTO_HBW_SIZE=X:Y, X or Y cannot be zero. "
            "Minimum value for X or Y must be 1.\n");
    }
    //
    // Set lower limit
    //
    if (lowlim >= 0) {
        lowlim = getLimit(lowlim, lowC);
        if (lowlim >= 0) {
            HBWLowLimit = lowlim;
        }
    }
    //
    // Set upper limit
    //
    if (highlim >= 0) {
        highlim = getLimit(highlim, highC);
        if (highlim >= 0) {
            // if the lower limit is greater than the upper limit, warn
            //
            if (HBWLowLimit > highlim) {
                fprintf(LogF,
                    "WARN: In AUTO_HBW_SIZE=X:Y, Y must be greater than X. "
                    "Y value is ignored\n");
            }
            else {
                HBWHighLimit = highlim;
            }
        }
    }
    //
    // if the user did not specify any limits, inform that we are using
    // default limits
    //
    if ((lowlim < 0) && (highlim < 0)) {
        LOG(LOG_INFO) fprintf(LogF,
            "INFO: Using default values for array size thresholds. "
            "Set AUTO_HBW_SIZE=X:Y to change.\n");
    }
    //
    // inform the user about limits
    //
    LOG(LOG_INFO) printLimits();

    fflush(LogF);
}

// Required for backtrace
//
#include <execinfo.h>

////////////////////////////////////////////////////////////////////////////
// Prints a backtrace pointers. See 'man backtrace' for more info.
// The pointers printed by this back trace can be resolved to source lines
// using addr2line Linux utility
////////////////////////////////////////////////////////////////////////////
void printBackTrace(char *msg_buf, int buf_sz)
{
    // Number of ptrs to be returned by backtrace, and buffer size for that
    //
    const int bt_size = 10;
    const int bt_buf_size = 1000;

    // Backtrace frame pointers will be deposited here
    //
    void * buffer[bt_buf_size];

    // Call backtrace to get the backtrace of pointers
    //
    int ptr_cnt = backtrace(buffer, bt_size);

    // Start backtrace from this pointer only. The very first pointers
    // point to stack addresses of functions in this library -- which is
    // not useful to the user. So, skip them.
    //
    int prog_stack_start = 4;
    int i = prog_stack_start;

    // Print releavant backtrace pointers
    //
    for ( ; (i < ptr_cnt) && (i < prog_stack_start + 3); i++) {
        snprintf(msg_buf, buf_sz, "[%d]:%p ", i-prog_stack_start, buffer[i]);

        int len = strlen(msg_buf);
        msg_buf += len;
        buf_sz -= len;
    }
}

////////////////////////////////////////////////////////////////////////////
// Log HBW allocation
////////////////////////////////////////////////////////////////////////////
void logHBW(void *p, size_t size)
{
    // Do not log if if logging is turned off
    //
    if (LogLevel) return;

    // For finding out the stats about HBW pool
    //
    size_t btotal, bfree;

    if (p) {
        if (LogLevel >= LOG_ALL) {
            // Allocation was successful AND logging of allocations requested
            //
            const int buf_sz = 128;
            char msg_buf[buf_sz];

            memkind_get_size(HBW_Type, &btotal, &bfree);
            printBackTrace(msg_buf, buf_sz);
            fprintf(LogF,
                "Log: allocated HBW bytes: %ld (%ld free) "
                "\tBacktrace:%s\n",
                size, bfree, msg_buf);
        }
        else if (LogLevel >= LOG_ALLOC) {
            // Allocation was NOT succesful AND logging of failures requested
            //
            memkind_get_size(HBW_Type, &btotal, &bfree);
            fprintf(LogF, "Log: allocated HBW bytes: %ld (%ld free)\n",
                size, bfree);
        }
    }
    else {
        // Allocation was NOT succesful AND logging of failures requested
        // This should not really happen because we are using PREFERRED policy
        //
        memkind_get_size(HBW_Type, &btotal, &bfree);
        fprintf(LogF,
            "Log: HBW -- call did NOT succeed: Requested:%ld, free:%ld\n",
            size, bfree);
    }
}

