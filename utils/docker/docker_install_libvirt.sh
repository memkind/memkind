#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2020 Intel Corporation.

# docker_install_libvirt.sh is called inside a Docker container;
# installs libvirt library
#


set -e
LIBVIRT_LIBRARY_VERSION=6.10.0
LIBVIRT_TAR_XZ=libvirt-"$LIBVIRT_LIBRARY_VERSION".tar.xz

LIBVIRT_TARBALL_URL=https://libvirt.org/sources/"$LIBVIRT_TAR_XZ"

LIBVIRT_LOCAL_DIR="$HOME"/libvirt/"$LIBVIRT_LIBRARY_VERSION"
LIBVIRT_LOCAL_TAR_XZ="$LIBVIRT_LOCAL_DIR"/"$LIBVIRT_TAR_XZ"

# create libvirt directory in home directory
mkdir -p "$LIBVIRT_LOCAL_DIR"

# download and untar libvirt library to libvirt directory
curl -L "$LIBVIRT_TARBALL_URL" -o "$LIBVIRT_LOCAL_TAR_XZ"
tar xvf "$LIBVIRT_LOCAL_TAR_XZ" -C "$LIBVIRT_LOCAL_DIR"  --strip-components=1

# go to libvirt directory and build libvirt library
cd "$LIBVIRT_LOCAL_DIR"

meson configure
meson build --prefix=/usr
ninja -C build
sudo ninja -C build install
sudo ldconfig
