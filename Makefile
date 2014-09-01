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

CC ?= gcc
INSTALL ?= install
JEMALLOC_PREFIX ?= /usr
MEMKIND_PREFIX ?= /usr
VERSION ?= $(shell git describe --long | sed 's|^v||')
prefix ?= /usr
exec_prefix ?= $(prefix)
sbindir ?= $(exec_prefix)/sbin
libdir ?= $(exec_prefix)/lib64
includedir ?= $(prefix)/include
datarootdir ?= $(prefix)/share
docdir ?= $(datarootdir)/doc
mandir ?= $(datarooddir)/man
initddir ?= /etc/rc.d/init.d

EXTRA_CFLAGS = -fPIC -Wall -Werror -O3 -msse4.2
OBJECTS = memkind.o memkind_hugetlb.o memkind_gbtlb.o memkind_hbw.o hbwmalloc.o memkind_default.o memkind_arena.o

all: libmemkind.so.0.0 memkind-pmtt

clean:
	rm -rf $(OBJECTS) libmemkind.so.0.0 memkind-pmtt memkind_pmtt.o doc
	make -C test clean

test:
	make -C test test

memkind.o: memkind.c memkind.h memkind_default.h memkind_hugetlb.h memkind_arena.h memkind_hbw.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -I $(JEMALLOC_PREFIX)/include -c memkind.c

memkind_hugetlb.o: memkind_hugetlb.c memkind.h memkind_default.h memkind_hugetlb.h memkind_arena.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -I $(JEMALLOC_PREFIX)/include -c memkind_hugetlb.c

memkind_gbtlb.o: memkind_gbtlb.c memkind.h memkind_default.h memkind_gbtlb.h memkind_arena.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -I $(JEMALLOC_PREFIX)/include -c memkind_gbtlb.c

memkind_hbw.o: memkind_hbw.c memkind.h memkind_default.h memkind_hugetlb.h memkind_arena.h memkind_hbw.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -I $(JEMALLOC_PREFIX)/include -c memkind_hbw.c

memkind_default.o: memkind_default.c memkind.h memkind_default.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -I $(JEMALLOC_PREFIX)/include -c memkind_default.c

memkind_arena.o: memkind_arena.c memkind.h memkind_default.h memkind_arena.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -I $(JEMALLOC_PREFIX)/include -c memkind_arena.c

hbwmalloc.o: hbwmalloc.c hbwmalloc.h memkind.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c hbwmalloc.c

libmemkind.so.0.0: $(OBJECTS)
	$(CC) -shared -Wl,-soname,libmemkind.so.0 -o libmemkind.so.0.0 $^

memkind-pmtt: memkind_pmtt.c $(OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(OBJECTS) $(LDFLAGS) -lpthread -lnuma -L$(JEMALLOC_PREFIX)/lib64 -ljemalloc memkind_pmtt.c -o $@

install:
	$(INSTALL) -d $(DESTDIR)$(includedir)
	$(INSTALL) -m 644 memkind.h hbwmalloc.h $(DESTDIR)$(includedir)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) libmemkind.so.0.0 $(DESTDIR)$(libdir)
	ln -sf libmemkind.so.0.0 $(DESTDIR)$(libdir)/libmemkind.so.0
	ln -sf libmemkind.so.0.0 $(DESTDIR)$(libdir)/libmemkind.so
	$(INSTALL) -d $(DESTDIR)$(docdir)/memkind-$(VERSION)
	$(INSTALL) -m 644 COPYING.txt $(DESTDIR)$(docdir)/memkind-$(VERSION)
	$(INSTALL) -m 644 README.txt $(DESTDIR)$(docdir)/memkind-$(VERSION)
	$(INSTALL) -d $(DESTDIR)$(mandir)/man3
	$(INSTALL) -m 644 hbwmalloc.3 $(DESTDIR)$(mandir)/man3/hbwmalloc.3
	$(INSTALL) -m 644 memkind.3 $(DESTDIR)$(mandir)/man3/memkind.3
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(INSTALL) memkind-pmtt $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)$(initddir)
	$(INSTALL) memkind-init $(DESTDIR)$(initddir)/memkind

.PHONY: all clean install doc test
