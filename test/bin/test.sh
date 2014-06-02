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

basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
numactl --hardware | grep "^node 1" > /dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: $0 requires a NUMA enabled system with more than one node."
    exit -1
fi

slimit=`ulimit -s`
if [[ "$slimit" != "unlimited" ]];then
    ulimit -s hard
fi
which simics_start >& /dev/null
if [ $? -eq 0 ]; then
    mkdir -p /tmp/$$/lib64
    simics_start > /tmp/$$/simics_start.out
    if [ $? -ne 0 ]; then
        exit -1
    fi
    ip=`grep 'ip:' /tmp/$$/simics_start.out | awk '{print $2}'`
    pid=`grep 'pid:' /tmp/$$/simics_start.out | awk '{print $2}'`
    mkdir -p /tmp/$$/lib64
    for dd in `echo  $LD_LIBRARY_PATH | sed 's|:|\n|g' | tac`; do
        cp -rpL $dd/* /tmp/$$/lib64
    done
    pushd /tmp/$$
    tar zcvf lib64.tar.gz lib64
    popd
    scp /tmp/$$/lib64.tar.gz mic@$ip:
    ssh mic@$ip "tar xvf lib64.tar.gz"
    slimit=`ulimit -s`
    if [[ "$slimit" != "unlimited" ]];then
	ssh mic@ip "ulimit -s hard"
    fi
    scp $basedir/all_tests mic@$ip:
    ssh mic@$ip 'LD_LIBRARY_PATH=./lib64 NUMAKIND_HBW_NODES=1 ./all_tests --gtest_output=xml:all_tests.xml' $@
    err=$?
    scp mic@$ip:all_tests.xml .
elif [ ! -f /sys/firmware/acpi/tables/PMTT ]; then
    export NUMAKIND_HBW_NODES=1
    numactl --membind=0 $basedir/all_tests $@ --gtest_output=xml:all_tests.xml
    err=$?
else
    $basedir/all_tests $@ --gtest_output=xml:all_tests.xml
    err=$?
fi
rm -rf /tmp/$$
if [ ! -z "$pid" ]; then
    kill -9 $pid
fi
exit $err
