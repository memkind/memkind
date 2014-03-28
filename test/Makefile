CC=g++
CFLAGS=-O3 -Wall -fopenmp
INCLUDES=-I./include
LIBS=-lnumakind -lnuma -lgtest -lpthread -ljemalloc -lgomp
bin = $(shell mkdir -p bin)

all:			$(bin) bin/all_tests

bin/all_tests:		main.o api_tests.o most_tests.o check.o include/common.h
			$(CC) $(CFLAGS) -o bin/all_tests api_tests.o most_tests.o main.o check.o $(LIBS)

main.o:			main.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c main.cpp -o main.o

api_tests.o:		api_tests.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c api_tests.cpp -o api_tests.o

most_tests.o:		most_tests.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c most_tests.cpp -o most_tests.o

check.o:		check.cpp
			$(CC) $(CFLAGS) $(INCLUDES) -c check.cpp -o check.o

clean:
			rm -f *.o bin/all_tests