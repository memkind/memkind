CC=g++
CFLAGS=-O3 -Wall -fopenmp
INCLUDES=-I./include
LIBS=-lnumakind -lnuma -lgtest -lpthread -ljemalloc -lgomp
bin = $(shell mkdir -p bin)

all:			$(bin) bin/all_tests

bin/all_tests:		main.o bat_tests.o threaded_tests.o malloc_tests.o check.o include/common.h
			$(CC) $(CFLAGS) -o bin/all_tests bat_tests.o threaded_tests.o malloc_tests.o main.o check.o $(LIBS)

main.o:			main.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c main.cpp -o main.o

bat_tests.o:		bat_tests.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c bat_tests.cpp -o bat_tests.o

threaded_tests.o:	threaded_tests.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c threaded_tests.cpp -o threaded_tests.o

malloc_tests.o:		malloc_tests.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c malloc_tests.cpp -o malloc_tests.o

check.o:		check.cpp
			$(CC) $(CFLAGS) $(INCLUDES) -c check.cpp -o check.o

clean:
			rm -f *.o bin/all_tests