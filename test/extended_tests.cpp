/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
 */

#include <fstream>
#include <algorithm>
#include "common.h"
#include "check.h"
#include "omp.h"
#include "numakind.h"
#include "trial_generator.h"


class EXTendedTest : public :: testing :: Test
{
protected:
    size_t num_bandwidth;
    int *bandwidth;
    TrialGenerator *tgen;

    void SetUp()
    {
        size_t node;
        char *hbw_nodes_env, *endptr;
        tgen = new TrialGenerator();

        hbw_nodes_env = getenv("NUMAKIND_HBW_NODES");
        if (hbw_nodes_env) {
            num_bandwidth = 128;
            bandwidth = new int[num_bandwidth];
            for (node = 0; node < num_bandwidth; node++) {
                bandwidth[node] = 1;
            }
            node = strtol(hbw_nodes_env, &endptr, 10);
            bandwidth[node] = 2;
            while (*endptr == ':') {
                hbw_nodes_env = endptr + 1;
                node = strtol(hbw_nodes_env, &endptr, 10);
                if (endptr != hbw_nodes_env && node >= 0 && node < num_bandwidth) {
                    bandwidth[node] = 2;
                }
            }
        }
        else {
            const char *node_bandwidth_path = "/etc/numakind/node-bandwidth";
            std::ifstream nbw_file;

            nbw_file.open(node_bandwidth_path, std::ifstream::binary);
            nbw_file.seekg(0, nbw_file.end);
            num_bandwidth = nbw_file.tellg()/sizeof(int);
            nbw_file.seekg(0, nbw_file.beg);
            bandwidth = new int[num_bandwidth];
            nbw_file.read((char *)bandwidth, num_bandwidth*sizeof(int));
            nbw_file.close();
        }

    }

    void TearDown()
    {
        delete[] bandwidth;
        delete tgen;
    }
};


TEST_F(EXTendedTest, hbw_malloc_1KB_2GB_sizes)
{
    tgen->generate_size_1KB_2GB(HBW_MALLOC);
    tgen->run(num_bandwidth,
              bandwidth);
}

TEST_F(EXTendedTest, hbw_realloc_1KB_2GB_sizes)
{
    tgen->generate_size_1KB_2GB(HBW_REALLOC);
    tgen->run(num_bandwidth,
              bandwidth);
}


TEST_F(EXTendedTest, hbw_calloc_1KB_2GB_sizes)
{
    tgen->generate_size_1KB_2GB(HBW_CALLOC);
    tgen->run(num_bandwidth,
              bandwidth);
}

TEST_F(EXTendedTest, hbw_memalign_1KB_2GB_sizes)
{
    tgen->generate_size_1KB_2GB(HBW_MEMALIGN);
    tgen->run(num_bandwidth,
              bandwidth);
}


TEST_F(EXTendedTest, hbw_memalign_psize_1KB_2GB_sizes)
{
    tgen->generate_size_1KB_2GB(HBW_MEMALIGN_PSIZE);
    tgen->run(num_bandwidth,
              bandwidth);
}
