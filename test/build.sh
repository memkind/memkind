#!/bin/sh
#
#
#  Copyright (2014) Intel Corporation All Rights Reserved.
#
#  This software is supplied under the terms of a license
#  agreement or nondisclosure agreement with Intel Corp.
#  and may not be copied or disclosed except in accordance
#  with the terms of that agreement.
#
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
memkind_prefix=`ls -t $topdir/BUILDROOT/memkind-*.x86_64/usr/lib64/libmemkind.so | head -n1 | sed 's|/lib64/libmemkind.so||'`
googletest_prefix=`ls -t $topdir/BUILDROOT/googletest-*.x86_64/usr/lib64/libgtest.a | head -n1 | sed 's|/lib64/libgtest.a||'`
CPPFLAGS="-I $jemalloc_prefix/include -I $memkind_prefix/include -I $googletest_prefix/include"
LDFLAGS="-L $jemalloc_prefix/lib64 -L $memkind_prefix/lib64 -L $googletest_prefix/lib64"
make CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS" LD_LIBRARY_PATH="$jemalloc_prefix/lib64:$memkind_prefix/lib64:$LD_LIBRARY_PATH"
