/*
 * Copyright (C) 2019 Intel Corporation.
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

#include <memkind/internal/memkind_dax_kmem.h>
#include <memkind.h>
#include <numa.h>
#include <stdio.h>

#define MAX_ARG_LEN 8

static const char *const help_message =
    "\n"
    "NAME\n"
    "    memkind-auto-dax-kmem-nodes - prints comma-separated list of automatically detected persistent memory NUMA nodes.\n"
    "\n"
    "SYNOPSIS\n"
    "    memkind-auto-dax-kmem-nodes -h | --help\n"
    "        Prints this help message.\n"
    "\n"
    "DESCRIPTION\n"
    "    Prints a comma-separated list of automatically detected persistent memory NUMA nodes\n"
    "    that can be used with the numactl --membind option.\n"
    "\n"
    "EXIT STATUS\n"
    "    Return code is :\n"
    "        0 on success\n"
    "        1 if automatic detection is disabled (memkind wasn't build with suitable libdaxctl-devel version)\n"
    "        2 on invalid argument\n"
    "        3 on other failure\n"
    "\n"
    "COPYRIGHT\n"
    "    Copyright 2019 Intel Corporation All Rights Reserved.\n"
    "\n"
    "AUTHORS\n"
    "    Michal Biesek\n"
    "\n"
    "SEE ALSO\n"
    "    memkind(3)\n"
    "\n";


static int print_dax_kmem_nodes()
{
    unsigned i, j = 0;

    nodemask_t nodemask;
    struct bitmask nodemask_dax_kmem = {NUMA_NUM_NODES, nodemask.n};

    // ensuring functions in numa library are defined
    if (numa_available() == -1) {
        return 3;
    }
    numa_bitmask_clearall(&nodemask_dax_kmem);

    //WARNING: code below is usage of memkind experimental API which may be changed in future
    int status = memkind_dax_kmem_all_get_mbind_nodemask(NULL, nodemask.n,
                                                         NUMA_NUM_NODES);

    if (status == MEMKIND_ERROR_OPERATION_FAILED ) {
        return 1;
    } else if (status != MEMKIND_SUCCESS ) {
        return 3;
    }

    for(i = 0; i<NUMA_NUM_NODES; i++) {
        if(numa_bitmask_isbitset(&nodemask_dax_kmem, i)) {
            printf("%u%s", i, (++j == numa_bitmask_weight(&nodemask_dax_kmem)) ? "" : ",");
        }
    }
    printf("\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc == 1) {
        return print_dax_kmem_nodes();
    } else if ((argc == 2) && (strncmp(argv[1], "-h", MAX_ARG_LEN) == 0 ||
                               strncmp(argv[1], "--help", MAX_ARG_LEN) == 0)) {
        printf("%s", help_message);
        return 0;
    }

    printf("ERROR: Unknown option %s. More info with \"%s --help\".\n", argv[1],
           argv[0]);
    return 2;
}
