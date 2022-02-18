// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/memkind_log.h>
#include <memkind/internal/memkind_memtier.h>
#include <memkind/internal/memkind_private.h>
#include <memkind/internal/pebs.h>

#include <assert.h>

#define PEBS_SAMPLING_INTERVAL        1000
#define MMAP_DATA_SIZE                8
#define MMAP_PAGES_NUM                1 + MMAP_DATA_SIZE
#define HOTNESS_PEBS_THREAD_FREQUENCY 10.0

void pebs_monitor(PebsMetadata *pebs)
{
    // must call this before read from data head
    asm volatile("lfence" ::: "memory");

    int all_cpu_samples = 0;

    for (size_t cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx) {
        perf_event_mmap_page_t *pebs_metadata =
            (perf_event_mmap_page_t *)pebs->pebs_mmap[cpu_idx];
        assert(pebs_metadata);

        __u64 timestamp = 0;
        __u64 addr = 0;
        int samples = 0;
        while (pebs->last_head[cpu_idx] < pebs_metadata->data_head) {
            char *data_mmap = pebs->pebs_mmap[cpu_idx] + getpagesize() +
                pebs->last_head[cpu_idx] % (MMAP_DATA_SIZE * getpagesize());

            perf_event_header_t *event = (perf_event_header_t *)data_mmap;

            if (event->type == PERF_RECORD_SAMPLE) {
                // content of this struct is defined by
                // "pe.sample_type = PERF_SAMPLE_ADDR | PERF_SAMPLE_TIME"
                // in pebs_init()
                // 'addr' is the accessed address
                char *read_ptr = data_mmap + sizeof(perf_event_header_t);
                timestamp = *(__u64 *)read_ptr;
                read_ptr += sizeof(__u64);
                addr = *(__u64 *)read_ptr;
                pebs->cb(addr, timestamp);
            } else if ((event->type >= PERF_RECORD_MAX) || (event->size == 0)) {
                log_fatal("PEBS buffer corrupted!!!");
                pebs->last_head[cpu_idx] = pebs_metadata->data_head;
                ioctl(pebs->pebs_fd[cpu_idx], PERF_EVENT_IOC_REFRESH, 0);
                break;
            }

            // TODO remove after POC - debug only
            // log_info("PEBS: lh %llu, mdlh %llu, es %d, et %d",
            //    last_head[cpu_idx], pebs_metadata->data_head, event->size,
            //    event->type);

            pebs->last_head[cpu_idx] += event->size;
            data_mmap += event->size;
            samples++;
        }

        if (samples > 0) {
            pebs_metadata->data_tail = pebs->last_head[cpu_idx];
            all_cpu_samples += samples;

            // TODO remove after POC - debug only
            // log_fatal("PEBS: cpu %ld samples %d", cpu_idx, samples);
        }

        ioctl(pebs->pebs_fd[cpu_idx], PERF_EVENT_IOC_REFRESH, 0);
    }

    log_info("PEBS: processed %d samples", all_cpu_samples);
}

void pebs_unmap_all_cpus(PebsMetadata *pebs)
{
    for (size_t cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx) {
        munmap(pebs->pebs_mmap[cpu_idx], MMAP_PAGES_NUM * getpagesize());
        pebs->pebs_mmap[cpu_idx] = 0;
        close(pebs->pebs_fd[cpu_idx]);
    }
}

void pebs_create(PebsMetadata *pebs, touch_cb cb)
{
    pebs->cb = cb;
    memset(&pebs->last_head[0], 0, sizeof(pebs->last_head));
    // check if we have access to perf events
    int pf = 0;
    FILE *f = fopen("/proc/sys/kernel/perf_event_paranoid", "r");
    if (fscanf(f, "%d", &pf) == 0 || pf > 3) {
        log_fatal("PEBS: /proc/sys/kernel/perf_event_paranoid is set to %d "
                  "while it should be set to 3 or less!",
                  pf);
        exit(-1);
    }

    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));

#if USE_LIBPFM
    int ret = pfm_initialize();
    if (ret != PFM_SUCCESS) {
        log_fatal("PEBS: pfm_initialize() failed!");
        exit(-1);
    }

    pfm_perf_encode_arg_t arg;
    memset(&arg, 0, sizeof(arg));
    arg.attr = &pe;
    char event[] = "MEM_LOAD_RETIRED:L3_MISS";
    ret =
        pfm_get_os_event_encoding(event, PFM_PLM3, PFM_OS_PERF_EVENT_EXT, &arg);
    if (ret != PFM_SUCCESS) {
        log_err("PEBS: pfm_get_os_event_encoding() failed - "
                "using magic numbers!");
        pe.type = PERF_TYPE_RAW;
        pe.config = 0x5120D1;
    }

#else // USE_LIBPFM == 0
    // monitor last-level cache read misses
    pe.type = PERF_TYPE_HW_CACHE;
    pe.config = PERF_COUNT_HW_CACHE_LL | PERF_COUNT_HW_CACHE_OP_READ |
        PERF_COUNT_HW_CACHE_RESULT_MISS;
#endif

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
    int map_size = MMAP_PAGES_NUM * getpagesize();
    pid_t pid = getpid();

    // our event data need to be accessed via memory from mmap() -
    // in this case we can't use "any" cpu binding, so we have to monitor
    // all cpus
    for (cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx) {
        if ((fd = syscall(SYS_perf_event_open, &pe, pid, cpu_idx, group_fd,
                          flags)) == -1) {
            pebs_unmap_all_cpus(pebs);
            log_err("PEBS: PEBS NOT SUPPORTED! continuing without pebs!");
            return;
        }

        pebs->pebs_fd[cpu_idx] = fd;
        pebs->pebs_mmap[cpu_idx] = mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, pebs->pebs_fd[cpu_idx], 0);
    }

    for (cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx) {
        ioctl(pebs->pebs_fd[cpu_idx], PERF_EVENT_IOC_RESET, 0);
        ioctl(pebs->pebs_fd[cpu_idx], PERF_EVENT_IOC_ENABLE, 0);
    }

    log_info("PEBS enabled");
}

void pebs_destroy(PebsMetadata *pebs)
{
    for (size_t cpu_idx = 0u; cpu_idx < CPU_LOGICAL_CORES_NUMBER; ++cpu_idx)
        ioctl(pebs->pebs_fd[cpu_idx], PERF_EVENT_IOC_DISABLE, 0);

    pebs_unmap_all_cpus(pebs);
    log_info("PEBS disabled");
}
