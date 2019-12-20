#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019 - 2020 Intel Corporation.

#
# docker_run_test.sh - is called inside a Docker container;
# runs specified memkind tests
#
# Parameters:
# -heap manager
set -e

HEAP_MANAGER=$1

if [ "$TEST_SUITE_NAME" = "HBW" ]; then
    # running tests and display output in case of failure
    make check || { cat test-suite.log; exit 1; }
elif [ "$TEST_SUITE_NAME" = "PMEM" ]; then
    MEMKIND_HOG_MEMORY=$HOG_MEMORY make PMEM_PATH="$PMEM_CONTAINER_PATH" unit_tests_pmem
    # running pmem examples
    find examples/.libs -name "pmem*" -executable -type f -exec sh -c "MEMKIND_HEAP_MANAGER=$HEAP_MANAGER "{}" $PMEM_CONTAINER_PATH" \;
elif [ "$TEST_SUITE_NAME" = "DAX_KMEM" ]; then
    make unit_tests_dax_kmem
else
    echo "Unknown Test suite ${TEST_SUITE_NAME}"
fi;

# executing coverage script if codecov token is set
if [ -n "$CODECOV_TOKEN" ]; then
    "$UTILS_PREFIX"/docker_run_coverage.sh "$PWD"
fi
