#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

set -e

PERF_CMD="perf stat -r 5 -B"
NUMA_CMD="numactl -N0"
MEMKIND_BIN="./memtier_counter_bench -m"
MEMTIER_KIND_BIN="./memtier_counter_bench -k"
MEMTIER_BIN="./memtier_counter_bench -r"
MEMTIER_MULTIPLE_STATIC_BIN="./memtier_counter_bench -s"
MEMTIER_MULTIPLE_DYNAMIC_BIN="./memtier_counter_bench -d"
THREADS=(1 2 4 8 16 25 32 64)

for thread in ${THREADS[*]}
do
    $PERF_CMD $MEMKIND_BIN -t "$thread"
    $PERF_CMD $MEMTIER_KIND_BIN -t "$thread"
    $PERF_CMD $MEMTIER_BIN -t "$thread"
    $PERF_CMD $MEMTIER_MULTIPLE_STATIC_BIN -t "$thread"
    $PERF_CMD $MEMTIER_MULTIPLE_DYNAMIC_BIN -t "$thread"
done

echo "NUMA test with CPU binding"

for thread in ${THREADS[*]}
do
    $NUMA_CMD $PERF_CMD $MEMKIND_BIN -t "$thread"
    $NUMA_CMD $PERF_CMD $MEMTIER_KIND_BIN -t "$thread"
    $NUMA_CMD $PERF_CMD $MEMTIER_BIN -t "$thread"
    $NUMA_CMD $PERF_CMD $MEMTIER_MULTIPLE_STATIC_BIN -t "$thread"
    $NUMA_CMD $PERF_CMD $MEMTIER_MULTIPLE_DYNAMIC_BIN -t "$thread"
done
