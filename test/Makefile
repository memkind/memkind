CC=g++
CFLAGS=-O3 -Wall
INCLUDES=-I./include
LIBS=-lnumakind -lnuma -lgtest -lpthread -ljemalloc
bin = $(shell mkdir -p bin)

all:			$(bin) all_api malloc_tests threaded_malloc

all_api:		main.o all_api.o include/common.h
			$(CC) $(CFLAGS) -o bin/all_api bin/all_api.o bin/main.o $(LIBS)

malloc_tests:		main.o malloc_tests.o include/common.h
			$(CC) $(CFLAGS) -o bin/malloc_tests bin/malloc_tests.o bin/main.o $(LIBS)

threaded_malloc:	main.o threaded_malloc.o include/common.h
			$(CC) $(CFLAGS) -o bin/threaded_malloc bin/threaded_malloc.o bin/main.o $(LIBS) -lgomp

main.o:			main.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c main.cpp -o bin/main.o

all_api.o:		api_tests/all_api.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c api_tests/all_api.cpp -o bin/all_api.o

malloc_tests.o:		api_tests/malloc_tests.cpp include/common.h
			$(CC) $(CFLAGS) $(INCLUDES) -c api_tests/malloc_tests.cpp -o bin/malloc_tests.o

threaded_malloc.o:	api_tests/threaded_malloc.cpp include/common.h
			$(CC) $(CFLAGS) -fopenmp $(INCLUDES) -c api_tests/threaded_malloc.cpp -o bin/threaded_malloc.o

clean:
			rm -f *~ *.o bin/* include/*~
			rm -f api_tests/*~ api_tests/*.o *.o