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
# Optional parameters:
# -number of pull request on memkind repository on GitHub
# -Codecov token for memkind repository to upload the coverage results
#

usage() {
 echo "Runs memkind unit tests. When pull request number speified, it checks out repository on specific pull request, 
otherwise tests run on master branch. For measuring coverage of tests, Codecov token must be passed as parameter
Usage: docker_run_build_and_test.sh [-p <PR_number>] [-c <codecov_token>]"
 exit 1;
}

while getopts ":p:c:" opt; do
    case "${opt}" in
        p)
            PULL_REQUEST_NO=${OPTARG}
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
BUILD_DIR=$HOME/memkind

# set parameters for enabling the kernel's huge page pool
sudo sysctl vm.nr_hugepages=3000
sudo sysctl vm.nr_overcommit_hugepages=128


# cloning the repo
cd $BUILD_DIR
git clone $MEMKIND_URL .

# if pull request number specified, checking out to that PR
if [ -n "$PULL_REQUEST_NO" ]; then
    git fetch origin pull/$PULL_REQUEST_NO/head:PR-${PULL_REQUEST_NO}
    git checkout PR-${PULL_REQUEST_NO}
    git merge master --no-commit --no-ff
fi

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
find . -name "pmem*" -executable -type f -exec sh -c "sudo "{}" " \;

# executing coverage script if codecov token is set
if [ -n "$CODECOV_TOKEN" ]; then
    /docker_run_coverage.sh $CODECOV_TOKEN $BUILD_DIR
fi
