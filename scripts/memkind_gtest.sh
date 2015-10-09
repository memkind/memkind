#!/bin/bash
#  Copyright (C) 2015 Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright notice(s),
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#  notice(s),
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
#  EXPRESS
#  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
#  OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#  EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
#  OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#set -x
# Default paths (to installation dir of memkind-tests RPM)
TEST_PATH=/usr/share/mpss/test/memkind-dt/
LD_PATH=/usr/share/mpss/test/memkind-dt/
# Test bineries executed by Berta
TEST_BINARIES=(all_tests environerr_test schedcpu_test tieddisterr_test decorator_test environerr_hbw_malloc_test allocator_perf_tool_tests)
PROGNAME=`basename $0`
function usage () {
   cat <<EOF
Usage: $PROGNAME [-c csv_file] [-l log_file] [-f test_filter] [-T tests_dir] [-L libraries_dir ]
EOF
   exit 0
}

function emit() {
    if [ "$LOG_FILE" != "" ]; then
        echo "$@" 2>&1 | tee -a $LOG_FILE;
    else
        echo "$@"
    fi
}

function normalize_path {
    local PATH=$1
    if [ ! -d $PATH ];
    then
        echo "Not a directory: '$PATH'"
        usage
        exit 1
    fi
    if [[ $PATH != /* ]]; then
        PATH=`pwd`/$PATH
    fi
    echo $PATH
}

# Execute getopt
ARGS=$(getopt -o T:L:c:l:f:h -- "$@");

#Bad arguments
if [ $? -ne 0 ];
then
    usage
    exit 1
fi

eval set -- "$ARGS";

while true; do
    case "$1" in
        -T)
            TEST_PATH=$2;
            shift 2;
            ;;
        -L)
            LD_PATH=$2;
            shift 2;
            ;;
        -c)
            CSV=$2;
            shift 2;
            ;;
        -f)
            TEST_FILTER=$2;
            shift 2;
            ;;
        -l)
            LOG_FILE=$2;
            shift 2;
            ;;
        -h)
            usage;
            shift;
            ;;
        --)
            shift;
            break;
            ;;
    esac
done

TEST_PATH=`normalize_path "$TEST_PATH"`
LD_PATH=`normalize_path "$LD_PATH"`

# Additional enviroment settings required by some tests (currently schedcpu_test and tieddisterr_test)
TEST_ENVS=("" "" "LD_PRELOAD=$LD_PATH/libsched.so %s" "LD_PRELOAD=$LD_PATH/libnumadist.so %s" "" "" "")
# Detect platform
# TODO: what in case of single-numanode machine?
CPSIG="Intel(R) Xeon(R) CPU E5-2697 v2 @ 2.70GH"
cat /proc/cpuinfo | grep "$CPSIG" &> /dev/null
if [ "$?" == "0" ]; then
    # Configure environment for non-MCDRAM system
    NUMACTL="MEMKIND_HBW_NODES=0 /bin/bash -c '/usr/bin/numactl --membind=1 --cpunodebind=0 %s'";
fi

# Clear any remnants of previous execution(s)
rm -rf $CSV
rm -rf $LOG_FILE

# If *.hex files are not present in test binary dir, assume that they should be copied from <KNL-SOURCES>/tmp/eng/rpmbuild/BUILD/mpsp-memkind-*/test/
if [ "`ls $TEST_PATH/*.hex 2> /dev/null`" == "" ]; then
    cp -f $TEST_PATH/../*.hex $TEST_PATH
fi

function execute()
{
    TESTCMD=$1
    TEST=$2
    TEST_ENV=$3
    # Apply filter (if provided)
    if [ "$TEST_FILTER" != "" ]; then
        if [[ $TEST != $TEST_FILTER ]]; then
            return
        fi
    fi
    # Concatenate test command
    TESTCMD=$(printf "$TESTCMD" "$TEST")
    # And CrownPass environment confiuguration if applicable
    if [ "$NUMACTL" != "" ]; then
        TESTCMD=$(printf "$NUMACTL" "$TESTCMD")
    fi
    # And additional test environment if applicable
    if [ "$TEST_ENV" != "" ]; then
        TESTCMD=$(printf "$TEST_ENV" "$TESTCMD")
    fi
    OUTPUT=`eval $TESTCMD`
    PATOK='.*OK ].*'
    PATFAILED='.*FAILED  ].*'
    if [[ $OUTPUT =~ $PATOK ]]; then
        RESULT="$TEST,OK"
    elif [[ $OUTPUT =~ $PATFAILED ]]; then
        RESULT="$TEST,FAILED"
    else
        RESULT="$TEST,CRASH"
    fi
    if [ "$CSV" != "" ]; then
        emit "$OUTPUT"
        echo $RESULT >> $CSV
    else
        echo $RESULT
    fi
}

for i in ${!TEST_BINARIES[*]}; do
    TEST_BINARY_PATH=$TEST_PATH${TEST_BINARIES[$i]}
    emit "### Processing test binary '$TEST_BINARY_PATH' ###"
    emit
    for LINE in $($TEST_BINARY_PATH --gtest_list_tests); do
        if [[ $LINE == *. ]]; then
            TEST_SUITE=$LINE;
        else
            TEST_CMD="$TEST_BINARY_PATH --gtest_filter=%s 2>&1"
            execute "$TEST_CMD" "$TEST_SUITE$LINE" "${TEST_ENVS[$i]}"
        fi
    done
done
