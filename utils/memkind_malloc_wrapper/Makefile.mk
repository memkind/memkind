# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2020 Intel Corporation.

noinst_LTLIBRARIES += utils/memkind_malloc_wrapper/libmemkind_malloc_wrapper.la \
                   # end

utils_memkind_malloc_wrapper_libmemkind_malloc_wrapper_la_LIBADD = libmemkind.la
utils_memkind_malloc_wrapper_libmemkind_malloc_wrapper_la_LDFLAGS = -rpath /nowhere

utils_memkind_malloc_wrapper_libmemkind_malloc_wrapper_la_SOURCES = utils/memkind_malloc_wrapper/memkind_malloc_wrapper.c

clean-local: utils_memkind_malloc_wrapper-clean

utils_memkind_malloc_wrapper-clean:
	rm -f utils/memkind_malloc_wrapper/*.gcno