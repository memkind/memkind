#
#  Copyright (2014) Intel Corporation All Rights Reserved.
#
#  This software is supplied under the terms of a license
#  agreement or nondisclosure agreement with Intel Corp.
#  and may not be copied or disclosed except in accordance
#  with the terms of that agreement.
#

CC=g++
CFLAGS=-O3 -Wall -fopenmp -std=c++0x
JEMALLOC_PREFIX ?= /usr
NUMAKIND_PREFIX ?= /usr
GOOGLETEST_PREFIX ?= /usr
INCLUDE = -I$(JEMALLOC_PREFIX)/include -I$(NUMAKIND_PREFIX)/include -I$(GOOGLETEST_PREFIX)/include
LDFLAGS = -L$(JEMALLOC_PREFIX)/lib64 -L$(NUMAKIND_PREFIX)/lib64
LIBS = -lnumakind -lnuma -lpthread -ljemalloc -lgomp $(GOOGLETEST_PREFIX)/lib64/libgtest.a
OBJECTS = main.o bat_tests.o threaded_tests.o malloc_tests.o check.o
bin = $(shell mkdir -p bin)

all:			$(bin) bin/all_tests test

bin/all_tests:		$(OBJECTS) common.h
			$(CC) $(OBJECTS) $(LIBS) $(LDFLAGS) -o bin/all_tests

main.o:			main.cpp common.h
			$(CC) $(CFLAGS) $(INCLUDE) -c main.cpp -o main.o

bat_tests.o:		bat_tests.cpp common.h
			$(CC) $(CFLAGS) $(INCLUDE) -c bat_tests.cpp -o bat_tests.o

threaded_tests.o:	threaded_tests.cpp common.h
			$(CC) $(CFLAGS) $(INCLUDE) -c threaded_tests.cpp -o threaded_tests.o

malloc_tests.o:		malloc_tests.cpp common.h
			$(CC) $(CFLAGS) $(INCLUDE) -c malloc_tests.cpp -o malloc_tests.o

check.o:		check.cpp
			$(CC) $(CFLAGS) $(INCLUDE) -c check.cpp -o check.o

clean:
			rm -f $(OBJECTS) bin/all_tests
test: bin/all_tests bin/test.sh
			LD_LIBRARY_PATH+=:$(JEMALLOC_PREFIX)/lib64:$(NUMAKIND_PREFIX)/lib64 ./bin/test.sh

.PHONY: all clean test
