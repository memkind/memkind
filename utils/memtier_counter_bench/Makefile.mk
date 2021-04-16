# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

noinst_PROGRAMS += utils/memtier_counter_bench/memtier_counter_bench

EXTRA_DIST += utils/memtier_counter_bench/run_perf.sh

utils_memtier_counter_bench_memtier_counter_bench_LDADD = libmemkind.la
utils_memtier_counter_bench_memtier_counter_bench_SOURCES = utils/memtier_counter_bench/memtier_counter_bench.cpp

clean-local: utils_memtier_counter_bench_memtier_counter_bench-clean

utils_memtier_counter_bench_memtier_counter_bench-clean:
	rm -f utils/memtier_counter_bench/*.gcno
