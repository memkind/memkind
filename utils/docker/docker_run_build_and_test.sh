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

if [ -n "$CODECOV_TOKEN" ]; then
    GCOV_OPTION="--enable-gcov"
fi

MEMKIND_URL=https://github.com/memkind/memkind.git

# set parameters for enabling the kernel's huge page pool
sudo sysctl vm.nr_hugepages=3000
sudo sysctl vm.nr_overcommit_hugepages=128

# cloning the repo
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
if [ -n "$CODECOV_TOKEN" ]; then
# always execute coverage script even if some tests fail
    make check || true
    cat test-suite.log
else
    make check
fi

# running pmem examples
find examples/.libs -name "pmem*" -executable -type f -exec sh -c "sudo "{}" " \;

# executing coverage script if codecov token is set
if [ -n "$CODECOV_TOKEN" ]; then
    /docker_run_coverage.sh $CODECOV_TOKEN $PWD
fi
