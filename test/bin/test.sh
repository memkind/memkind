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
