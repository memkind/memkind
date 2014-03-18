CC ?= gcc
JEPREFIX ?= /usr/local

OBJECTS = numakind.o numakind_mcdram.o
all: $(OBJECTS)

clean:
	rm -rf $(OBJECTS)

numakind.o: numakind.c numakind.h numakind_mcdram.h
	$(CC) -I $(JEPREFIX)/include -c numakind.c 

numakind_mcdram.o: numakind_mcdram.c numakind.h numakind_mcdram.h
	$(CC) -I $(JEPREFIX)/include -c numakind_mcdram.c

.PHONY: all clean