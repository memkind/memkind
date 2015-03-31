/*
 * Copyright (C) 2014, 2015 Intel Corporation.
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

#include "common.h"

int hexDump2Bin(const char*);
int parse_node_bandwidth(size_t, int*,const char*);
int run_pmtt_parser(const char*,const int[], size_t);

static const char *MOCK_PMTT_PATH = "/tmp/mock-pmtt.aml";

class MemkindPmttTest: public :: testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {
        //Delete generated files
    }

};


TEST_F(MemkindPmttTest, TC_PmttParser_001)
{
    size_t NUMA_NUM_NODES = 2;
    const char *mockPmtt = "/tmp/mock-pmtt.txt";
    int rv = 0;

    //Known bandwidths from MOCK PMTT table.
    const int MEM_CTRLS_BW[2] = {17072, 760800};

    rv = run_pmtt_parser(mockPmtt, MEM_CTRLS_BW, NUMA_NUM_NODES);
    EXPECT_EQ(0, rv);

}

TEST_F(MemkindPmttTest, TC_PmttParser_002)
{
    size_t NUMA_NUM_NODES = 2;
    const char *mockPmtt = "/tmp/mock-pmtt2.txt";
    int rv = 0;

    //Known bandwidths from MOCK PMTT table.
    const int MEM_CTRLS_BW[2] = {8536, 204800};

    rv = run_pmtt_parser(mockPmtt, MEM_CTRLS_BW, NUMA_NUM_NODES);
    EXPECT_EQ(0, rv);

}

int run_pmtt_parser(const char *mockPmtt, const int MEM_CTRLS_BW[],
                    size_t NUMA_NUM_NODES)
{
    int rv = 0;
    static const char *MOCK_NBW_PATH = "/tmp/node-bandwidth";
    char pmtt_parser_exe_path[64] = "/usr/sbin/memkind-pmtt";
    char pmtt_parser_exe[256];
    int bandwidth[NUMA_NUM_NODES];

    rv = hexDump2Bin(mockPmtt);
    EXPECT_EQ(0, rv);

    if(FILE *file = fopen(pmtt_parser_exe_path, "r")) {
        fclose(file);
    }
    else {
        strcpy(pmtt_parser_exe_path,"./memkind-pmtt");
    }

    snprintf(pmtt_parser_exe,sizeof(pmtt_parser_exe),"%s %s %s",
             pmtt_parser_exe_path, MOCK_PMTT_PATH, MOCK_NBW_PATH);

    printf("Running memkind_pmtt with args: %s\n", pmtt_parser_exe);
    rv = system(pmtt_parser_exe);
    EXPECT_EQ(0, rv);

    rv = parse_node_bandwidth(NUMA_NUM_NODES, bandwidth, MOCK_NBW_PATH);
    EXPECT_EQ(0, rv);

    for(size_t i=0; i < NUMA_NUM_NODES; i++) {
        EXPECT_EQ(bandwidth[i], MEM_CTRLS_BW[i]);
    }

    remove(MOCK_PMTT_PATH);
    remove(MOCK_NBW_PATH);

    return rv;
}

int parse_node_bandwidth(size_t num_bandwidth, int *bandwidth,
                         const char *bandwidth_path)
{
    FILE *fid = NULL;
    size_t nread = 0;
    int err = 0;
    fid = fopen(bandwidth_path, "r");
    if (fid == NULL) {
        return -1;
    }
    nread = fread(bandwidth, sizeof(int), num_bandwidth, fid);
    if (nread != num_bandwidth) {
        fclose(fid);
        return -1;
    }

    return err;
}


int hexDump2Bin(const char *hexDumpFile)
{
    char cmd[128];
    int rv = 0;
    snprintf(cmd,sizeof(cmd),"xxd -r %s %s",hexDumpFile, MOCK_PMTT_PATH);
    rv = system(cmd);
    return rv;
}
