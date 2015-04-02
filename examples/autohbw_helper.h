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
// File   : autohbw_helper.c
// Purpose: Helper functions (e.g., debug, env read) for AutoHBM
// Author : Ruchira Sasanka (ruchira.sasanka@intel.com)
// Date   : Jan 30, 2015
///////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Debug printing routines
////////////////////////////////////////////////////////////////////////////
#define DBG(x) if (DbgLevel >= (x))

////////////////////////////////////////////////////////////////////////////
// Error printing routine
////////////////////////////////////////////////////////////////////////////
void errPrn(const char *msg)
{
    fprintf(stderr, "%s", msg);
}


static BOOL isAutoHBMEnabled = TRUE;



////////////////////////////////////////////////////////////////////////////
// API functions to enable and disable AutoHBM
////////////////////////////////////////////////////////////////////////////
void enableAutoHBM()
{

    isAutoHBMEnabled = TRUE;
    printf("INFO: HBM allocations enabled by application (for this rank)\n");
}
//
void disableAutoHBM()
{

    isAutoHBMEnabled = FALSE;
    printf("INFO: HBM allocations disabled by application (for this rank)\n");

}


////////////////////////////////////////////////////////////////////////////
// Define the hueristic here.
////////////////////////////////////////////////////////////////////////////
BOOL isAllocInHBM(int size)
{

    if (!isAutoHBMEnabled)
        return FALSE;

    // if an upper limit is specified, and size is grearter than this upper
    // limit, return false
    //
    if ((HBMHighLimit > 0) && (size > HBMHighLimit))
        return FALSE;

    // if size is greater than the lower limit, return true
    //
    if (size > HBMLowLimit)
        return TRUE;

    return FALSE;
}






