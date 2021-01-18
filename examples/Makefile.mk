# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2014 - 2020 Intel Corporation.

noinst_PROGRAMS += examples/autohbw_candidates \
                   examples/filter_memkind \
                   examples/hello_hbw \
                   examples/hello_memkind \
                   examples/hello_memkind_debug \
                   examples/memkind_get_stat \
                   examples/pmem_alignment \
                   examples/pmem_and_dax_kmem_kind \
                   examples/pmem_and_default_kind \
                   examples/pmem_config \
                   examples/pmem_detect_kind \
                   examples/pmem_free_with_unknown_kind \
                   examples/pmem_kinds \
                   examples/pmem_malloc \
                   examples/pmem_malloc_unlimited \
                   examples/pmem_multithreads \
                   examples/pmem_multithreads_onekind \
                   examples/pmem_usable_size \
                   # end
if HAVE_CXX11
noinst_PROGRAMS += examples/memkind_allocated
noinst_PROGRAMS += examples/memkind_cpp_allocator
noinst_PROGRAMS += examples/pmem_cpp_allocator

if ENABLE_TXX_EXAMPLES
noinst_PROGRAMS += examples/memkind_cpp_concurrent_hash_map
endif
endif

examples_autohbw_candidates_LDADD = libmemkind.la
examples_filter_memkind_LDADD = libmemkind.la
examples_hello_hbw_LDADD = libmemkind.la
examples_hello_memkind_LDADD = libmemkind.la
examples_hello_memkind_debug_LDADD = libmemkind.la
examples_memkind_get_stat_LDADD = libmemkind.la
examples_pmem_alignment_LDADD = libmemkind.la
examples_pmem_and_dax_kmem_kind_LDADD = libmemkind.la
examples_pmem_and_default_kind_LDADD = libmemkind.la
examples_pmem_config_LDADD = libmemkind.la
examples_pmem_detect_kind_LDADD = libmemkind.la
examples_pmem_free_with_unknown_kind_LDADD = libmemkind.la
examples_pmem_kinds_LDADD = libmemkind.la
examples_pmem_malloc_LDADD = libmemkind.la
examples_pmem_malloc_unlimited_LDADD = libmemkind.la
examples_pmem_multithreads_LDADD = libmemkind.la
examples_pmem_multithreads_onekind_LDADD = libmemkind.la
examples_pmem_usable_size_LDADD = libmemkind.la

if HAVE_CXX11
examples_memkind_allocated_LDADD = libmemkind.la
examples_memkind_cpp_allocator_LDADD  = libmemkind.la
examples_pmem_cpp_allocator_LDADD = libmemkind.la

if ENABLE_TXX_EXAMPLES
examples_memkind_cpp_concurrent_hash_map_LDADD = libmemkind.la
endif
endif

examples_autohbw_candidates_SOURCES = examples/autohbw_candidates.c
examples_filter_memkind_SOURCES = examples/filter_example.c
examples_hello_hbw_SOURCES = examples/hello_hbw_example.c
examples_hello_memkind_SOURCES = examples/hello_memkind_example.c
examples_hello_memkind_debug_SOURCES = examples/hello_memkind_example.c examples/memkind_decorator_debug.c
examples_pmem_alignment_SOURCES = examples/pmem_alignment.c
examples_pmem_and_dax_kmem_kind_SOURCES = examples/pmem_and_dax_kmem_kind.c
examples_pmem_and_default_kind_SOURCES = examples/pmem_and_default_kind.c
examples_pmem_config_SOURCES = examples/pmem_config.c
examples_pmem_detect_kind_SOURCES = examples/pmem_detect_kind.c
examples_pmem_free_with_unknown_kind_SOURCES = examples/pmem_free_with_unknown_kind.c
examples_pmem_kinds_SOURCES = examples/pmem_kinds.c
examples_pmem_malloc_SOURCES = examples/pmem_malloc.c
examples_pmem_malloc_unlimited_SOURCES = examples/pmem_malloc_unlimited.c
examples_pmem_multithreads_SOURCES = examples/pmem_multithreads.c
examples_pmem_multithreads_onekind_SOURCES = examples/pmem_multithreads_onekind.c
examples_pmem_usable_size_SOURCES = examples/pmem_usable_size.c
if HAVE_CXX11
examples_memkind_allocated_SOURCES = examples/memkind_allocated_example.cpp examples/memkind_allocated.hpp
examples_memkind_cpp_allocator_SOURCES = examples/memkind_cpp_allocator.cpp
examples_pmem_cpp_allocator_SOURCES = examples/pmem_cpp_allocator.cpp

if ENABLE_TXX_EXAMPLES
examples_memkind_cpp_concurrent_hash_map_SOURCES = examples/memkind_cpp_concurrent_hash_map.cpp
endif
endif

if HAVE_CXX11
if ENABLE_TXX_EXAMPLES
examples_memkind_cpp_concurrent_hash_map_LDFLAGS = -ltbb
endif
endif

clean-local:
	rm -f examples/*.gcno examples/*.gcda
