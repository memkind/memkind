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

if [ $# -lt 3 ] || [ $1 == --help ] || [ $1 == -h ]; then
    echo "Usage: $0 rpmdir login ip [outdir] [identity]"
    exit 1
fi
rpmdir=$1
remote_login=$2
remote_ip=$3
if [ $# -gt 3 ]; then
    outdir=$4
    mkdir -p $outdir
else
    outdir=.
fi
if [ $# -gt 4 ]; then
    identity=$5
else
    identity=$HOME/.ssh/id_rsa
fi
alias ssh='ssh -i $identity'
alias scp='scp -i $identity'

basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm -f all_tests.xml

pushd $rpmdir
mkrpm=`ls -t memkind-devel*.rpm | head -n1`
scp $mkrpm $remote_login@$remote_ip:
popd

scp $basedir/.libs/all_tests $remote_login@$remote_ip:
scp $basedir/.libs/environerr_test $remote_login@$remote_ip:
scp $basedir/.libs/mallctlerr_test $remote_login@$remote_ip:
scp $basedir/.libs/mallocerr_test $remote_login@$remote_ip:
scp $basedir/.libs/pmtterr_test $remote_login@$remote_ip:
scp $basedir/.libs/schedcpu_test $remote_login@$remote_ip:
scp $basedir/.libs/tieddisterr_test $remote_login@$remote_ip:
scp $basedir/.libs/pmtterr_test $remote_login@$remote_ip:

scp $basedir/mock-pmtt.txt $remote_login@$remote_ip:/tmp/
scp $basedir/libfopen.so $remote_login@$remote_ip:
scp $basedir/libmallctl.so $remote_login@$remote_ip:
scp $basedir/libmalloc.so $remote_login@$remote_ip:
scp $basedir/libnumadist.so $remote_login@$remote_ip:
scp $basedir/libsched.so $remote_login@$remote_ip:

scp $basedir/.libs/hello_memkind $remote_login@$remote_ip:
scp $basedir/.libs/hello_hbw $remote_login@$remote_ip:
scp $basedir/.libs/memkind_allocated $remote_login@$remote_ip:
scp $basedir/.libs/filter_memkind $remote_login@$remote_ip:
scp $basedir/.libs/stream $remote_login@$remote_ip:
scp $basedir/.libs/stream_memkind $remote_login@$remote_ip:
scp $basedir/.libs/new_kind $remote_login@$remote_ip:
scp $basedir/.libs/gb_realloc $remote_login@$remote_ip:
scp $basedir/.libs/hello_memkind_debug $remote_login@$remote_ip:

scp $basedir/test.sh $remote_login@$remote_ip:
if [ -n "$COVFILE" ]; then
    ssh $remote_login@$remote_ip "mkdir -p gtest_output"
    scp $COVFILE $remote_login@$remote_ip:gtest_output/memkind.cov
fi

ssh root@$remote_ip "rpm -e memkind-devel >& /dev/null"
ssh root@$remote_ip "rpm -i ~$remote_login/$mkrpm"
ssh root@$remote_ip "echo 4000 > /proc/sys/vm/nr_hugepages"
ssh root@$remote_ip "echo 4000 > /proc/sys/vm/nr_overcommit_hugepages"

ssh $remote_login@$remote_ip "COVFILE=gtest_output/memkind.cov ./test.sh --gtest_output=xml:gtest_output/" 2>&1| tee $outdir/test.out
err=${PIPESTATUS[0]}
scp $remote_login@$remote_ip:gtest_output/\* $outdir
exit $err
