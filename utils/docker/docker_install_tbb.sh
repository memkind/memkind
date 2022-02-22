#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019 - 2020 Intel Corporation.

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
make -j "$(nproc)" --directory="$TBB_LOCAL_DIR"
TBB_RELEASE_DIR=$(ls -d "$TBB_LOCAL_DIR"/build/*release)

# set environment variables regarding TBB package
# shellcheck source=/dev/null
source "$TBB_RELEASE_DIR"/"$TBB_VARS_SH"
