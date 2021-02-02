#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2020 Intel Corporation.

# docker_install_qemu.sh is called inside a Docker container;
# installs QEMU library
#


set -e
QEMU_LIBRARY_VERSION=5.1.0
QEMU_TAR_XZ=qemu-"$QEMU_LIBRARY_VERSION".tar.xz

QEMU_TARBALL_URL=https://download.qemu.org/"$QEMU_TAR_XZ"

QEMU_LOCAL_DIR="$HOME"/qemu/"$QEMU_LIBRARY_VERSION"
QEMU_LOCAL_TAR_XZ="$QEMU_LOCAL_DIR"/"$QEMU_TAR_XZ"

# create qemu directory in home directory
mkdir -p "$QEMU_LOCAL_DIR"

# download and untar qemu library to qemu directory
curl -L "$QEMU_TARBALL_URL" -o "$QEMU_LOCAL_TAR_XZ"
tar xvf "$QEMU_LOCAL_TAR_XZ" -C "$QEMU_LOCAL_DIR"  --strip-components=1

# go to qemu directory and build qemu library
cd "$QEMU_LOCAL_DIR"
./configure --prefix=/usr --enable-virtfs
make -j "$(nproc)"
sudo make -j "$(nproc)" install
sudo ldconfig
