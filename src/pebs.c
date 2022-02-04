// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_memtier.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/pebs.h>

#include <assert.h>

int pebs_fd[CPU_LOGICAL_CORES_NUMBER];
static char *pebs_mmap[CPU_LOGICAL_CORES_NUMBER];

#define PEBS_SAMPLING_INTERVAL        1000
#define MMAP_DATA_SIZE                8
#define HOTNESS_PEBS_THREAD_FREQUENCY 10.0

// TODO remove debug only
MEMKIND_EXPORT int pebs_all_samples;

void pebs_monitor()
{
    // must call this before read from data head
    asm volatile("lfence" ::: "memory");

    __u64 last_head[CPU_LOGICAL_CORES_NUMBER] = {0};
    int samples = 0;

    for (size_t cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx) {
        perf_event_mmap_page_t *pebs_metadata =
            (perf_event_mmap_page_t *)pebs_mmap[cpu_idx];

        if (last_head[cpu_idx] < pebs_metadata->data_head) {

            __u64 timestamp = 0;
            while (last_head[cpu_idx] < pebs_metadata->data_head) {
                char *data_mmap = pebs_mmap[cpu_idx] + getpagesize() +
                    last_head[cpu_idx] % (MMAP_DATA_SIZE * getpagesize());

                perf_event_header_t *event = (perf_event_header_t *)data_mmap;

                if (event->type == PERF_RECORD_SAMPLE) {
                    // content of this struct is defined by
                    // "pe.sample_type = PERF_SAMPLE_ADDR | PERF_SAMPLE_TIME"
                    // in pebs_init()
                    // 'addr' is the accessed address
                    char *read_ptr = data_mmap + sizeof(perf_event_header_t);
                    timestamp = *(__u64 *)read_ptr;
                    read_ptr += sizeof(__u64);
                    __u64 addr = *(__u64 *)read_ptr;
                    // TODO do something useful with this data!!!
                    log_info("PEBS: cpu %lu, timestamp %llu, addr %llx",
                             cpu_idx, timestamp, addr);
                } else if ((event->type >= PERF_RECORD_MAX) ||
                           (event->size == 0)) {
                    log_info("PEBS buffer corrupted!!!");
                    last_head[cpu_idx] = pebs_metadata->data_head;
                    break;
                }

                // log_info("PEBS: lh %llu, mdlh %llu, es %d, et %d",
                //    last_head[cpu_idx], pebs_metadata->data_head, event->size,
                //    event->type);

                last_head[cpu_idx] += event->size;
                data_mmap += event->size;
                samples++;
            }

            ioctl(pebs_fd[cpu_idx], PERF_EVENT_IOC_REFRESH, 0);
            last_head[cpu_idx] = pebs_metadata->data_head;
            pebs_metadata->data_tail = pebs_metadata->data_head;

        } else {
            log_info("PEBS: no new data for cpu %lu", cpu_idx);
        }
    }

    log_info("PEBS: processed %d samples", samples);

    // TODO remove debug only
    pebs_all_samples += samples;
}

void pebs_create()
{
    // TODO add code that writes to /proc/sys/kernel/perf_event_paranoid ?

    int ret = pfm_initialize();
    if (ret != PFM_SUCCESS) {
        log_fatal("PEBS: pfm_initialize() failed!");
        exit(-1);
    }

    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pfm_perf_encode_arg_t arg;
    memset(&arg, 0, sizeof(arg));
    arg.attr = &pe;
    char event[] = "MEM_LOAD_RETIRED:L3_MISS";
    ret =
        pfm_get_os_event_encoding(event, PFM_PLM3, PFM_OS_PERF_EVENT_EXT, &arg);
    if (ret != PFM_SUCCESS) {
        log_err("PEBS: pfm_get_os_event_encoding() failed - "
                "using magic numbers!");
        pe.type = 4;
        pe.config = 0x5120D1;
    }

    pe.size = sizeof(struct perf_event_attr);
    pe.sample_period = PEBS_SAMPLING_INTERVAL;
    pe.sample_type = PERF_SAMPLE_ADDR | PERF_SAMPLE_TIME;
    pe.precise_ip = 2; // NOTE: this is required but is not set
                       // by the pfm_get_os_event_encoding()
    pe.read_format = 0;
    pe.disabled = 1;
    pe.pinned = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.wakeup_events = 1;
    pe.inherit = 1;
    pe.inherit_stat = 1;

    int group_fd = -1; // use single event group
    unsigned long flags = 0;
    int fd = 0;
    size_t cpu_idx = 0u;
    int mmap_pages = 1 + MMAP_DATA_SIZE;
    int map_size = mmap_pages * getpagesize();
    pid_t pid = getpid();

    for (cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx) {
        if ((fd = perf_event_open(&pe, pid, cpu_idx, group_fd, flags)) == -1) {
            goto pebs_not_supported;
        }

        pebs_fd[cpu_idx] = fd;
        pebs_mmap[cpu_idx] = mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, pebs_fd[cpu_idx], 0);
    }

    for (cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx) {
        ioctl(pebs_fd[cpu_idx], PERF_EVENT_IOC_RESET, 0);
        ioctl(pebs_fd[cpu_idx], PERF_EVENT_IOC_ENABLE, 0);
    }

    log_info("PEBS enabled");
    return;

pebs_not_supported:
    for (cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx) {
        munmap(pebs_mmap[cpu_idx], mmap_pages * getpagesize());
        pebs_mmap[cpu_idx] = 0;
        close(pebs_fd[cpu_idx]);
    }

    log_err("PEBS: PEBS NOT SUPPORTED! continuing without pebs!");
}

void pebs_destroy()
{
    int mmap_pages = 1 + MMAP_DATA_SIZE;

    for (size_t cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx)
        ioctl(pebs_fd[cpu_idx], PERF_EVENT_IOC_DISABLE, 0);

    for (size_t cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx) {
        munmap(pebs_mmap[cpu_idx], mmap_pages * getpagesize());
        pebs_mmap[cpu_idx] = 0;
        close(pebs_fd[cpu_idx]);
    }

    log_info("PEBS disabled");
}
