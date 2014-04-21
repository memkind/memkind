CC=g++
CFLAGS=-O3 -Wall -fopenmp -std=c++0x
JEMALLOC_PREFIX ?= /usr
NUMAKIND_PREFIX ?= /usr
INCLUDE = -I$(JEMALLOC_PREFIX)/include -I$(NUMAKIND_PREFIX)/include
LDFLAGS = -L$(JEMALLOC_PREFIX)/lib64 -L$(NUMAKIND_PREFIX)/lib64
LIBS = -lnumakind -lnuma -lgtest -lpthread -ljemalloc -lgomp
OBJECTS = main.o bat_tests.o threaded_tests.o malloc_tests.o check.o
bin = $(shell mkdir -p bin)

all:			$(bin) bin/all_tests test

bin/all_tests:		$(OBJECTS) common.h
			$(CC) $(LDFLAGS) -o bin/all_tests $(OBJECTS) $(LIBS)

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
			./bin/test.sh