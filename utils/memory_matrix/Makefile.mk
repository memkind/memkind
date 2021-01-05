# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

noinst_PROGRAMS += memkind-memory-matrix

memkind_memory_matrix_SOURCES = utils/memory_matrix/memory_matrix.c

clean-local: memkind_memory_matrix-clean

memkind_memory_matrix-clean:
	rm -f utils/memory_matrix/*.gcno
