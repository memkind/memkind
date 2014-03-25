#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <numa.h>
#include <numaif.h>
#include <sys/syscall.h>
#include <bits/syscall.h>
#include <sys/types.h>
#include <jemalloc/jemalloc.h>

// The include file actbl.h comes from acpica which is available from this URL:
// https://acpica.org/sites/acpica/files/acpica-unix-20130517.tar.gz
//
#define ACPI_MACHINE_WIDTH 64
#define COMPILER_DEPENDENT_UINT64 unsigned long
#define COMPILER_DEPENDENT_INT64 long
#include "actypes.h"
#include "actbl.h"

#include "numakind.h"
#include "numakind_mcdram.h"

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
    int mfd;
    ACPI_TABLE_PMTT hdr;
    unsigned char buf[PMTT_BUF_SIZE];
    ACPI_PMTT_HEADER *pbuf = (ACPI_PMTT_HEADER *)&buf;
    int size;
    int nread;
    int pmtt_socket_size;

    memset(bandwidth, 0, sizeof(int)*num_bandwidth);

    if (numa_available() == -1) {
        return NUMAKIND_ERROR_MCDRAM;
    }
    mfd = open(pmtt_path, O_RDONLY);
    if (mfd == -1) {
        return NUMAKIND_ERROR_PMTT;
    }
    nread = read(mfd, &hdr, sizeof(ACPI_TABLE_PMTT));
    if (nread != sizeof(ACPI_TABLE_PMTT) ||
        memcmp(hdr.Header.Signature, "PMTT", 4) != 0) {
        /* PMTT signature failure */
        return NUMAKIND_ERROR_PMTT;
    }
    size = hdr.Header.Length;
    if (size < sizeof(ACPI_TABLE_PMTT) ||
        size > PMTT_BUF_SIZE + sizeof(ACPI_TABLE_PMTT)) {
        /* PMTT byte count failure */
        return NUMAKIND_ERROR_PMTT;
    }
    nread = read(mfd, &buf, size);
    if (nread != size - sizeof(ACPI_TABLE_PMTT)) {
        /* PMTT incorrect number of bytes read */
        return NUMAKIND_ERROR_PMTT;
    }

    if (pbuf->Type != ACPI_PMTT_TYPE_SOCKET) { /* SOCKET */
        /* PMTT did not find socket record first */
        return NUMAKIND_ERROR_PMTT;
    }
    pmtt_socket_size = pbuf->Length;

    if (pmtt_socket_size != nread) {
        /* PMTT extra bytes after socket record */
        return NUMAKIND_ERROR_PMTT;
    }

    if (parse_pmtt_memory_controllers(num_bandwidth, bandwidth,
                                      (ACPI_PMTT_HEADER *)&buf[sizeof(ACPI_PMTT_SOCKET)],
                                      pmtt_socket_size - sizeof(ACPI_PMTT_SOCKET))) {
        return NUMAKIND_ERROR_PMTT;
    }
    return 0;
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
    } while (!err && bytes_remaining > 0);

    return err;
}

static int parse_pmtt_one_memory_controller(int num_bandwidth, int *bandwidth,
               ACPI_PMTT_HEADER *buf_in, int *bytes_remaining)
{
    int err = 0;
    int i, j;
    struct memctlr_t *buf = (struct memctlr_t *)buf_in;

    if (buf->memctlr.Header.Type != ACPI_PMTT_TYPE_CONTROLLER) {
        err = NUMAKIND_ERROR_PMTT;
    }
    if (buf->memctlr.Header.Flags != 2 && buf->memctlr.Header.Flags != 0) {
        err = NUMAKIND_ERROR_PMTT;
    }
    if (!err) {
        /* Read bandwidth */
        for (i = 0; i < buf->memctlr.DomainCount; i++) {
            j = buf->domain[i].ProximityDomain;
            if (j < num_bandwidth) {
                bandwidth[j] = buf->memctlr.ReadBandwidth;
            }
            else {
                err = NUMAKIND_ERROR_MCDRAM;
            }
        }
    }
    *bytes_remaining -= buf->memctlr.Header.Length;
    return err;
}

int main (int argc, char *argv[]){

    int err = 0;
    FILE *fp = NULL;
    int *bandwidth = NULL;
    size_t nwrite;

    bandwidth = (int *)je_malloc(sizeof(int) *
                                 NUMA_NUM_NODES);
    if (!bandwidth) {
        fprintf(stderr, "ERROR: <%s> in allocating bandwidth array\n", argv[0]);
        return -1;
    }

    fp = fopen(NUMAKIND_BANDWIDTH_PATH, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: <%s> opening %s for writing\n", argv[0], NUMAKIND_BANDWIDTH_PATH);
        return -2;
    }
    err = parse_pmtt_bandwidth(NUMA_NUM_NODES,
                               bandwidth,
                               PMTT_PATH);
    if (err) {
        fprintf(stderr, "ERROR: <%s> opening %s for reading\n", argv[0], PMTT_PATH);
        return -3;

    }
    nwrite = fwrite(bandwidth, sizeof(int), NUMA_NUM_NODES, fp);
    if (nwrite != NUMA_NUM_NODES) {
        return -4;
    }
    fclose(fp);

    je_free(bandwidth);
    return 0;
}


