#!/bin/bash
#
#  Copyright (C) 2014, 2015 Intel Corporation.
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
err=0
basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ ! -z "$TEST_HOST" ] && [ ! -z "$TEST_LOGIN" ] && [ ! -z "$TEST_RPMDIR" ]; then
    $basedir/test_remote.sh $TEST_RPMDIR $TEST_LOGIN $TEST_HOST $TEST_OUTDIR $TEST_SSHID
    err=$?
else
    if [ -z "$TEST_OUTDIR" ]; then
        TEST_OUTDIR=gtest_output
    fi
    mkdir -p $TEST_OUTDIR
    $basedir/test.sh --gtest_output=xml:$TEST_OUTDIR/ 2>&1| tee $TEST_OUTDIR/test.out
    err=${PIPESTATUS[0]}
fi

exit $err
