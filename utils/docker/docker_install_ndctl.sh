#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019- 2020 Intel Corporation.

# docker_install_ndctl.sh - is called inside a Docker container;
# installs ndctl library
#

set -e

NDCTL_RELEASE_URL=https://github.com/pmem/ndctl/releases/tag/"$NDCTL_LIBRARY_VERSION"
NDCTL_TAR_GZ="$NDCTL_LIBRARY_VERSION".tar.gz
NDCTL_TARBALL_URL=https://github.com/pmem/ndctl/archive/"$NDCTL_TAR_GZ"

# check if specified ndctl release exists
if curl --output /dev/null --silent --head --fail "$NDCTL_RELEASE_URL"; then
  echo "ndctl version ${NDCTL_LIBRARY_VERSION} exist."
else
  echo "ndctl url: ${NDCTL_RELEASE_URL} is not valid. ndctl version: ${NDCTL_LIBRARY_VERSION} doesn't exist."
  exit 1
fi

NDCTL_LOCAL_DIR="$HOME"/ndctl/"$NDCTL_LIBRARY_VERSION"
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
make -j "$(nproc)"
sudo make -j "$(nproc)" install

# update shared library cache
sudo ldconfig
# return to previous directory
cd -
