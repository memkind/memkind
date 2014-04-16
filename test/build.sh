#!/bin/sh

jemalloc_prefix=`ls -dt $HOME/rpmbuild/BUILDROOT/jemalloc-*.x86_64/usr | head -n1`
numakind_prefix=`ls -dt $HOME/rpmbuild/BUILDROOT/numakind-*.x86_64/usr | head -n1`
make JEMALLOC_PREFIX=$jemalloc_prefix NUMAKIND_PREFIX=$numakind_prefix

