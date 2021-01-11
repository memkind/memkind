#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019 - 2021 Intel Corporation.

#
# run_local.sh - builds a container with Docker image
# and runs docker_run_build.sh script inside it
#
# Parameters:
# -name of the Docker image file

set -e

DOCKER_IMAGE_NAME="$1"
export PMEM_HOST_PATH=${PMEM_HOST_PATH:-/tmp/}

if [[ ! -f "$DOCKER_IMAGE_NAME" ]]; then
    echo "Docker image "$DOCKER_IMAGE_NAME" does not exist."
    exit 1
fi

if [[ -z "$MEMKIND_HOST_WORKDIR" ]]; then
    echo "MEMKIND_HOST_WORKDIR has to be set."
    exit 1
fi

if [ -z "$QEMU_TEST" ]; then qemu_enable=; else qemu_enable="--device=/dev/kvm -v /var/run/libvirt/libvirt-sock:/var/run/libvirt/libvirt-sock"; fi

# need to be inline with Dockerfile WORKDIR
MEMKIND_CONTAINER_WORKDIR=/home/memkinduser/memkind/
PMEM_CONTAINER_PATH=/home/memkinduser/mnt_pmem/

docker build --tag memkind_cont \
             --file "$DOCKER_IMAGE_NAME" \
             --build-arg http_proxy=$http_proxy \
             --build-arg https_proxy=$https_proxy \
             .
docker run --rm \
           --privileged=true \
           --tty=true \
           --env http_proxy=$http_proxy \
           --env https_proxy=$https_proxy \
           --env CODECOV_TOKEN="$CODECOV_TOKEN" \
           --env TEST_SUITE_NAME="$TEST_SUITE_NAME" \
           --env TBB_LIBRARY_VERSION="$TBB_LIBRARY_VERSION" \
           --env HOG_MEMORY="$HOG_MEMORY" \
           --env NDCTL_LIBRARY_VERSION="$NDCTL_LIBRARY_VERSION" \
           --env ENABLE_HWLOC="$ENABLE_HWLOC" \
           --env PMEM_CONTAINER_PATH="$PMEM_CONTAINER_PATH" \
           --env QEMU_TEST="$QEMU_TEST" \
           $qemu_enable \
           --mount type=bind,source="$MEMKIND_HOST_WORKDIR",target="$MEMKIND_CONTAINER_WORKDIR" \
           --mount type=bind,source="$PMEM_HOST_PATH",target="$PMEM_CONTAINER_PATH" \
           memkind_cont utils/docker/docker_run_build.sh
