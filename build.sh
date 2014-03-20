#!/bin/sh

if [ $# -lt 2 ]
then
    echo "Usage: $0 [-i <install path> (default <pwd>)] [-j <jemalloc source>] [-p prefix (default: /usr)]"
    exit 1
fi

DESTDIR=$(dirname $0)
PREFIX=/usr

while getopts i:j:p: opt
do
    case "$opt" in
      i)  DESTDIR="$OPTARG";;
      j)  JEPREFIX="$OPTARG";;
      p)  PREFIX="$OPTARG";;
        \?)# unknown flag
          echo >&2 \
                "usage: $0 [-i <install path> (default <pwd>)] [-j <jemalloc source>] [-p prefix] "
  exit 1;;
    esac
done
shift `expr $OPTIND - 1`

echo $PREFIX
make CC=gcc JEPREFIX=$JEPREFIX PREFIX=$PREFIX
make install DESTDIR=$DESTDIR


