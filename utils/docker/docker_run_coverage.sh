#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019 - 2021 Intel Corporation.

# docker_run_coverage.sh - is called inside a Docker container;
# calculates coverage and creates online report
#
# Parameters:
# -memkind repository directory
# Options used:
# -p dir       Project root directory

MEMKIND_REPO_PATH=$1
CURL_ARGS="--retry 10 --retry-delay 2 --connect-timeout 10"
bash <(curl -s  https://codecov.io/bash) -t "$CODECOV_TOKEN" -cF "$TEST_SUITE_NAME" -p "$MEMKIND_REPO_PATH" -U "$CURL_ARGS"
