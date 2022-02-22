#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2016 - 2020 Intel Corporation.

set -e

cd "$(dirname "$0")"
EXTRA_CONF=( "$@" )

./autogen.sh
./configure "${EXTRA_CONF[@]}"

#use V=1 for full cmdlines of build
make all -j "$(nproc)"
make checkprogs -j"$(nproc)"
