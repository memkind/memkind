// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2019 Intel Corporation. */

#include <memkind/internal/memkind_hbw.h>

#include <numa.h>
#include <stdio.h>

#define MAX_ARG_LEN 8

const char *help_message =
    "\n"
    "NAME\n"
    "    memkind-hbw-nodes - Prints comma-separated list of high bandwidth nodes.\n"
    "\n"
    "SYNOPSIS\n"
    "    memkind-hbw-nodes -h | --help\n"
    "        Prints this help message.\n"
    "\n"
    "DESCRIPTION\n"
    "    Prints a comma-separated list of high bandwidth NUMA nodes\n"
    "    that can be used with the numactl --membind option.\n"
    "\n"
    "EXIT STATUS\n"
    "    Return code is :\n"
    "        0 on success\n"
    "        1 on failure\n"
    "        2 on invalid argument\n"
    "\n"
    "COPYRIGHT\n"
    "    Copyright 2016 Intel Corporation All Rights Reserved.\n"
    "\n"
    "AUTHORS\n"
    "    Krzysztof Kulakowski\n"
    "\n"
    "SEE ALSO\n"
    "    hbwmalloc(3), memkind(3)\n"
    "\n";

extern unsigned int numa_bitmask_weight(const struct bitmask *bmp );

int print_hbw_nodes()
{
    int i, j = 0;

    nodemask_t nodemask;
    struct bitmask nodemask_bm = {NUMA_NUM_NODES, nodemask.n};

    // ensuring functions in numa library are defined
    if (numa_available() == -1) {
        return 3;
    }
    numa_bitmask_clearall(&nodemask_bm);

    //WARNING: code below is usage of memkind experimental API which may be changed in future
    if(memkind_hbw_all_get_mbind_nodemask(NULL, nodemask.n, NUMA_NUM_NODES) != 0) {
        return 1;
    }

    for(i=0; i<NUMA_NUM_NODES; i++) {
        if(numa_bitmask_isbitset(&nodemask_bm, i)) {
            printf("%d%s", i, (++j == numa_bitmask_weight(&nodemask_bm)) ? "" : ",");
        }
    }
    printf("\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc == 1) {
        return print_hbw_nodes();
    } else if ((argc == 2) && (strncmp(argv[1], "-h", MAX_ARG_LEN) == 0 ||
                               strncmp(argv[1], "--help", MAX_ARG_LEN) == 0)) {
        printf("%s", help_message);
        return 2;
    }

    printf("ERROR: Unknown option %s. More info with \"%s --help\".\n", argv[1],
           argv[0]);
    return 2;
}
