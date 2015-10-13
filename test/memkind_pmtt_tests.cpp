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
#include <unistd.h>
#include <libgen.h>

#include "common.h"

class MemkindPmttTest: public :: testing::Test
{
public:
    char cwd[1024];
    int hex_dump_to_bin(const char*, const char*);
    int parse_node_bandwidth(size_t, int*,const char*);
    int run_pmtt_parser(char*,const int[], size_t);
protected:
    void SetUp()
    {
        /* The setup will provide a current working directory to where
        the .hex mock files are placed which is the location of the
        test binary */
        realpath("/proc/self/exe", cwd);
        dirname(cwd);
        strncat(cwd, "/", 1);
    }

    void TearDown()
    {
        //Delete generated files
    }

};


TEST_F(MemkindPmttTest, TC_Memkind_PmttParser_2NodeSystem)
{
    size_t NUMA_NUM_NODES = 2;
    char mock_hex[] = "mock-pmtt-2-nodes.hex";
    //Known bandwidths from MOCK PMTT table.
    const int MEM_CTRLS_BW[2] = {17072, 760800};
    int rv = 0;

    rv = run_pmtt_parser(mock_hex, MEM_CTRLS_BW, NUMA_NUM_NODES);
    EXPECT_EQ(0, rv);

}

TEST_F(MemkindPmttTest, TC_Memkind_PmttParser_EmptyController)
{
    size_t NUMA_NUM_NODES = 2;
    char mock_hex[] = "mock-pmtt-empty-controller.hex";
    //Known bandwidths from MOCK PMTT table.
    const int MEM_CTRLS_BW[2] = {8536, 204800};
    int rv = 0;

    rv = run_pmtt_parser(mock_hex, MEM_CTRLS_BW,NUMA_NUM_NODES);
    EXPECT_EQ(0, rv);

}

int MemkindPmttTest::run_pmtt_parser(char *mock_hex, const int MEM_CTRLS_BW[],
                                     size_t NUMA_NUM_NODES)
{
    int rv = 0;
    static const char *MOCK_NBW_PATH = "/tmp/node-bandwidth";
    char pmtt_parser_exe_path[] = "/usr/sbin/memkind-pmtt";
    char tmp_mock_dir[256] = "/tmp/";
    char pmtt_parser_exe[256];
    int bandwidth[NUMA_NUM_NODES];
    char *mock_aml;
    char mock_aml_ext[] = ".aml";
    char *mock_pmtt_hex;
    char tmp_cwd[1024];

    strncpy(tmp_cwd, cwd, sizeof(tmp_cwd));
    mock_pmtt_hex = strncat(tmp_cwd, mock_hex, sizeof(cwd));
    mock_aml = strncat(mock_hex, mock_aml_ext, sizeof(cwd));
    strncat(tmp_mock_dir, mock_aml, sizeof(cwd));

    rv = hex_dump_to_bin(mock_pmtt_hex, tmp_mock_dir);
    EXPECT_EQ(0, rv);
    if(FILE *file = fopen(pmtt_parser_exe_path, "r")) {
        fclose(file);
    }
    else {
        strcpy(pmtt_parser_exe_path,"./memkind-pmtt");
    }

    snprintf(pmtt_parser_exe, sizeof(pmtt_parser_exe), "%s %s %s",
             pmtt_parser_exe_path, tmp_mock_dir, MOCK_NBW_PATH);

    fprintf(stdout, "Running memkind_pmtt with args: %s\n", pmtt_parser_exe);
    rv = system(pmtt_parser_exe);
    EXPECT_EQ(0, rv);

    rv = parse_node_bandwidth(NUMA_NUM_NODES, bandwidth, MOCK_NBW_PATH);
    EXPECT_EQ(0, rv);

    for(size_t i=0; i < NUMA_NUM_NODES; i++) {
        EXPECT_EQ(bandwidth[i], MEM_CTRLS_BW[i]);
    }

    remove(tmp_mock_dir);
    remove(MOCK_NBW_PATH);

    return rv;
}

int MemkindPmttTest::parse_node_bandwidth(size_t num_bandwidth, int *bandwidth,
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


int MemkindPmttTest::hex_dump_to_bin(const char *hex_dump_file, const char *mock_pmtt_path)
{
    char cmd[256];
    int rv = 0;
    snprintf(cmd, sizeof(cmd), "/usr/bin/sudo /usr/bin/xxd -r %s %s", hex_dump_file, mock_pmtt_path);
    rv = system(cmd);
    return rv;
}
