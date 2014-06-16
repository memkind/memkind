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
NUMAKIND_PREFIX ?= /usr
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

EXTRA_CFLAGS = -fPIC -Wall -Werror -O3
OBJECTS = numakind.o numakind_hbw.o hbwmalloc.o

all: libnumakind.so.0.0 numakind-pmtt

clean:
	rm -rf $(OBJECTS) libnumakind.so.0.0 numakind-pmtt numakind_pmtt.o doc
	make -C test clean

test:
	make -C test test

numakind.o: numakind.c numakind.h numakind_hbw.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -I $(JEMALLOC_PREFIX)/include -c numakind.c

numakind_hbw.o: numakind_hbw.c numakind.h numakind_hbw.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -I $(JEMALLOC_PREFIX)/include -c numakind_hbw.c

hbwmalloc.o: hbwmalloc.c hbwmalloc.h numakind.h numakind_hbw.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c hbwmalloc.c

libnumakind.so.0.0: $(OBJECTS)
	$(CC) -shared -Wl,-soname,libnumakind.so.0 -o libnumakind.so.0.0 $^

numakind-pmtt: numakind_pmtt.c $(OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(OBJECTS) $(LDFLAGS) -lpthread -lnuma -L$(JEMALLOC_PREFIX)/lib64 -ljemalloc numakind_pmtt.c -o $@

install:
	$(INSTALL) -d $(DESTDIR)$(includedir)
	$(INSTALL) -m 644 numakind.h hbwmalloc.h $(DESTDIR)$(includedir)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) libnumakind.so.0.0 $(DESTDIR)$(libdir)
	ln -sf libnumakind.so.0.0 $(DESTDIR)$(libdir)/libnumakind.so.0
	ln -sf libnumakind.so.0.0 $(DESTDIR)$(libdir)/libnumakind.so
	$(INSTALL) -d $(DESTDIR)$(docdir)/numakind-$(VERSION)
	$(INSTALL) -m 644 COPYING.txt $(DESTDIR)$(docdir)/numakind-$(VERSION)
	$(INSTALL) -m 644 README.txt $(DESTDIR)$(docdir)/numakind-$(VERSION)
	$(INSTALL) -d $(DESTDIR)$(mandir)/man3
	$(INSTALL) -m 644 hbwmalloc.3 $(DESTDIR)$(mandir)/man3/hbwmalloc.3
	$(INSTALL) -m 644 numakind.3 $(DESTDIR)$(mandir)/man3/numakind.3
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(INSTALL) numakind-pmtt $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)$(initddir)
	$(INSTALL) numakind-init $(DESTDIR)$(initddir)/numakind

.PHONY: all clean install doc test
