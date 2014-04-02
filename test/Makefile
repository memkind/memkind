CC=g++
CFLAGS=-O3 -Wall -fopenmp
LIBS=-lnumakind -lnuma -lgtest -lpthread -ljemalloc -lgomp
bin = $(shell mkdir -p bin)

all:			$(bin) bin/all_tests

bin/all_tests:		main.o bat_tests.o threaded_tests.o malloc_tests.o check.o common.h
			$(CC) $(CFLAGS) -o bin/all_tests bat_tests.o threaded_tests.o malloc_tests.o main.o check.o $(LIBS)

main.o:			main.cpp common.h
			$(CC) $(CFLAGS) -c main.cpp -o main.o

bat_tests.o:		bat_tests.cpp common.h
			$(CC) $(CFLAGS) -c bat_tests.cpp -o bat_tests.o

threaded_tests.o:	threaded_tests.cpp common.h
			$(CC) $(CFLAGS) -c threaded_tests.cpp -o threaded_tests.o

malloc_tests.o:		malloc_tests.cpp common.h
			$(CC) $(CFLAGS) -c malloc_tests.cpp -o malloc_tests.o

check.o:		check.cpp
			$(CC) $(CFLAGS) -c check.cpp -o check.o

clean:
			rm -f *.o bin/all_tests