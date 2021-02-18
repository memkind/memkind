# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

noinst_LTLIBRARIES += tiering/libutils.la \
                   # end

tiering_libutils_la_SOURCES = tiering/utils.c
tiering_libutils_la_LDFLAGS = -rpath /nowhere

clean-local: utils-clean

utils-clean:
	rm -f tiering/utils.gcno
