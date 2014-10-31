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

AM_CPPFLAGS += -I$(googletest)/include

check_PROGRAMS += test/all_tests \
                  test/schedcpu_test \
                  test/mallocerr_test \
                  test/pmtterr_test \
                  test/mallctlerr_test \
                  test/environerr_test \
                  test/tieddisterr_test \
                  # end

TESTS += test/check.sh

test_all_tests_LDADD = libgtest.a libmemkind.la
test_schedcpu_test_LDADD = libgtest.a libmemkind.la
test_mallocerr_test_LDADD = libgtest.a libmemkind.la
test_mallctlerr_test_LDADD = libgtest.a libmemkind.la
test_pmtterr_test_LDADD = libgtest.a libmemkind.la
test_environerr_test_LDADD = libgtest.a libmemkind.la
test_tieddisterr_test_LDADD = libgtest.a libmemkind.la

test_all_tests_SOURCES = test/common.h \
                         test/bat_tests.cpp \
                         test/gb_page_tests.cpp \
                         test/trial_generator.cpp \
                         test/check.cpp \
                         test/main.cpp \
                         test/multithreaded_tests.cpp \
                         test/trial_generator.h \
                         test/check.h \
                         test/extended_tests.cpp \
                         test/negative_tests.cpp \
                         # end

test_schedcpu_test_SOURCES = test/main.cpp test/sched_cpu_test.cpp
test_mallocerr_test_SOURCES = test/main.cpp test/malloc_err_test.cpp
test_mallctlerr_test_SOURCES = test/main.cpp test/mallctl_err_test.cpp
test_pmtterr_test_SOURCES = test/main.cpp test/pmtt_err_test.cpp
test_environerr_test_SOURCES = test/main.cpp test/environ_err_test.cpp
test_tieddisterr_test_SOURCES = test/main.cpp test/tied_dist_test.cpp

# Examples as tests
check_PROGRAMS += test/hello_memkind \
                  test/hello_hbw \
                  test/filter_memkind \
                  test/stream \
                  test/stream_memkind \
                  test/new_kind \
                  test/gb_realloc \
                  # end

test_hello_memkind_LDADD = libmemkind.la
test_hello_hbw_LDADD = libmemkind.la
test_filter_memkind_LDADD = libmemkind.la
test_stream_LDADD = libmemkind.la
test_stream_memkind_LDADD = libmemkind.la
test_new_kind_LDADD = libmemkind.la
test_gb_realloc_LDADD = libmemkind.la

test_hello_memkind_SOURCES = examples/hello_memkind_example.c
test_hello_hbw_SOURCES = examples/hello_hbw_example.c
test_filter_memkind_SOURCES = examples/filter_example.c
test_stream_SOURCES = examples/stream_example.c
test_stream_memkind_SOURCES = examples/stream_example.c
test_new_kind_SOURCES = examples/new_kind_example.c
test_gb_realloc_SOURCES = examples/gb_realloc_example.c

test_stream_memkind_CPPFLAGS = $(AM_CPPFLAGS) $(CPPFLAGS) -DENABLE_DYNAMIC_ALLOC


# All of the non-standard requirements for testing (gtest and mock .so)
.PHONY: test clean-local-gtest clean-local-mock

test: check-am

check-am: libgtest.a test/libsched.so test/libnumadist.so test/libmalloc.so test/libmallctl.so test/libfopen.so

clean-local: clean-local-gtest clean-local-mock

googletest_version = 1.7.0
googletest = gtest-$(googletest_version)
googletest_archive = $(googletest).zip
googletest_sha1 = f85f6d2481e2c6c4a18539e391aa4ea8ab0394af

$(googletest_archive):
	wget http://googletest.googlecode.com/files/$(googletest_archive)
	if [ $$(sha1sum $(googletest_archive) | awk '{print $$1}') != $(googletest_sha1) ]; then exit -1; fi

$(googletest)/VERSION: $(googletest_archive)
	rm -rf $(googletest)
	unzip $(googletest_archive)
	echo $(googletest_version) > $(googletest)/VERSION

libgtest.a: $(googletest)/VERSION
	$(CXX) $(CXXFLAGS) -isystem $(googletest)/include -I$(googletest) -pthread \
	      -c $(googletest)/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o

clean-local-gtest:
	rm -rf libgtest.a $(googletest)

# shared libraries to enable mocks using LD_PRELOAD
test/libsched.so: test/sched_mock.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -fPIC -shared $^ -o $@
test/libnumadist.so: test/numadist_mock.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -fPIC -shared $^ -o $@
test/libmalloc.so: test/jemalloc_mock.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -fPIC -shared $^ -o $@
test/libmallctl.so: test/mallctl_mock.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -fPIC -shared $^ -o $@
test/libfopen.so: test/fopen_mock.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -fPIC -shared $^ -o $@

clean-local-mock:
	rm -f test/*.so
