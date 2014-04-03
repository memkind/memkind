#!/bin/sh

make -f make_rpm.mk JEMALLOC_PREFIX=$HOME/rpmbuild/BUILDROOT/jemalloc-3.5.1-1.x86_64/usr $1

