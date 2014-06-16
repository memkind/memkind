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

#hugetot=$(cat /proc/meminfo | grep HugePages_Total | awk '{print $2}')
#if [ $hugetot -lt 4000 ]; then
#    echo "ERROR: $0 requires at least 4000 HugePages_Total total (see /proc/meminfo)"
#    exit -1
#fi

if [ ! -f /sys/firmware/acpi/tables/PMTT ]; then
    export NUMAKIND_HBW_NODES=1
    numactl --membind=0 $basedir/all_tests $@
else
    $basedir/all_tests $@
fi

LD_PRELOAD=$basedir/test_libs/libsched.so $basedir/schedcpu_test
LD_PRELOAD=$basedir/test_libs/libmallctl.so $basedir/mallctlerr_test
LD_PRELOAD=$basedir/test_libs/libmalloc.so $basedir/mallocerr_test
LD_PRELOAD=$basedir/test_libs/libnumadist.so $basedir/tieddisterr_test
export NUMAKIND_HBW_NODES=-1
$basedir/environerr_test
unset NUMAKIND_HBW_NODES
LD_PRELOAD=$basedir/test_libs/libfopen.so $basedir/pmtterr_test