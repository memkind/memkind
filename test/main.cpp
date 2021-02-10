// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include "common.h"

char *PMEM_DIR = const_cast<char *>("/tmp/");

int main(int argc, char **argv)
{
    int opt = 0;
    struct stat st;

    testing::InitGoogleTest(&argc, argv);

    if (testing::GetGtestHelpFlag()) {
        printf("\nMemkind options:\n"
               "-d <directory_path>   change directory on which PMEM kinds\n"
               "                      are created (default /tmp/)\n");
        return 0;
    }

    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
            case 'd':
                PMEM_DIR = optarg;

                if (stat(PMEM_DIR, &st) != 0 || !S_ISDIR(st.st_mode)) {
                    printf("%s : Error in getting path status or"
                           " invalid or non-existent directory\n",
                           PMEM_DIR);
                    return -1;
                }

                break;
            default:
                return -1;
        }
    }

    return RUN_ALL_TESTS();
}
