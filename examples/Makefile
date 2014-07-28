CC ?= cc

all: hello_nk filter_nk

hello_nk:
	$(CC) hello_example.c -l numakind -ljemalloc -lnuma -lpthread -o hello_nk
filter_nk:
	$(CC) filter_example.c -l numakind -ljemalloc -lnuma -lpthread -o filter_nk
clean:
	rm hello_nk filter_nk

test: hello_nk filter_nk
	./hello_nk
	./filter_nk

