#
#  Copyright (C) 2014 Intel Corperation.
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
CPP=g++
CC=gcc
EXTRA_CPPFLAGS=-O0 -g -Wall -fopenmp -std=c++0x
LIBS = -lnumakind -lnuma -lpthread -ljemalloc -lgomp -lgtest
OBJECTS = main.o \
	  check.o \
	  trial_generator.o \
	  multithreaded_tests.o \
	  extended_tests.o \
	  bat_tests.o

EXECS   = bin/all_tests \
	  bin/negative_tests \
	  bin/schedcpu_test \
	  bin/mallocerr_test \
	  bin/mallctlerr_test \
	  bin/pmtterr_test \
	  bin/environerr_test \
	  bin/tieddisterr_test

SHLIBS  = test_libs/libsched.so \
	  test_libs/libmalloc.so \
	  test_libs/libmallctl.so \
	  test_libs/libfopen.so \
	  test_libs/libnumadist.so

SHOBJS  = schedcpu_test.o \
	  malloc_err_test.o \
	  mallctl_err_test.o \
	  negative_tests.o \
	  environ_err_test.o \
	  pmtt_err_test.o \
	  tied_dist_test.o 

bin = $(shell mkdir -p bin)
slib = $(shell mkdir -p test_libs)

all:			  $(bin) $(EXECS) $(slib) $(SHLIBS)

bin/all_tests:		  $(OBJECTS) common.h
			  $(CPP) $(OBJECTS) $(LIBS) $(LDFLAGS) -o bin/all_tests

bin/negative_tests:	  main.o negative_tests.o common.h
			  $(CPP) main.o negative_tests.o $(LIBS) $(LDFLAGS) -o bin/negative_tests

bin/schedcpu_test:	  main.o sched_cpu_test.o common.h
			  $(CPP) main.o sched_cpu_test.o $(LIBS) $(LDFLAGS) -o bin/schedcpu_test

bin/mallocerr_test:	  main.o malloc_err_test.o common.h
			  $(CPP) main.o malloc_err_test.o $(LIBS) $(LDFLAGS) -o bin/mallocerr_test

bin/mallctlerr_test:	  main.o mallctl_err_test.o common.h
			  $(CPP) main.o mallctl_err_test.o $(LIBS) $(LDFLAGS) -o bin/mallctlerr_test

bin/pmtterr_test:	  main.o pmtt_err_test.o common.h
			  $(CPP) main.o pmtt_err_test.o $(LIBS) $(LDFLAGS) -o bin/pmtterr_test

bin/environerr_test:	  main.o environ_err_test.o common.h
			  $(CPP) main.o environ_err_test.o $(LIBS) $(LDFLAGS) -o bin/environerr_test

bin/tieddisterr_test:	  main.o tied_dist_test.o common.h
			  $(CPP) main.o tied_dist_test.o $(LIBS) $(LDFLAGS) -o bin/tieddisterr_test

test_libs/libsched.so:	  sched_mock.c 
			  $(CC) -g -Wall -fPIC -shared sched_mock.c -o test_libs/libsched.so

test_libs/libnumadist.so: numadist_mock.c
			  $(CC) -g -Wall -fPIC -shared numadist_mock.c -o test_libs/libnumadist.so

test_libs/libmalloc.so:	  jemalloc_mock.c
			  $(CC) -g -Wall -fPIC -shared jemalloc_mock.c -o test_libs/libmalloc.so

test_libs/libmallctl.so:  mallctl_mock.c
			  $(CC) -g -Wall -fPIC -shared mallctl_mock.c -o test_libs/libmallctl.so

test_libs/libfopen.so:    fopen_mock.c
			  $(CC) -g -Wall -fPIC -shared fopen_mock.c -o test_libs/libfopen.so

main.o:			  main.cpp common.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c main.cpp -o main.o

bat_tests.o:		  bat_tests.cpp common.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c bat_tests.cpp -o bat_tests.o

multithreaded_tests.o:	  multithreaded_tests.cpp common.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c multithreaded_tests.cpp -o multithreaded_tests.o

extended_tests.o:	  extended_tests.cpp common.h trial_generator.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c extended_tests.cpp -o extended_tests.o

negative_tests.o:	  negative_tests.cpp common.h ../numakind.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c negative_tests.cpp -o negative_tests.o

schedcpu_test.o:	  sched_cpu_test.cpp main.o common.h  ../numakind.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c sched_cpu_test.cpp -o sched_cpu_test.o

malloc_err_test.o:	  malloc_err_test.cpp main.o common.h  ../numakind.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c malloc_err_test.cpp -o malloc_err_test.o

mallctl_err_test.o:	  mallctl_err_test.cpp main.o common.h  ../numakind.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c mallctl_err_test.cpp -o mallctl_err_test.o

pmtt_err_test.o:	  pmtt_err_test.cpp main.o common.h  ../numakind.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c pmtt_err_test.cpp -o pmtt_err_test.o

tied_dist_test.o:	  tied_dist_test.cpp main.o common.h  ../numakind.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c tied_dist_test.cpp -o tied_dist_test.o

check.o:		  check.cpp
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c check.cpp -o check.o

trial_generator.o:	  trial_generator.cpp trial_generator.h
			  $(CPP) $(CFLAGS) $(EXTRA_CPPFLAGS) $(CPPFLAGS) -c trial_generator.cpp -o trial_generator.o
clean:
			  rm -f $(OBJECTS) negative_tests.o sched_cpu_test.o $(EXECS) $(SHLIBS) $(SHOBJS) *~

test: 			  bin/all_tests bin/test.sh
			  LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) ./bin/test.sh

.PHONY: all clean test
