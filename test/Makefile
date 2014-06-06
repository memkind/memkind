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

CC=g++
EXTRA_CFLAGS=-O0 -g -Wall -fopenmp -std=c++0x
LIBS = -lnumakind -lnuma -lpthread -ljemalloc -lgomp -lgtest
OBJECTS = main.o \
	  check.o \
	  trial_generator.o \
	  negative_tests.o \
	  multithreaded_tests.o \
	  extended_tests.o \
	  bat_tests.o 

bin = $(shell mkdir -p bin)

all:			$(bin) bin/all_tests

bin/all_tests:		$(OBJECTS) common.h
			$(CC) $(OBJECTS) $(LIBS) $(LDFLAGS) -o bin/all_tests

main.o:			main.cpp common.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c main.cpp -o main.o

bat_tests.o:		bat_tests.cpp common.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c bat_tests.cpp -o bat_tests.o

multithreaded_tests.o:	multithreaded_tests.cpp common.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c multithreaded_tests.cpp -o multithreaded_tests.o

extended_tests.o:	extended_tests.cpp common.h trial_generator.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c extended_tests.cpp -o extended_tests.o

negative_tests.o:	negative_tests.cpp common.h trial_generator.h ../numakind.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c negative_tests.cpp -o negative_tests.o

check.o:		check.cpp
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c check.cpp -o check.o

trial_generator.o:	trial_generator.cpp trial_generator.h
			$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c trial_generator.cpp -o trial_generator.o

clean:
			rm -f $(OBJECTS) bin/all_tests *~

test: 			bin/all_tests bin/test.sh
			LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) ./bin/test.sh

.PHONY: all clean test
