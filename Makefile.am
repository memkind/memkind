#
#  Copyright (C) 2014 Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright notice(s),
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice(s),
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
#  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#  EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_srcdir)/src

lib_LTLIBRARIES = libmemkind.la

libmemkind_la_SOURCES = src/hbwmalloc.c \
                        src/memkind_arena.c \
                        src/memkind.c \
                        src/memkind_default.c \
                        src/memkind_gbtlb.c \
                        src/memkind_hbw.c \
                        src/memkind_hugetlb.c \
                        src/actbl1.h \
                        src/actbl2.h \
                        src/actbl3.h \
                        src/actbl.h \
                        src/actypes.h \
                        # end

libmemkind_la_LDFLAGS = -version-info $(memkind_abi_version)

include_HEADERS = src/hbwmalloc.h \
                  src/memkind_arena.h \
                  src/memkind_default.h \
                  src/memkind_gbtlb.h \
                  src/memkind.h \
                  src/memkind_hbw.h \
                  src/memkind_hugetlb.h \
                  # end

sbin_PROGRAMS = memkind-pmtt
memkind_pmtt_SOURCES = src/memkind_pmtt.c
memkind_pmtt_LDADD = libmemkind.la

EXTRA_DIST = autogen.sh init.d/memkind gtest-1.7.0.zip examples test/check.sh test/test.sh test/test_remote.sh test/*_mock.c VERSION
dist_doc_DATA = README COPYING VERSION
dist_man_MANS = man/hbwmalloc.3 \
                man/memkind.3 \
                man/memkind_arena.3 \
                man/memkind_default.3 \
                man/memkind_gbtlb.3 \
                man/memkind_hbw.3 \
                man/memkind_hugetlb.3 \
                # end

DISTCLEANFILES = VERSION

bin_PROGRAMS =
check_PROGRAMS =
noinst_PROGRAMS =

TESTS =
XFAIL_TESTS =

AM_CFLAGS = -Wall -Werror -msse4.2 -fopenmp
AM_CXXFLAGS = -Wall -Werror -msse4.2 -std=c++0x -fopenmp

include test/Makefile.mk
include examples/Makefile.mk

.PHONY: checkprogs test
checkprogs: $(check_PROGRAMS)
