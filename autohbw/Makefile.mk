# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2014 - 2019 Intel Corporation.

lib_LTLIBRARIES += autohbw/libautohbw.la \
                   # end

EXTRA_DIST += autohbw/autohbw_get_src_lines.pl

autohbw_libautohbw_la_LIBADD = libmemkind.la

autohbw_libautohbw_la_SOURCES = autohbw/autohbw.c

clean-local: autohbw-clean

autohbw-clean:
	rm -f autohbw/*.gcno
