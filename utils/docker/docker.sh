#!/bin/bash
#
#  Copyright (C) 2018 Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright notice(s),
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice(s),
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
#  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#  EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# docker.sh - preparation and run of memkind unit tests
#
# Parameters:
# -number of pull request on memkind repository on github
# -directory inside /mnt which will be mounted to /dev/pmem
#  e.g /mnt/pmem0 mounted to /dev/pmem0
#
# optional:
# -directory for memkind repository (default $HOME)
# -Codecov token for memkind repository to upload the coverage results
#

PULL_REQUEST_NO=$1
PMEM_DIR=$2
BUILD_DIR=${3:-$HOME}
CODECOV_TOKEN=$4

if [ -n "$CODECOV_TOKEN" ]; then
    GCOV_OPTION="--enable-gcov"
fi

MEMKIND_URL=https://github.com/memkind/memkind.git

# set parameters for enabling the kernel's huge page pool
sudo sysctl vm.nr_hugepages=3000
sudo sysctl vm.nr_overcommit_hugepages=128

# prepare test directory
mkdir /mnt/$PMEM_DIR
sudo mount -t ext4 -o dax /dev/$PMEM_DIR /mnt/$PMEM_DIR

# cloning the repo and merging with current master
mkdir -p $BUILD_DIR
cd $BUILD_DIR
git clone $MEMKIND_URL
cd $BUILD_DIR/memkind
git fetch origin pull/$PULL_REQUEST_NO/head:${PULL_REQUEST_NO}branch
git checkout ${PULL_REQUEST_NO}branch
git merge master --no-commit --no-ff

# building jemalloc and memkind
./build_jemalloc.sh
./autogen.sh
./configure --prefix=/usr $GCOV_OPTION
make -j8
make checkprogs -j8

# installing memkind
sudo make install

# running tests
set +e
if [ -n "$CODECOV_TOKEN" ]; then
    # ensuring that test run exites with 0 to allow gathering the coverage
    make check || true
else
    make check
    cat test-suite.log
fi

# running pmem examples
cd $HOME/memkind/examples/.libs
find . -name "pmem*" -executable -type f -exec sh -c "sudo "{}" /mnt/$PMEM_DIR" \;

# running coverage if codecov token is set
if [ -n "$CODECOV_TOKEN" ]; then
    cd $BUILD_DIR/memkind
    make test-clean
    ../codecov-script.sh $CODECOV_TOKEN $BUILD_DIR/memkind
fi
