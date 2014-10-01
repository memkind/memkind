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

CC = cc
CFLAGS = -O0 -g -Wall -Werror
LDFLAGS = -lmemkind -ljemalloc -lnuma -lpthread

all: hello_memkind hello_hbw filter_memkind stream stream_memkind new_kind

hello_memkind: hello_memkind_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) hello_memkind_example.c -o hello_memkind

hello_hbw: hello_hbw_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) hello_hbw_example.c -o hello_hbw

filter_memkind: filter_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) filter_example.c -o filter_memkind

stream: stream_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -fopenmp stream_example.c -o stream

stream_memkind: stream_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -DENABLE_DYNAMIC_ALLOC -fopenmp stream_example.c -o stream_memkind

new_kind: new_kind_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) new_kind_example.c -o new_kind

clean:
	rm -f hello_memkind hello_hbw filter_memkind stream_memkind stream new_kind

test: all
	./hello_memkind
	./hello_hbw
	./filter_memkind
	./stream
	./stream_memkind
	./new_kind
