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
#
# Optional parameters:
# -number of pull request on memkind repository on GitHub
# -Codecov token for memkind repository to upload the coverage results
# -Threading Building Blocks library version (e.g. "2019_U4")
#

usage() {
 echo "Runs memkind unit tests inside docker container.
When pull request number specified, it checks out repository on specific pull request,
otherwise tests run on master branch.
For measuring coverage of tests, Codecov token must be passed as parameter.
For testing Threading Building Blocks, TBB library version tag must be passed as parameter.
Usage: docker_run_build_and_test.sh [-p <PR_number>] [-c <codecov_token>] [-t <TBB_LIBRARY_VERSION>]"
 exit 1;
}

while getopts ":p:c:t:" opt; do
    case "${opt}" in
        p)
            PULL_REQUEST_NO=${OPTARG}
            ;;
        c)
            CODECOV_TOKEN=${OPTARG}
            ;;
        t)
            TBB_LIBRARY_VERSION=${OPTARG}
            ;;
        \?)
            usage
            exit 1
            ;;
    esac
done

docker build --tag memkind_cont \
             --file Dockerfile.ubuntu-18.04 \
             --build-arg http_proxy=$http_proxy \
             --build-arg https_proxy=$https_proxy \
             .
docker run --rm \
           --privileged=true \
           --env http_proxy=$http_proxy \
           --env https_proxy=$https_proxy \
           --env GIT_SSL_NO_VERIFY=true \
           --env PULL_REQUEST_NO="$PULL_REQUEST_NO" \
           --env CODECOV_TOKEN="$CODECOV_TOKEN" \
           --env TBB_LIBRARY_VERSION="$TBB_LIBRARY_VERSION" \
           memkind_cont /docker_run_build_and_test.sh
