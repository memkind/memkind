#!/bin/sh

if [ $# -ne 1 ]; then
    echo "Usage: $0 <topdir>"
    exit 1
fi

jemalloc_prefix=`ls -dt $1/BUILDROOT/jemalloc-*.x86_64/usr | head -n1`
numakind_prefix=`ls -dt $1/BUILDROOT/numakind-*.x86_64/usr | head -n1`
googletest_prefix=`ls -dt $1/BUILDROOT/googletest-*.x86_64/usr | head -n1`
CPPFLAGS="-I $jemalloc_prefix/include -I $numakind_prefix/include -I $googletest_prefix/include"
LDFLAGS="-L $jemalloc_prefix/lib64 -L $numakind_prefix/lib64 -L $googletest_prefix/lib64"
make CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS"
