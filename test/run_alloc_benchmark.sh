#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2014 - 2020 Intel Corporation.

# Allocator
ALLOCATOR="glibc tbb hbw pmem"
# Thread configuration
THREADS=(1 2 4 8 16 18 36)
# Memory configuration (in kB) / iterations
MEMORY=(1 4 16 64 256 1024 4096 16384)
# Iterations
ITERS=1000

export KMP_AFFINITY=scatter,granularity=fine

# For each algorithm
for alloc in $ALLOCATOR
do
    rm -f alloctest_$alloc.txt
    echo "# Number of threads, allocation size [kB], average malloc and free time [ms], average allocation time [ms], \
average free time [ms], first allocation time [ms], first free time [ms]" >> alloctest_$alloc.txt
    # For each number of threads
    for nthr in ${THREADS[*]}
    do
        # For each amount of memory
        for mem in ${MEMORY[*]}
        do
            echo "OMP_NUM_THREADS=$nthr ./alloc_benchmark_$alloc $ITERS $mem >> alloctest_$alloc.txt"
            OMP_NUM_THREADS=$nthr ./alloc_benchmark_$alloc $ITERS $mem >> alloctest_$alloc.txt
            ret=$?
            if [ $ret -ne 0 ]; then
                echo "Error: alloc_benchmark_$alloc returned $ret"
                exit
            fi
        done
    done
done

echo "Data collected. You can draw performance plots using python draw_plots.py."
