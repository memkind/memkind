#
#  Copyright (2014) Intel Corporation All Rights Reserved.
#
#  This software is supplied under the terms of a license
#  agreement or nondisclosure agreement with Intel Corp.
#  and may not be copied or disclosed except in accordance
#  with the terms of that agreement.
#

CC=g++
EXTRA_CFLAGS=-O0 -g -Wall -fopenmp -std=c++0x
LIBS = -lnumakind -lnuma -lpthread -ljemalloc -lgomp -lgtest
OBJECTS = main.o bat_tests.o threaded_tests.o check.o trial_generator.o extended_tests.o
bin = $(shell mkdir -p bin)

all:			$(bin) bin/all_tests

bin/all_tests:		$(OBJECTS) common.h
			$(CC) $(OBJECTS) $(LIBS) $(LDFLAGS) -o bin/all_tests

main.o:			main.cpp common.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c main.cpp -o main.o

bat_tests.o:		bat_tests.cpp common.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c bat_tests.cpp -o bat_tests.o

threaded_tests.o:	threaded_tests.cpp common.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c threaded_tests.cpp -o threaded_tests.o

extended_tests.o:	extended_tests.cpp common.h trial_generator.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c extended_tests.cpp -o extended_tests.o

check.o:		check.cpp
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c check.cpp -o check.o

trial_generator.o:	trial_generator.cpp trial_generator.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c trial_generator.cpp -o trial_generator.o

clean:
			rm -f $(OBJECTS) bin/all_tests
test: bin/all_tests bin/test.sh
			LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) ./bin/test.sh

.PHONY: all clean test
