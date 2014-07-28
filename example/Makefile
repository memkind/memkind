CC ?= cc

all: hello_nk filter_nk stream_nk

hello_nk: hello_example.c
	$(CC) hello_example.c -l numakind -ljemalloc -lnuma -lpthread -o hello_nk
filter_nk: filter_example.c
	$(CC) filter_example.c -l numakind -ljemalloc -lnuma -lpthread -o filter_nk
stream_nk: stream_example.c
	$(CC) stream_example.c -DENABLE_DYNAMIC_ALLOC -l numakind -ljemalloc -lnuma -lpthread -o stream_nk
clean:
	rm -f hello_nk filter_nk stream_nk

test: hello_nk filter_nk
	./hello_nk
	./filter_nk
	./stream_nk

