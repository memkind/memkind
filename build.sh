#!/bin/sh                                                                                                             

export DESTDIR=/tmp/numakind_build
export PREFIX=/usr
make JEPREFIX=/tmp/jemalloc_build/usr
make install DESTDIR=$DESTDIR


