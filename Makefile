CC ?= gcc
INSTALL ?= install
JEPREFIX ?= /usr/local
VERSION ?= $(shell git describe --long | sed 's|^v||')
prefix ?= /usr
exec_prefix ?= $(prefix)
sbindir ?= $(exec_prefix)/sbin
libdir ?= $(exec_prefix)/lib64
includedir ?= $(prefix)/include
datarootdir ?= $(prefix)/share
docdir ?= $(datarootdir)/doc/numakind-$(VERSION)

CFLAGS_EXTRA = -fPIC -Wall -Werror -g -O0
OBJECTS = numakind.o numakind_mcdram.o hbwmalloc.o

all: libnumakind.so.0.0 numakind-pmtt

clean:
	rm -rf $(OBJECTS) libnumakind.so.0.0 numakind-pmtt numakind_pmtt.o

numakind.o: numakind.c numakind.h numakind_mcdram.h
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -I $(JEPREFIX)/include -c numakind.c

numakind_mcdram.o: numakind_mcdram.c numakind.h numakind_mcdram.h
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -I $(JEPREFIX)/include -c numakind_mcdram.c

hbwmalloc.o: hbwmalloc.c hbwmalloc.h numakind.h numakind_mcdram.h
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -c hbwmalloc.c

libnumakind.so.0.0: $(OBJECTS)
	$(CC) -shared -Wl,-soname,libnumakind.so.0 -o libnumakind.so.0.0 $^

numakind-pmtt: numakind_pmtt.c $(OBJECTS)
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $(OBJECTS) -lnuma -ljemalloc numakind_pmtt.c -o $@

install:
	$(INSTALL) -d $(DESTDIR)$(includedir)
	$(INSTALL) -m 644 numakind.h hbwmalloc.h $(DESTDIR)$(includedir)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) libnumakind.so.0.0 $(DESTDIR)$(libdir)
	ln -sf libnumakind.so.0.0 $(DESTDIR)$(libdir)/libnumakind.so.0
	ln -sf libnumakind.so.0.0 $(DESTDIR)$(libdir)/libnumakind.so
	$(INSTALL) -d $(DESTDIR)$(docdir)
	$(INSTALL) -m 644 README.txt $(DESTDIR)$(docdir)
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(INSTALL) numakind-pmtt $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)/etc/init.d
	$(INSTALL) numakind-init $(DESTDIR)/etc/init.d/numakind

.PHONY: all clean install
