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
numactl --hardware | grep "^node 1" > /dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: $0 requires a NUMA enabled system with more than one node."
    exit -1
fi

unset MEMKIND_HBW_NODES

#hugetot=$(cat /proc/meminfo | grep HugePages_Total | awk '{print $2}')
#if [ $hugetot -lt 4000 ]; then
#    echo "ERROR: $0 requires at least 4000 HugePages_Total total (see /proc/meminfo)"
#    exit -1
#fi

if [ ! -f /sys/firmware/acpi/tables/PMTT ]; then
    export MEMKIND_HBW_NODES=1
    test_prefix='numactl --membind=0'
fi
for line in $($basedir/all_tests --gtest_list_tests); do
    if [[ $line == *. ]]; then
        test_main=$line;
    else
        if [[ $test_main == GBPagesTest. ]]; then
            if [ -f /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages ]; then
                $test_prefix $basedir/all_tests --gtest_filter="$test_main$line" $@
                ret=$?
                if [ $err -eq 0 ]; then err=$ret; fi
            fi
        else
            $test_prefix $basedir/all_tests --gtest_filter="$test_main$line" $@
            ret=$?
            if [ $err -eq 0 ]; then err=$ret; fi
        fi
    fi
done

$basedir/decorator_test $@
ret=$?
if [ $err -eq 0 ]; then err=$ret; fi

$basedir/allocator_perf_tool_tests $@
ret=$?
if [ $err -eq 0 ]; then err=$ret; fi

$basedir/environerr_test $@
ret=$?
if [ $err -eq 0 ]; then err=$ret; fi

LD_PRELOAD=$basedir/libsched.so $basedir/schedcpu_test $@
ret=$?
if [ $err -eq 0 ]; then err=$ret; fi

LD_PRELOAD=$basedir/libnumadist.so $basedir/tieddisterr_test $@
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
# LD_PRELOAD=$basedir/libfopen.so $basedir/pmtterr_test $@
# ret=$?
# if [ $err -eq 0 ]; then err=$ret; fi



#
# Run the examples as tests
#
$basedir/hello_memkind
ret=$?
if [ $ret -ne 0 ]; then echo "FAIL: hello_memkind" 1>&2; fi
if [ $err -eq 0 ]; then err=$ret; fi

$basedir/hello_memkind_debug
ret=$?
if [ $ret -ne 0 ]; then echo "FAIL: hello_memkind_debug" 1>&2; fi
if [ $err -eq 0 ]; then err=$ret; fi

$basedir/hello_hbw
ret=$?
if [ $ret -ne 0 ]; then echo "FAIL: hello_hbw" 1>&2; fi
if [ $err -eq 0 ]; then err=$ret; fi

$basedir/memkind_allocated
ret=$?
if [ $ret -ne 0 ]; then echo "FAIL: memkind_allocated" 1>&2; fi
if [ $err -eq 0 ]; then err=$ret; fi

$basedir/filter_memkind
ret=$?
if [ $ret -ne 0 ]; then echo "FAIL: filter_memkind" 1>&2; fi
if [ $err -eq 0 ]; then err=$ret; fi

$basedir/new_kind
ret=$?
if [ $ret -ne 0 ]; then echo "FAIL: new_kind" 1>&2; fi
if [ $err -eq 0 ]; then err=$ret; fi

$basedir/pmem
ret=$?
if [ $ret -ne 0 ]; then echo "FAIL: pmem" 1>&2; fi
if [ $err -eq 0 ]; then err=$ret; fi

$basedir/stream
ret=$?
if [ $ret -ne 0 ]; then echo "FAIL: stream" 1>&2; fi
if [ $err -eq 0 ]; then err=$ret; fi

for kind in memkind_default \
            memkind_hbw \
            memkind_hbw_hugetlb \
            memkind_hbw_preferred \
            memkind_hbw_preferred_hugetlb; do
    $basedir/stream_memkind $kind
    ret=$?
    if [ $ret -ne 0 ]; then echo "FAIL: stream_memkind $kind" 1>&2; fi
    if [ $err -eq 0 ]; then err=$ret; fi
done

if [ -f /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages ]; then
    for kind in memkind_gbtlb \
                memkind_hbw_gbtlb \
                memkind_hbw_preferred_gbtlb; do
        $basedir/stream_memkind $kind
        ret=$?
        if [ $ret -ne 0 ]; then echo "FAIL: stream_memkind $kind" 1>&2; fi
        if [ $err -eq 0 ]; then err=$ret; fi
    done
    $basedir/gb_realloc
    ret=$?
    if [ $ret -ne 0 ]; then echo "FAIL: gb_realloc" 1>&2; fi
    if [ $err -eq 0 ]; then err=$ret; fi
fi

exit $err
