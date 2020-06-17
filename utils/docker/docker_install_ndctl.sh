#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019- 2020 Intel Corporation.

# docker_install_ndctl.sh - is called inside a Docker container;
# installs ndctl library
#

set -e

git clone --branch="$NDCTL_LIBRARY_VERSION" https://github.com/pmem/ndctl.git $HOME/ndctl/
cd $HOME/ndctl/

if [[ $(cat /etc/os-release) = *"fedora"* ]]; then

  #reverting ldconfig_scriptlets changes for centos 7
  git config user.name "someone"
  git config user.email "someone@someplace.com"
  git revert 27e8d272b88bf03b1e931202f58094771c27ebef --no-edit

  rpmdev-setuptree

  SPEC=./rhel/ndctl.spec
  VERSION=$(./git-version)
  RPMDIR=$HOME/rpmbuild/

  git archive --format=tar --prefix="ndctl-${VERSION}/" HEAD | gzip > "$RPMDIR/SOURCES/ndctl-${VERSION}.tar.gz"

  ./autogen.sh
  ./configure --prefix=/usr --sysconfdir=/etc --libdir=/usr/lib64 --disable-docs
  make -j "$(nproc)"

  ./rpmbuild.sh

  RPM_ARCH=$(uname -m)
  sudo rpm -i $RPMDIR/RPMS/$RPM_ARCH/*.rpm

else

  ./autogen.sh
  ./configure --prefix=/usr --sysconfdir=/etc --libdir=/usr/lib
  make -j "$(nproc)"

  sudo make -j "$(nproc)" install

fi
# update shared library cache
sudo ldconfig
# return to previous directory
cd -
rm -rf $HOME/ndctl
