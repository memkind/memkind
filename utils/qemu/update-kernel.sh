# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

set -e

KERNEL_VERSION_SHORT=5.6.10
KERNEL_VERSION="$KERNEL_VERSION_SHORT"-050610
KERNEL_VERSION_LONG="$KERNEL_VERSION".202005052301
MAINLINE_URL=https://kernel.ubuntu.com/~kernel-ppa/mainline/v"$KERNEL_VERSION_SHORT"/
LINUX_HDR="$MAINLINE_URL"linux-headers-"$KERNEL_VERSION"_"$KERNEL_VERSION_LONG"_all.deb
LINUX_IMG="$MAINLINE_URL"linux-image-unsigned-"$KERNEL_VERSION"-generic_"$KERNEL_VERSION_LONG"_amd64.deb
LINUX_MODULE="$MAINLINE_URL"linux-modules-"$KERNEL_VERSION"-generic_"$KERNEL_VERSION_LONG"_amd64.deb

wget "$LINUX_HDR"
wget "$LINUX_IMG"
wget "$LINUX_MODULE"

sudo dpkg -i *.deb
