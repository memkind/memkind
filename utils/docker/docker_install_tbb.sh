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

# docker_install_tbb.sh - is called inside a Docker container;
# installs Threading Building Block library
#

set -e

TBB_RELEASE_URL=https://github.com/01org/tbb/releases/tag/"$TBB_LIBRARY_VERSION"
TBB_TAR_GZ="$TBB_LIBRARY_VERSION".tar.gz
TBB_TARBALL_URL=https://github.com/01org/tbb/archive/"$TBB_TAR_GZ"
TBB_VARS_SH="tbbvars.sh"

# check if specified TBB release exists
if curl --output /dev/null --silent --head --fail "$TBB_RELEASE_URL"; then
  echo "TBB version ${TBB_LIBRARY_VERSION} exist."
else
  echo "TBB url: ${TBB_RELEASE_URL} is not valid. TBB version: ${TBB_LIBRARY_VERSION} doesn't exist."
  exit 1
fi

TBB_LOCAL_DIR="$HOME"/tbb/"$TBB_LIBRARY_VERSION"
TBB_LOCAL_TAR_GZ="$TBB_LOCAL_DIR"/"$TBB_TAR_GZ"

# create TBB directory in home directory
mkdir -p "$TBB_LOCAL_DIR"

# download and untar TBB library to TBB directory
curl -L "$TBB_TARBALL_URL" -o "$TBB_LOCAL_TAR_GZ"
tar -xzf "$TBB_LOCAL_TAR_GZ" -C "$TBB_LOCAL_DIR" --strip-components=1

# build TBB library
make -j "$(nproc --all)" --directory="$TBB_LOCAL_DIR"
TBB_RELEASE_DIR=$(ls -d "$TBB_LOCAL_DIR"/build/*release)

# set environment variables regarding TBB package
source "$TBB_RELEASE_DIR"/"$TBB_VARS_SH"