////////////////////////////////////////////////////////////////////////////
// Returns the limit in bytes using a limit value and a multiplier
// character like K, M, G
////////////////////////////////////////////////////////////////////////////
long getLimit(const unsigned long limit, const char lchar)
{

    DBG(2) printf("limit w/o moidifier: %lu\n", limit);

    // Now read the trailing character (e.g., K, M, G)
    // Based on the character, determine the multiplier
    //
    if ((limit > 0) && isalpha(lchar)) {

        int mult = 1;              // default multiplier

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
// Once HBMLowLimit (and HBMHighLimit) are set, call this method to
// inform the user about the size range of arrays that will be allocated
// in HBM
////////////////////////////////////////////////////////////////////////////
void printLimits()
{


    // Inform accoriding to the limits set
    //
    if ((HBMLowLimit >= 0) && (HBMHighLimit >= 0)) {

        // if both high and low limits are specified, we use a range
        //
        printf("INFO: Allocations between %ldK - %ldK will be allocated in HBM. "
               "Set AUTO_HBM_SIZE=X:Y to change this limit.\n",
               HBMLowLimit/1024, HBMHighLimit/1024);

    }
    else if (HBMLowLimit >= 0) {

        // if only a low limit is provided, use that
        //
        printf("INFO: Allocations greater than %ldK will be allocated in HBM. "
               "Set AUTO_HBM_SIZE=X:Y to change this limit.\n",
               HBMLowLimit/1024);

    }
    else if (HBMHighLimit >= 0) {

        // if only a high limit is provided, use that
        //
        printf("INFO: Allocations smaller than %ldK will be allocated in HBM. "
               "Set AUTO_HBM_SIZE=X:Y to change this limit.\n",
               HBMHighLimit/1024);

    }
    else {
        printf("INFO: ERROR: No threshold set. This case should not happen\n");
    }

}



////////////////////////////////////////////////////////////////////////////
// Read from the enviornment and sets global variables
// Env variables are:
//   AUTO_HBM_SIZE = gives the size for auto HBM allocation
//   AUTO_HBM_DbgLevel = sets debug level
//   AUTO_HBM_LOG = gives logging level
////////////////////////////////////////////////////////////////////////////
void setEnvValues()
{

    // STEP: Read Debug level from the env variable
    //       Note: do this first because DBG(x) macro is used below
    //
    char *debug_str = getenv("AUTO_HBM_DEBUG");
    if (debug_str && strlen(debug_str)) {
        int level = atoi(debug_str);
        if (level >= 0) {
            DbgLevel = level;
            printf("INFO: Setting debug level to %d\n", DbgLevel);
        }
    }

    // STEP: Set the size limits (thresholds) for HBM allocation
    //
    // Reads the enviornment variable
    //
    const char *size_str = getenv("AUTO_HBM_SIZE");
    //
    long lowlim = -1, highlim = -1;
    char lowC = 'K', highC = 'K';
    //
    if (size_str && strlen(size_str)) {

        sscanf(size_str, "%ld%c:%ld%c", &lowlim, &lowC, &highlim, &highC);

        DBG(1) printf("INFO:lowlim=%ld(%c), highlim=%ld(%c)\n",
                      lowlim, lowC, highlim, highC);

    }

    if (lowlim == 0 || highlim == 0) {
        printf("WARN: In AUTO_HBM_SIZE=X:Y, X or Y cannot be zero. "
               "Minimum value for X or Y must be 1.\n");
    }
    //
    // Set lower limit
    //
    if (lowlim >= 0) {
        lowlim = getLimit(lowlim, lowC);
        if (lowlim >= 0) {
            HBMLowLimit = lowlim;
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
            if (HBMLowLimit > highlim) {
                printf("WARN: In AUTO_HBM_SIZE=X:Y, Y must be greater than X. "
                       "Y value is ignored\n");
            }
            else {
                HBMHighLimit = highlim;
            }
        }
    }
    //
    // if the user did not specify any limits, inform that we are using
    // default limits
    //
    if ((lowlim < 0) && (highlim < 0)) {
        printf("INFO: Using default values for array size thesholds. "
               "Set AUTO_HBM_SIZE=X:Y to change.\n");
    }
    //
    // inform the user about limits
    //
    printLimits();


    // STEP: Read the log level from the env variable
    //
    char *log_str = getenv("AUTO_HBM_LOG");
    if (log_str && strlen(log_str)) {
        int level = atoi(log_str);
        if (level >= 0) {
            LogLevel = level;
            printf("INFO: Setting log level to %d\n", LogLevel);
        }
    }

    if (LogLevel == LOG_NONE) {
        printf("INFO: HBM allocation stats will not be printed. "
               "Set AUTO_HBM_LOG to enable.\n");
    }
    else if (LogLevel == LOG_ALLOC) {
        printf("INFO: Only HBM allocations will be printed. "
               "Set AUTO_HBM_LOG to disable/enable.\n");
    }
    else if (LogLevel == LOG_ALL) {
        printf("INFO: HBM allocation with backtrace info will be printed. "
               "Set AUTO_HBM_LOG to disable.\n");
    }


    fflush(stdout);

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
    //
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
    if (!LogLevel) return;

    // For finding out the stats about HBW pool
    //
    size_t btotal, bfree;



    if (p) {

        if (LogLevel >= LOG_ALL) {

            // Allocation was successful AND logging of allocations requested
            //
            const int buf_sz = 128;
            char msg_buf[buf_sz];

            memkind_get_size(MEMKIND_HBW, &btotal, &bfree);
            printBackTrace(msg_buf, buf_sz);
            printf("Log: allocated HBW bytes: %ld (%ld free) \tBacktrace:%s\n",
                   size, bfree, msg_buf);


        }
        else if (LogLevel >= LOG_ALLOC) {

            // Allocation was NOT succesful AND logging of failures requested
            //
            memkind_get_size(MEMKIND_HBW, &btotal, &bfree);
            printf("Log: allocated HBW bytes: %ld (%ld free)\n",
                   size, bfree);

        }

    }
    else {


        // Allocation was NOT succesful AND logging of failures requested
        // This should not really happen because we are using PREFERRED policy
        //
        memkind_get_size(MEMKIND_HBW, &btotal, &bfree);
        printf("Log: HBW -- call did NOT succeed: Requested:%ld, free:%ld\n",
               size, bfree);
    }

}





