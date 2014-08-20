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

all: hello_nk hello_hbw filter_nk stream_nk

hello_nk: hello_memkind_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) hello_memkind_example.c -o hello_nk

hello_hbw: hello_hbw_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) hello_hbw_example.c -o hello_hbw

filter_nk: filter_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) filter_example.c -o filter_nk

stream_nk: stream_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -DENABLE_DYNAMIC_ALLOC -fopenmp stream_example.c -o stream_nk

stream: stream_example.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -fopenmp stream_example.c -o stream

clean:
	rm -f hello_nk hello_hbw filter_nk stream_nk stream

test: hello_nk filter_nk
	./hello_nk
	./hello_hbw
	./filter_nk
	./stream
	./stream_nk
