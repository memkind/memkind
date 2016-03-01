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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <libgen.h>
#include <sys/syscall.h>
#include <bits/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>


enum {STRLEN = 512};
// The include file actbl.h comes from acpica which is available from this URL:
// https://acpica.org/sites/acpica/files/acpica-unix-20130517.tar.gz
//
#define ACPI_MACHINE_WIDTH 64
#define COMPILER_DEPENDENT_UINT64 unsigned long
#define COMPILER_DEPENDENT_INT64 long
#include "actypes.h"
#include "actbl.h"

#include <memkind.h>
#include <memkind/internal/memkind_hbw.h>

static const char *PMTT_PATH = "/sys/firmware/acpi/tables/PMTT";

struct memctlr_t {
    ACPI_PMTT_CONTROLLER memctlr;
    ACPI_PMTT_DOMAIN domain[1];
};

static int parse_pmtt_memory_controllers(int num_bandwidth, int *bandwidth,
        ACPI_PMTT_HEADER *buf, int size);

static int parse_pmtt_one_memory_controller(int num_bandwidth, int *bandwidth,
        ACPI_PMTT_HEADER *buf_in, int *bytes_remaining);


static int parse_pmtt_bandwidth(int num_bandwidth, int *bandwidth,
                                const char *pmtt_path)
{
    /***************************************************************************
    *   num_bandwidth (IN):                                                    *
    *       Length of bandwidth vector and maximum number of numa nodes.       *
    *   bandwidth (OUT):                                                       *
    *       Vector giving bandwidth for all numa nodes.  If numa bandwidth is  *
    *       not discovered the value is set to zero.                           *
    *   pmtt_path (IN):                                                        *
    *       Path to PMTT table to be parsed.                                   *
    *   RETURNS zero on success, error code on failure                         *
    ***************************************************************************/
    const size_t PMTT_BUF_SIZE = 2000;
    int err = 0;
    FILE *mfp = NULL;
    ACPI_TABLE_PMTT hdr;
    unsigned char buf[PMTT_BUF_SIZE];
    ACPI_PMTT_HEADER *pbuf = (ACPI_PMTT_HEADER *)&buf;
    size_t size;
    size_t nread;
    size_t pmtt_socket_size;

    memset(bandwidth, 0, sizeof(int)*num_bandwidth);

    if (numa_available() == -1) {
        err = MEMKIND_ERROR_PMTT;
        goto exit;
    }
    mfp = fopen(pmtt_path, "r");
    if (mfp == NULL) {
        err = MEMKIND_ERROR_PMTT;
        goto exit;
    }
    nread = fread(&hdr, sizeof(ACPI_TABLE_PMTT), 1, mfp);
    if (nread != 1 ||
        memcmp(hdr.Header.Signature, "PMTT", 4) != 0) {
        /* PMTT signature failure */
        err = MEMKIND_ERROR_PMTT;
        goto exit;
    }
    size = hdr.Header.Length - sizeof(ACPI_TABLE_PMTT);
    if (size > PMTT_BUF_SIZE) {
        /* PMTT byte count failure */
        err = MEMKIND_ERROR_PMTT;
        goto exit;
    }
    nread = fread(buf, size, 1, mfp);
    if (nread != 1 || fgetc(mfp) != EOF) {
        /* PMTT incorrect number of bytes read */
        err = MEMKIND_ERROR_PMTT;
        goto exit;
    }

    if (pbuf->Type != ACPI_PMTT_TYPE_SOCKET) { /* SOCKET */
        /* PMTT did not find socket record first */
        err = MEMKIND_ERROR_PMTT;
        goto exit;
    }
    pmtt_socket_size = pbuf->Length;

    if (pmtt_socket_size != size) {
        /* PMTT extra bytes after socket record */
        err = MEMKIND_ERROR_PMTT;
        goto exit;
    }

    if (parse_pmtt_memory_controllers(num_bandwidth, bandwidth,
                                      (ACPI_PMTT_HEADER *)&buf[sizeof(ACPI_PMTT_SOCKET)],
                                      pmtt_socket_size - sizeof(ACPI_PMTT_SOCKET))) {
        err = MEMKIND_ERROR_PMTT;
        goto exit;
    }

exit:
    if (mfp != NULL) {
        fclose(mfp);
    }
    return err;
}

static int parse_pmtt_memory_controllers(int num_bandwidth, int *bandwidth,
        ACPI_PMTT_HEADER *buf, int size)
{
    int err = 0;
    int bytes_remaining = size;
    ACPI_PMTT_HEADER *pbuf = buf;

    do {
        err = parse_pmtt_one_memory_controller(num_bandwidth, bandwidth,
                                               pbuf, &bytes_remaining);
        pbuf = (ACPI_PMTT_HEADER *)((char *)pbuf + size - bytes_remaining);
        size = bytes_remaining;
    }
    while (!err && bytes_remaining > 0);

    return err;
}

