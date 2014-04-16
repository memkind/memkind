#!/bin/sh

jemalloc_prefix=`ls -dt $HOME/rpmbuild/BUILDROOT/jemalloc-*.x86_64/usr | head -n1`
make -f make_rpm.mk JEMALLOC_PREFIX=$jemalloc_prefix $1

