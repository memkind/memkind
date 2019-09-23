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

# docker_install_ndctl.sh - is called inside a Docker container;
# installs ndctl library
#
# Parameters:
# -ndctl library version (e.g. "v66")

set -e

NDCTL_VERSION="$1"
NDCTL_RELEASE_URL=https://github.com/pmem/ndctl/releases/tag/"$NDCTL_VERSION"
NDCTL_TAR_GZ="$NDCTL_VERSION".tar.gz
NDCTL_TARBALL_URL=https://github.com/pmem/ndctl/archive/"$NDCTL_TAR_GZ"

# check if specified ndctl release exists
if curl --output /dev/null --silent --head --fail "$NDCTL_RELEASE_URL"; then
  echo "ndctl version ${NDCTL_VERSION} exist."
else
  echo "ndctl url: ${NDCTL_RELEASE_URL} is not valid. ndctl version: ${NDCTL_VERSION} doesn't exist."
  exit 1
fi

NDCTL_LOCAL_DIR="$HOME"/ndctl/"$NDCTL_VERSION"
NDCTL_LOCAL_TAR_GZ="$NDCTL_LOCAL_DIR"/"$NDCTL_TAR_GZ"

# create ndctl directory in home directory
mkdir -p "$NDCTL_LOCAL_DIR"

# download and untar ndctl library to ndctl directory
curl -L "$NDCTL_TARBALL_URL" -o "$NDCTL_LOCAL_TAR_GZ"
tar -xzf "$NDCTL_LOCAL_TAR_GZ" -C "$NDCTL_LOCAL_DIR" --strip-components=1

# go to ndctl directory and build ndctl library
cd "$NDCTL_LOCAL_DIR"
./autogen.sh
./configure --prefix=/usr --sysconfdir=/etc --libdir=/usr/lib
make -j "$(nproc --all)"
sudo make -j "$(nproc --all)" install

# update shared library cache
sudo ldconfig
# return to previous directory
cd -