static int parse_pmtt_one_memory_controller(int num_bandwidth, int *bandwidth,
        ACPI_PMTT_HEADER *buf_in, int *bytes_remaining)
{
    int err = 0;
    unsigned int i, j;
    struct memctlr_t *buf = (struct memctlr_t *)buf_in;

    if (buf->memctlr.Header.Type != ACPI_PMTT_TYPE_CONTROLLER) {
        err = MEMKIND_ERROR_PMTT;
    }
    if (buf->memctlr.Header.Flags != 2 && buf->memctlr.Header.Flags != 0) {
        err = MEMKIND_ERROR_PMTT;
    }
    if (!err) {
        /* Read bandwidth */
        for (i = 0; i < buf->memctlr.DomainCount; i++) {
            j = buf->domain[i].ProximityDomain;
            if (j < num_bandwidth) {
                /* Skip empty controllers (with ReadBandwidth == 0) */
                if (buf->memctlr.ReadBandwidth > 0) {
                    bandwidth[j] = buf->memctlr.ReadBandwidth;
                }
            }
            else {
                err = MEMKIND_ERROR_PMTT;
            }
        }
    }
    *bytes_remaining -= buf->memctlr.Header.Length;
    return err;
}

int memkind_pmtt(char *pmtt_path, char *bandwidth_path)
{
    int fd;
    int err = 0;
    FILE *fp = NULL;
    int *bandwidth = NULL;
    size_t nwrite;
    char dir[STRLEN] = {0};

    bandwidth = (int *)malloc(sizeof(int) * NUMA_NUM_NODES);
    if (bandwidth == NULL) {
        fprintf(stderr, "ERROR: <memkind_pmtt> in allocating bandwidth array\n");
        err = errno ? -errno : 1;
        goto exit;
    }
    if (strlen(bandwidth_path) > STRLEN - 1) {
        fprintf(stderr, "ERROR <memkind_pmtt> bandwidth path too long:\n    %s\n", bandwidth_path);
        err = 1;
        goto exit;
    }
    strncpy(dir, bandwidth_path, STRLEN - 1);
    dirname(dir);
    err = mkdir(dir, 0755);
    if (err && errno != EEXIST) {
        fprintf(stderr, "ERROR: <memkind_pmtt> creating output directory %s\n", dir);
        err = errno ? -errno : 1;
        goto exit;
    }
    fd = open(bandwidth_path, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd == -1) {
        fprintf(stderr, "ERROR: <memkind_pmtt> opening %s for writing\n", bandwidth_path);
        err = errno ? -errno : 1;
        goto exit;
    }
    fp = fdopen(fd, "w");
    if (fp == NULL) {
        close(fd);
        err = errno ? -errno : 1;
        goto exit;
    }
    err = parse_pmtt_bandwidth(NUMA_NUM_NODES, bandwidth, pmtt_path);
    if (err) {
        fprintf(stderr, "ERROR: <memkind_pmtt> parsing file %s\n", pmtt_path);
        err = errno ? -errno : 1;
        goto exit;
    }
    nwrite = fwrite(bandwidth, sizeof(int), NUMA_NUM_NODES, fp);
    if (nwrite != NUMA_NUM_NODES) {
        fprintf(stderr, "ERROR: <memkind_pmtt> could not write all of %s\n", bandwidth_path);
        err = errno ? -errno : 1;
        goto exit;
    }

exit:
    if (fp != NULL) {
        fclose(fp);
    }
    if (bandwidth != NULL) {
        free(bandwidth);
    }
    return err;
}

int main (int argc, char *argv[])
{
    char pmtt_path[STRLEN];
    char bandwidth_path[STRLEN];

    pmtt_path[STRLEN - 1] = '\0';
    bandwidth_path[STRLEN - 1] = '\0';

    strncpy(pmtt_path, PMTT_PATH, STRLEN - 1);
    strncpy(bandwidth_path, MEMKIND_BANDWIDTH_PATH, STRLEN - 1);

    if ((argc > 1 &&
         (strncmp("--help", argv[1], strlen("--help")) == 0 ||
          strncmp("-h", argv[1], strlen("-h")) == 0)) ||
        argc > 3) {
        fprintf(stdout, "Usage: %s [pmtt_path] [bandwidth_path]\n", argv[0]);
        fprintf(stdout, "    pmtt_path: path to input pmtt file (default %s)\n", pmtt_path);
        fprintf(stdout, "    bandwidth_path: path to output bandwidth file (default %s)\n", bandwidth_path);
        return 0;
    }
    if (argc > 1) {
        strncpy(pmtt_path, argv[1], STRLEN - 1);
    }
    if (argc > 2) {
        strncpy(bandwidth_path, argv[2], STRLEN - 1);
    }
    return memkind_pmtt(pmtt_path, bandwidth_path);
}
