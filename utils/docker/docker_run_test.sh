#!/bin/bash
#
#  Copyright (C) 2019 Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright notice(s),
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice(s),
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
#  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#  EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# docker_run_test.sh - is called inside a Docker container;
# runs memkind unit tests for specified pull request number
#
# Parameters:
# -test suite name
# -heap manager
set -e

TEST_SUITE=$1
HEAP_MANAGER=$2

if [ "$TEST_SUITE" = "HBW" ]; then
    # running tests and display output in case of failure
    make check || { cat test-suite.log; exit 1; }
elif [ "$TEST_SUITE" = "PMEM" ]; then
    make unit_tests_pmem
    # running pmem examples
    find examples/.libs -name "pmem*" -executable -type f -exec sh -c "MEMKIND_HEAP_MANAGER=$HEAP_MANAGER "{}" " \;
elif [ "$TEST_SUITE" = "DAX_KMEM" ]; then
    make unit_tests_dax_kmem
else
    echo "Unknown Test suite ${TEST_SUITE}"
fi;
