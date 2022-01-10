# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2022 Intel Corporation.

bin_PROGRAMS += memtier

memtier_SOURCES = utils/memtier/memtier.c
memtier_CFLAGS = -std=c11
memtier_LDADD = libmemkind.la
