#!/bin/sh
topdir=$HOME/rpmbuild
if [ $# -ne 0 ]; then
    if [ $1 == --help ] || [ $1 == -h ]; then
        echo "Usage: $0 [topdir]"
        echo "    default topdir=$topdir"
      exit 0
    fi
    topdir=$1
fi

jemalloc_prefix=`ls -t $topdir/BUILDROOT/jemalloc-*.x86_64/usr/lib64/libjemalloc.so | head -n1 | sed 's|/lib64/libjemalloc.so||'`

CPPFLAGS="-I $jemalloc_prefix/include"
LDFLAGS="-L $jemalloc_prefix/lib64"
make -f make_rpm.mk CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS" JEMALLOC_INSTALLED=true

