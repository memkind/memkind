#!/bin/bash
#
#  Copyright (C) 2014 Intel Corporation.
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
numactl --hardware | grep "^node 1" > /dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: $0 requires a NUMA enabled system with more than one node."
    exit -1
fi

#hugetot=$(cat /proc/meminfo | grep HugePages_Total | awk '{print $2}')
#if [ $hugetot -lt 4000 ]; then
#    echo "ERROR: $0 requires at least 4000 HugePages_Total total (see /proc/meminfo)"
#    exit -1
#fi

if [ ! -f /sys/firmware/acpi/tables/PMTT ]; then
    export MEMKIND_HBW_NODES=1
    $basedir/all_tests --gtest_list_tests > .tmp
    cat .tmp | while read line
    do
        if [[ $line == *. ]]; then
            test_main=$line;
        else
            if [[ $test_main == GBPagesTest. ]]; then
                if [ -f /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages ]; then
                    numactl --membind=0 $basedir/all_tests --gtest_filter="$test_main$line" $@
                    ret=$?
                    if [ $err -eq 0 ]; then err=$ret; fi
                fi
            else
                numactl --membind=0 $basedir/all_tests --gtest_filter="$test_main$line" $@
                ret=$?
                if [ $err -eq 0 ]; then err=$ret; fi
            fi
        fi
    done
    rm -f .tmp

else
    $basedir/all_tests --gtest_list_tests > .tmp
    cat .tmp | while read line
    do
	    if [[ $line == *. ]]; then
	        test_main=$line;
	    else
            if [[ $test_main == GBPagesTest. ]]; then
                if [ -f /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages ]; then
                    $basedir/all_tests --gtest_filter="$test_main$line" $@
                    ret=$?
                    if [ $err -eq 0 ]; then err=$ret; fi
                fi
            else
                $basedir/all_tests --gtest_filter="$test_main$line" $@
                ret=$?
                if [ $err -eq 0 ]; then err=$ret; fi
            fi
        fi
    done
    rm -f .tmp
fi


LD_PRELOAD=$basedir/libsched.so $basedir/schedcpu_test $@
ret=$?
if [ $err -eq 0 ]; then err=$ret; fi
#
# These tests are broken.  Will avoid using LD_PRELOAD in future test
# implementation.
#
# LD_PRELOAD=$basedir/libmallctl.so $basedir/mallctlerr_test $@
# ret=$?
# if [ $err -eq 0 ]; then err=$ret; fi
# LD_PRELOAD=$basedir/libmalloc.so $basedir/mallocerr_test $@
# ret=$?
# if [ $err -eq 0 ]; then err=$ret; fi
#
LD_PRELOAD=$basedir/libnumadist.so $basedir/tieddisterr_test $@
ret=$?
if [ $err -eq 0 ]; then err=$ret; fi
$basedir/environerr_test $@
ret=$?
if [ $err -eq 0 ]; then err=$ret; fi
LD_PRELOAD=$basedir/libfopen.so $basedir/pmtterr_test $@
ret=$?
if [ $err -eq 0 ]; then err=$ret; fi

exit $err
