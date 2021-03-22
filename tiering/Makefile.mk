# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

lib_LTLIBRARIES += tiering/libmemtier.la \
                   # end

tiering_libmemtier_la_SOURCES = tiering/ctl.c \
                  tiering/ctl.h \
                  tiering/memtier.c \
                  tiering/memtier_log.c \
                  tiering/memtier_log.h \
                  # end

tiering_libmemtier_la_LIBADD = libmemkind.la

clean-local: tiering-clean

tiering-clean:
	rm -f tiering/*.gcno
