# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2022 Intel Corporation.

if HAVE_CXX11
noinst_PROGRAMS += utils/memtier_zipf_bench/memtier_zipf_bench

utils_memtier_zipf_bench_memtier_zipf_bench_SOURCES = utils/memtier_zipf_bench/memtier_zipf_counter_bench.cpp
utils_memtier_zipf_bench_memtier_zipf_bench_LDADD = libmemkind.la
utils_memtier_zipf_bench_memtier_zipf_bench_LDFLAGS = $(PTHREAD_CFLAGS)
utils_memtier_zipf_bench_memtier_zipf_bench_CXXFLAGS = -DCACHE_LINE_SIZE_U64=`getconf LEVEL1_DCACHE_LINESIZE` -std=c++11
endif

clean-local: utils_memtier_zipf_bench_memtier_zipf_bench-clean

utils_memtier_zipf_bench_memtier_zipf_bench-clean:
	rm -f utils/memtier_zipf_bench/*.gcno
