#!/bin/bash
#
#
#  Copyright (2014) Intel Corporation All Rights Reserved.
#
#  This software is supplied under the terms of a license
#  agreement or nondisclosure agreement with Intel Corp.
#  and may not be copied or disclosed except in accordance
#  with the terms of that agreement.
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
nkrpm=`ls -t numakind*.rpm | grep -v debuginfo | head -n1`
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

ssh root@$remote_ip "rpm -e numakind >& /dev/null"
ssh root@$remote_ip "rpm -e jemalloc >& /dev/null"
ssh root@$remote_ip "rpm -i ~$remote_login/$nkrpm ~$remote_login/$jerpm"
ssh root@$remote_ip "echo 8000 > /proc/sys/vm/nr_hugepages"
ssh root@$remote_ip "echo 8000 > /proc/sys/vm/nr_overcommit_hugepages"

err=$(ssh $remote_login@$remote_ip "./test.sh --gtest_output=xml:all_tests.xml")
scp $remote_login@$remote_ip:all_tests.xml .
exit err