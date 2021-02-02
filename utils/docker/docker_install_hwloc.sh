#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2020 - 2021 Intel Corporation.

# docker_install_hwloc.sh - is called inside a Docker container;
# installs hwloc library
#
# Parameters:
# -memkind utils/docker directory
#

set -e

UTILS_DOCKER_PATH=$1

HWLOC_LIBRARY_VERSION=v2.3
HWLOC_VERSION="${HWLOC_LIBRARY_VERSION:1}".0
HWLOC_TAR_GZ=hwloc-"${HWLOC_VERSION}".tar.gz

HWLOC_TARBALL_URL=https://download.open-mpi.org/release/hwloc/"$HWLOC_LIBRARY_VERSION"/"$HWLOC_TAR_GZ"

HWLOC_LOCAL_DIR="$HOME"/hwloc/"$HWLOC_LIBRARY_VERSION"
HWLOC_LOCAL_TAR_GZ="$HWLOC_LOCAL_DIR"/"$HWLOC_TAR_GZ"

# create hwloc directory in home directory
mkdir -p "$HWLOC_LOCAL_DIR"

# download and untar hwloc library to hwloc directory
curl -L "$HWLOC_TARBALL_URL" -o "$HWLOC_LOCAL_TAR_GZ"
tar -xzf "$HWLOC_LOCAL_TAR_GZ" -C "$HWLOC_LOCAL_DIR" --strip-components=1

if [[ $(cat /etc/os-release) = *"fedora"* ]]; then
    rpmdev-setuptree

    SPEC="$UTILS_DOCKER_PATH"/hwloc.spec.mk
    RPMDIR=$HOME/rpmbuild
    RPM_ARCH=$(uname -m)

    cp "$HWLOC_LOCAL_TAR_GZ" "$RPMDIR/SOURCES/${HWLOC_TAR_GZ}"

    rpmbuild -ba $SPEC
    sudo rpm -i $RPMDIR/RPMS/$RPM_ARCH/*.rpm
else
    # go to hwloc directory, build and install library
    cd "$HWLOC_LOCAL_DIR"
    ./configure --prefix=/usr
    make -j "$(nproc)"
    sudo make -j "$(nproc)" install
fi
