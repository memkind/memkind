# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

noinst_LTLIBRARIES += tiering/libmemtier.la \
                   # end

tiering_libmemtier_la_SOURCES = tiering/memtier.c tiering/memtier_log.c
tiering_libmemtier_la_LDFLAGS = -rpath /nowhere

# TODO - handle debug version

clean-local: memtier-clean

memtier-clean:
	rm -f tiering/memtier.gcno
