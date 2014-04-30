#!/bin/sh
if [ $# -ne 1 ]; then
    echo "Usage: $0 <topdir>"
    exit 1
fi

jemalloc_prefix=`ls -dt $1/BUILDROOT/jemalloc-*.x86_64/usr | head -n1`

CPPFLAGS="-I $jemalloc_prefix/include"
LDFLAGS="-L $jemalloc_prefix/lib64"
make -f make_rpm.mk CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS"

