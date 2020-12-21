#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2020 Intel Corporation.

# docker_install_hwloc.sh - is called inside a Docker container;
# installs hwloc library
#

set -e

HWLOC_FILTER_VERSION="${HWLOC_LIBRARY_VERSION:1}"
HWLOC_TAR_GZ=hwloc-"$HWLOC_FILTER_VERSION".0.tar.gz

HWLOC_TARBALL_URL=https://download.open-mpi.org/release/hwloc/"$HWLOC_LIBRARY_VERSION"/"$HWLOC_TAR_GZ"

HWLOC_LOCAL_DIR="$HOME"/hwloc/"$HWLOC_LIBRARY_VERSION"
HWLOC_LOCAL_TAR_GZ="$HWLOC_LOCAL_DIR"/"$HWLOC_TAR_GZ"

# create hwloc directory in home directory
mkdir -p "$HWLOC_LOCAL_DIR"

# download and untar hwloc library to hwloc directory
curl -L "$HWLOC_TARBALL_URL" -o "$HWLOC_LOCAL_TAR_GZ"
tar -xzf "$HWLOC_LOCAL_TAR_GZ" -C "$HWLOC_LOCAL_DIR" --strip-components=1

# go to hwloc directory and build hwloc library
cd "$HWLOC_LOCAL_DIR"
./configure --prefix=/usr
make -j "$(nproc)"
sudo make -j "$(nproc)" install
