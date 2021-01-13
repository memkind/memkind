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
# -Z           Exit with 1 if not successful. Default will Exit with 0

MEMKIND_REPO_PATH=$1
CURL_ARGS="--retry 10 --retry-delay 2 --connect-timeout 10"
bash <(curl -s  https://codecov.io/bash) -t "$CODECOV_TOKEN" -cF "$TEST_SUITE_NAME" -p "$MEMKIND_REPO_PATH" -Z -U "$CURL_ARGS" -v
