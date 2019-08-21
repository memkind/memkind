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
# run_local.sh - builds a container with ubuntu-18.04 image
# and runs docker_run_build_and_test.sh script inside it
set -e

if [[ -z "$MEMKIND_HOST_WORKDIR" ]]; then
    echo "MEMKIND_HOST_WORKDIR has to be set."
    exit 1
fi

# need to be inline with Dockerfile WORKDIR
MEMKIND_CONTAINTER_WORKDIR=/home/memkinduser/memkind/

docker build --tag memkind_cont \
             --file Dockerfile.ubuntu-18.04 \
             --build-arg http_proxy=$http_proxy \
             --build-arg https_proxy=$https_proxy \
             .
docker run --rm \
           --privileged=true \
           --env http_proxy=$http_proxy \
           --env https_proxy=$https_proxy \
           --env CODECOV_TOKEN="$CODECOV_TOKEN" \
           --env TBB_LIBRARY_VERSION="$TBB_LIBRARY_VERSION" \
           --env NDCTL_LIBRARY_VERSION="$NDCTL_LIBRARY_VERSION" \
           --mount type=bind,source="$MEMKIND_HOST_WORKDIR",target="$MEMKIND_CONTAINTER_WORKDIR" \
           memkind_cont utils/docker/docker_run_build_and_test.sh
