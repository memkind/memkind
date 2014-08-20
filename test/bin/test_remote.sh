#!/bin/bash
#
#  Copyright (C) 2014 Intel Corperation.
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
    echo "Usage: $0 rpmdir login ip"
    exit 1
fi
rpmdir=$1
remote_login=$2
remote_ip=$3

basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm -f all_tests.xml

pushd $rpmdir
nkrpm=`ls -t memkind*.rpm | grep -v debuginfo | head -n1`
jerpm=`ls -t jemalloc*.rpm | grep -v debuginfo | head -n1`
scp $nkrpm $jerpm $remote_login@$remote_ip:
popd

scp $basedir/all_tests $remote_login@$remote_ip:
scp $basedir/environerr_test $remote_login@$remote_ip:
scp $basedir/mallctlerr_test $remote_login@$remote_ip:
scp $basedir/mallocerr_test $remote_login@$remote_ip:
scp $basedir/pmtterr_test $remote_login@$remote_ip:
scp $basedir/schedcpu_test $remote_login@$remote_ip:
scp $basedir/tieddisterr_test $remote_login@$remote_ip:
scp $basedir/pmtterr_test $remote_login@$remote_ip:
scp -r  $basedir/test_libs $remote_login@$remote_ip:
scp $basedir/test.sh $remote_login@$remote_ip:

ssh root@$remote_ip "rpm -e memkind >& /dev/null"
ssh root@$remote_ip "rpm -e jemalloc >& /dev/null"
ssh root@$remote_ip "rpm -i ~$remote_login/$nkrpm ~$remote_login/$jerpm"
ssh root@$remote_ip "echo 4000 > /proc/sys/vm/nr_hugepages"
ssh root@$remote_ip "echo 4000 > /proc/sys/vm/nr_overcommit_hugepages"

err=$(ssh $remote_login@$remote_ip "./test.sh --gtest_output=xml:output_xmls/")
scp -r $remote_login@$remote_ip:output_xmls .
exit err