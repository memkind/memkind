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
# docker_run_build.sh - is called inside a Docker container;
# prepares and runs memkind build for specified pull request number
#
set -e

export UTILS_PREFIX=utils/docker

if [ -n "$CODECOV_TOKEN" ]; then
    GCOV_OPTION="--enable-gcov"
fi

# if ndctl library version is specified install library
if [ -n "$NDCTL_LIBRARY_VERSION" ]; then
    "$UTILS_PREFIX"/docker_install_ndctl.sh
fi

# building memkind sources and tests
./autogen.sh
./configure --prefix=/usr $GCOV_OPTION
make -j "$(nproc --all)"
make -j "$(nproc --all)" checkprogs

# building RPM package
if [[ $(cat /etc/os-release) = *"Fedora"* ]]; then
    make -j "$(nproc --all)" rpm
fi

# installing memkind
sudo make install

# if TBB library version is specified install library and use it
# as MEMKIND_HEAP_MANAGER
if [ -n "$TBB_LIBRARY_VERSION" ]; then
    source "$UTILS_PREFIX"/docker_install_tbb.sh
    HEAP_MANAGER="TBB"
fi
if [ -n "$TEST_SUITE_NAME" ]; then
    "$UTILS_PREFIX"/docker_run_test.sh "$HEAP_MANAGER"
fi
