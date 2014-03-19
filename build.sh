#!/bin/sh

if [ $# -lt 2 ]
then
    echo "Usage: ./build.sh <install-path> <jemalloc-prefix>"
    exit 1
fi

if [ -z $1 ]
then
    echo "Empty string for install path"
    exit 1
fi

if [ -z $2 ]
then
    echo "Empty string for jemalloc-prefix"
    exit 1
fi

DESTDIR=$1
JEPREFIX=$2
PREFIX=/usr

make CC=gcc JEPREFIX=$JEPREFIX PREFIX=$PREFIX
make install DESTDIR=$DESTDIR


