CC ?= gcc
JEPREFIX ?= /usr/local

CFLAGS_EXTRA = -fPIC -Wall -Werror
OBJECTS = numakind.o numakind_mcdram.o
all: $(OBJECTS) libnumakind.so.1

clean:
	rm -rf $(OBJECTS) libnumakind.so.1

numakind.o: numakind.c numakind.h numakind_mcdram.h
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -I $(JEPREFIX)/include -c numakind.c

numakind_mcdram.o: numakind_mcdram.c numakind.h numakind_mcdram.h
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -I $(JEPREFIX)/include -c numakind_mcdram.c

libnumakind.so.1: $(OBJECTS)
	$(CC) -shared -Wl,-soname,numakind.so.1 -o libnumakind.so.1 $^

.PHONY: all clean