#
#  Copyright (2014) Intel Corporation All Rights Reserved.
#
#  This software is supplied under the terms of a license
#  agreement or nondisclosure agreement with Intel Corp.
#  and may not be copied or disclosed except in accordance
#  with the terms of that agreement.
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
