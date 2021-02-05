// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2021 Intel Corporation. */

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
    "    Copyright 2016 - 2021 Intel Corporation All Rights Reserved.\n"
    "\n"
    "AUTHORS\n"
    "    Krzysztof Kulakowski\n"
    "\n"
    "SEE ALSO\n"
    "    hbwmalloc(3), memkind(3)\n"
    "\n";

extern unsigned int numa_bitmask_weight(const struct bitmask *bmp);

int print_hbw_nodes()
{
    int i, j, status, max_node_id;

    nodemask_t nodemask;
    struct bitmask nodemask_bm = {NUMA_NUM_NODES, nodemask.n};
    struct bitmask *nodemask_all_bm;
    struct bitmask *node_cpumask;

    // ensuring functions in numa library are defined
    if (numa_available() == -1) {
        return 1;
    }

    max_node_id = numa_max_node();

    node_cpumask = numa_allocate_cpumask();
    if (!node_cpumask) {
        return 1;
    }

    nodemask_all_bm = numa_allocate_nodemask();
    if (!nodemask_all_bm) {
        status = 1;
        goto free_cpumask;
        ;
    }

    for (i = 0; i <= max_node_id; ++i) {
        numa_bitmask_clearall(&nodemask_bm);

        if ((numa_node_to_cpus(i, node_cpumask) != 0) ||
            numa_bitmask_weight(node_cpumask) == 0) {
            continue;
        }
        int ret = numa_run_on_node(i);
        if (ret) {
            status = 1;
            goto free_all_bm;
        }
        // WARNING: code below is usage of memkind experimental API which may be
        // changed in future
        if (memkind_hbw_all_get_mbind_nodemask(NULL, nodemask.n,
                                               NUMA_NUM_NODES) != 0) {
            status = 1;
            goto free_all_bm;
        }
        for (j = 0; j <= max_node_id; ++j) {
            if (numa_bitmask_isbitset(&nodemask_bm, j)) {
                numa_bitmask_setbit(nodemask_all_bm, j);
            }
        }
    }

    j = 0;

    for (i = 0; i < NUMA_NUM_NODES; ++i) {
        if (numa_bitmask_isbitset(nodemask_all_bm, i)) {
            printf("%d%s", i,
                   (++j == numa_bitmask_weight(nodemask_all_bm)) ? "" : ",");
        }
    }
    printf("\n");
    status = 0;

free_all_bm:
    numa_bitmask_free(nodemask_all_bm);

free_cpumask:
    numa_bitmask_free(node_cpumask);
    return status;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        return print_hbw_nodes();
    } else if ((argc == 2) &&
               (strncmp(argv[1], "-h", MAX_ARG_LEN) == 0 ||
                strncmp(argv[1], "--help", MAX_ARG_LEN) == 0)) {
        printf("%s", help_message);
        return 2;
    }

    printf("ERROR: Unknown option %s. More info with \"%s --help\".\n", argv[1],
           argv[0]);
    return 2;
}
