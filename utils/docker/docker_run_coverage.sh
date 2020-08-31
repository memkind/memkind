#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019 - 2020 Intel Corporation.

# docker_run_coverage.sh - is called inside a Docker container;
# calculates coverage and creates online report using codecov provided script
# The script is taken from a3be2bd tag, to prevent from using later version
# with hardcoded strict curl timeout
#
# Parameters:
# -memkind repository directory
# Options used:
# -p dir       Project root directory
# -Z           Exit with 1 if not successful. Default will Exit with 0

MEMKIND_REPO_PATH=$1
CODECOV_RELEASE=https://github.com/codecov/codecov-bash/archive/20200724-a3be2bd.tar.gz
ROOT_DIR=codecov-bash

mkdir $ROOT_DIR
curl -L $CODECOV_RELEASE | tar xz -C $ROOT_DIR --strip-components=1

$ROOT_DIR/codecov -t "$CODECOV_TOKEN" -cF "$TEST_SUITE_NAME" -p "$MEMKIND_REPO_PATH" -Z
