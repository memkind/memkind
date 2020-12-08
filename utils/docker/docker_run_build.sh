#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019 - 2020 Intel Corporation.

#
# docker_run_build.sh - is called inside a Docker container;
# prepares and runs memkind build for specified pull request number
#
set -e

export UTILS_PREFIX=utils/docker

if [ -n "$QEMU_TEST" ]; then
    UTILS_QEMU=utils/qemu
    "$UTILS_PREFIX"/docker_install_libvirt.sh
    "$UTILS_PREFIX"/docker_install_qemu.sh
    sudo python3 "$UTILS_QEMU"/main.py --image="$UTILS_QEMU"/qemu-image/ubuntu-1804.img  --mode=test --force_reinstall
else
    if [ -n "$CODECOV_TOKEN" ]; then
        GCOV_OPTION="--enable-gcov"
    fi

    # if ndctl library version is specified install library
    if [ -n "$NDCTL_LIBRARY_VERSION" ]; then
        "$UTILS_PREFIX"/docker_install_ndctl.sh
    fi

    "$UTILS_PREFIX"/docker_install_hwloc.sh

    # building memkind sources and tests
    ./autogen.sh
    ./configure --prefix=/usr $GCOV_OPTION
    make -j "$(nproc)"
    make -j "$(nproc)" checkprogs

    # building RPM package
    if [[ $(cat /etc/os-release) = *"Fedora"* ]]; then
        make -j "$(nproc)" rpm
    fi

    # installing memkind
    sudo make install

    # if TBB library version is specified install library and use it
    # as MEMKIND_HEAP_MANAGER
    if [ -n "$TBB_LIBRARY_VERSION" ]; then
        source "$UTILS_PREFIX"/docker_install_tbb.sh
        HEAP_MANAGER="TBB"
    fi
    if [ -n "$TEST_SUITE_NAME" ]; then
        "$UTILS_PREFIX"/docker_run_test.sh "$HEAP_MANAGER"
    fi
fi
