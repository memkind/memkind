CC=gcc
CFLAGS=-O3
INCLUDES=-I../ 
LIBS=-lnuma

all:
		$(CC) $(CFLAGS) $(INCLUDES) -o pmtt_reader pmtt_reader.c $(LIBS)

install:	pmtt_reader
		install -m 0755 pmtt_reader /usr/sbin/