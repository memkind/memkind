#!/bin/bash
#
#  Copyright (C) 2019 Intel Corporation.
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
# docker_run_build_and_test.sh - is called inside a Docker container;
# prepares and runs memkind unit tests for specified pull request number
#
# Parameters:
# -number of pull request on memkind repository on GitHub
# -directory inside /mnt which will be mounted to /dev/pmem
#  e.g /mnt/pmem0 mounted to /dev/pmem0
# -absolute path to directory for memkind repository
#
# optional:
# -Codecov token for memkind repository to upload the coverage results
#

usage() {
 echo "Runs memkind unit tests for specified pull request number.
Usage: docker_run_build_and_test.sh -p <PR_number> -d <pmem_dir> -b <build_dir> [-c <codecov_token>]"
 exit 1;
}

while getopts ":p:d:b:c:" opt; do
    case "${opt}" in
        p)
            PULL_REQUEST_NO=${OPTARG}
            ;;
        d)
            if [ ! -d /dev/${PMEM_DIR} ]; then
                echo "-d <pmem_dir>: device /dev/<pmem_dir> must be created"
                exit 1
            fi
            PMEM_DIR=${OPTARG}
            ;;
        b)
            BUILD_DIR=${OPTARG}
            ;;
        c)
            CODECOV_TOKEN=${OPTARG}
            ;;
        \?)
            usage
            exit 1
            ;;
    esac
done

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
git clone $MEMKIND_URL .
git fetch origin pull/$PULL_REQUEST_NO/head:PR-${PULL_REQUEST_NO}
git checkout PR-${PULL_REQUEST_NO}
git merge master --no-commit --no-ff

# building jemalloc and memkind
./build_jemalloc.sh
./autogen.sh
./configure --prefix=/usr $GCOV_OPTION
make -j $(nproc --all)
make checkprogs -j $(nproc --all)

# installing memkind
sudo make install

# running tests
set +e
if [ -n "$CODECOV_TOKEN" ]; then
# always execute coverage script even if some tests fail
    make check || true
else
    cat test-suite.log
fi

# running pmem examples
cd $BUILD_DIR/examples/.libs
find . -name "pmem*" -executable -type f -exec sh -c "sudo "{}" /mnt/$PMEM_DIR" \;

# executing coverage script if codecov token is set
if [ -n "$CODECOV_TOKEN" ]; then
    /docker_run_coverage.sh $CODECOV_TOKEN $BUILD_DIR
fi
