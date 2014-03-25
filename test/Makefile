CC=g++
CFLAGS=-O3 -Wall -fopenmp
INCLUDES=-I./include
LIBS=-lnumakind -lnuma -lgtest -lpthread -ljemalloc -lgomp
bin = $(shell mkdir -p bin)

all:			$(bin) all_tests

all_tests:		main.o all_tests.o include/common.h
			$(CC) $(CFLAGS) -o bin/all_tests bin/all_tests.o bin/main.o $(LIBS)

main.o:			main.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c main.cpp -o bin/main.o

all_tests.o:		all_tests.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c all_tests.cpp -o bin/all_tests.o

clean:
			rm -f *~ *.o bin/* include/*~