#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2014 - 2022 Intel Corporation.

set -e
set -x

basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROGNAME=$(basename "$0")

# Default path (to installation dir of memkind-tests RPM)
TEST_PATH="$basedir/"

# Gtest binaries executed by Berta
# TODO add allocator_perf_tool_tests binary to independent sh script
GTEST_BINARIES=(all_tests decorator_test \
                memkind_stat_test defrag_reallocate background_threads_test memkind_highcapacity_test)

# Pytest files executed by Berta
PYTEST_FILES=(hbw_detection_test.py autohbw_test.py trace_mechanism_test.py max_bg_threads_env_var_test.py \
              stats_print_test.py)

PYTEST=py.test
command -v $PYTEST || PYTEST=py.test-3
command -v $PYTEST || { echo >&2 "py.test not found"; exit 1; }

red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
default=$(tput sgr0)

err=0

function usage () {
   cat <<EOF

Usage: $PROGNAME [-c csv_file] [-l log_file] [-f test_filter] [-T tests_dir] [-d] [-p] [-e] [-x] [-s] [-h]

OPTIONS
    -c,
        path to CSV file
    -l,
        path to LOG file
    -f,
        test filter
    -T,
        path to tests directory
    -d,
        skip high bandwidth memory nodes detection tests
    -p,
        skip python tests
    -e,
        run only HBW performance operation tests
    -x,
        skip tests that are passed as value
    -s,
        run disabled test (e.g. pmem long time stress test)
    -h,
        parameter added to display script usage
EOF
}

function emit() {
    if [ "$LOG_FILE" != "" ]; then
        echo "$@" 2>&1 | tee -a "$LOG_FILE";
    else
        echo "$@"
    fi
}

function normalize_path {
    local PATH=$1
    if [ ! -d "$PATH" ];
    then
        echo "Not a directory: '$PATH'"
        usage
        exit 1
    fi
    if [[ $PATH != /* ]]; then
        path=$(pwd)/$PATH
    fi
    echo "$path"
}

function show_skipped_tests()
{
    SKIP_PATTERNS=$@
    DEFAULT_IFS=$IFS

    # Search for gtests that match given pattern
    for i in ${!GTEST_BINARIES[*]}; do
        GTEST_BINARY_PATH=$TEST_PATH${GTEST_BINARIES[$i]}
        for LINE in $($GTEST_BINARY_PATH --gtest_list_tests); do
            if [[ $LINE == *. ]]; then
                TEST_SUITE=$LINE;
            else
                for pattern in $SKIP_PATTERNS; do
                    if [[ "$TEST_SUITE$LINE" == *"$pattern"* ]]; then
                        emit "$TEST_SUITE$LINE,${yellow}SKIPPED${default}"
                    fi
                done
            fi
        done
    done

    # Search for pytests that match given pattern
    for i in ${!PYTEST_FILES[*]}; do
        PTEST_BINARY_PATH=$TEST_PATH${PYTEST_FILES[$i]}
        IFS=$'\n'
        for LINE in $($PYTEST $PTEST_BINARY_PATH --collect-only); do
            IFS=$DEFAULT_IFS
            if [[ $LINE == *"<Class "* ]]; then
                TEST_SUITE="${LINE//^.*'\(.*\)'.*$/\1}"
            elif [[ $LINE == *"<Function "* ]]; then
                LINE=$(sed "s/^.*'\(.*\)'.*$/\1/" <<< $LINE)
                for pattern in $SKIP_PATTERNS; do
                    if [[ "$TEST_SUITE.$LINE" == *"$pattern"* ]]; then
                        emit "$TEST_SUITE.$LINE,${yellow}SKIPPED${default}"
                        break
                    fi
                done
            fi
        done
    done

    emit ""
}

function execute_gtest()
{
    ret_val=1
    TESTCMD=$1
    TEST=$2
    # Apply filter (if provided)
    if [ "$TEST_FILTER" != "" ]; then
        if [[ $TEST != "$TEST_FILTER" ]]; then
            return
        fi
    fi
    # Concatenate test command
    TESTCMD=$(printf "%s%s%s%s""$TESTCMD" "$TEST""$SKIPPED_GTESTS""$RUN_DISABLED_GTEST")
    # And test prefix if applicable
    if [ "$TEST_PREFIX" != "" ]; then
        TESTCMD=$(printf "%s%s" "$TEST_PREFIX" "$TESTCMD")
    fi
    OUTPUT=$(eval "$TESTCMD")
    PATOK='.*OK ].*'
    PATFAILED='.*FAILED  ].*'
        PATSKIPPED='.*PASSED  ] 0.*'
    if [[ $OUTPUT =~ $PATOK ]]; then
        RESULT="$TEST,${green}PASSED${default}"
        ret_val=0
    elif [[ $OUTPUT =~ $PATFAILED ]]; then
        RESULT="$TEST,${red}FAILED${default}"
    elif [[ $OUTPUT =~ $PATSKIPPED ]]; then
        RESULT="$TEST,${yellow}SKIPPED${default}"
        ret_val=0
    else
        RESULT="$TEST,${red}CRASH${default}"
    fi
    if [ "$CSV" != "" ]; then
        emit "$OUTPUT"
        echo "$RESULT" >> "$CSV"
    else
        echo "$RESULT"
    fi
    return $ret_val
}

function execute_pytest()
{
    ret=1
    TESTCMD=$1
    TEST_SUITE=$2
    TEST=$3
    # Apply filter (if provided)
    if [ "$TEST_FILTER" != "" ]; then
        if [[ $TEST_SUITE.$TEST != "$TEST_FILTER" ]]; then
            return
        fi
    fi
    FILTERED_TEST_NAME=$(sed "s/^\s*<Function\s\(.*\)>/\1/" <<< $TEST)
    # Concatenate test command
    TESTCMD=$(printf "%s%s" "$TESTCMD" "${FILTERED_TEST_NAME}${SKIPPED_PYTESTS}")
    # And test prefix if applicable
    if [ "$TEST_PREFIX" != "" ]; then
        TESTCMD=$(printf "%s%s" "$TEST_PREFIX" "$TESTCMD")
    fi
    OUTPUT=$(eval "$TESTCMD")
    PATOK='.*1 passed.*'
    PATFAILED='.*1 failed.*'
    PATSKIPPED='.*deselected.*'
    if [[ $OUTPUT =~ $PATOK ]]; then
        RESULT="$TEST_SUITE.$TEST,${green}PASSED${default}"
        ret=0
    elif [[ $OUTPUT =~ $PATFAILED ]]; then
        RESULT="$TEST_SUITE.$TEST,${red}FAILED${default}"
    elif [[ $OUTPUT =~ $PATSKIPPED ]]; then
        return 0
    else
        RESULT="$TEST_SUITE.$TEST,${red}CRASH${default}"
    fi
    if [ "$CSV" != "" ]; then
        emit "$OUTPUT"
        echo "$RESULT" >> "$CSV"
    else
        echo "$RESULT"
    fi
    return $ret
}

#Check support for numa nodes (at least two)
function check_numa()
{
    if ! numactl --hardware | grep "^node 1" > /dev/null; then
        echo "ERROR: $0 requires a NUMA enabled system with more than one node."
        exit 1
    fi
}

#Check support for High Bandwidth Memory - simulate one if no one was found
function check_hbw_nodes()
{
    if [ ! -f /usr/bin/memkind-hbw-nodes ]; then
        if [ -x ./memkind-hbw-nodes ]; then
            export PATH=$PATH:$PWD
        else
            echo "Cannot find 'memkind-hbw-nodes' in $PWD. Did you run 'make'?"
            exit 1
        fi
    fi

    ret=$(memkind-hbw-nodes)
    if [[ $ret == "" ]]; then
        export MEMKIND_HBW_NODES=1
        TEST_PREFIX="numactl --membind=0 --cpunodebind=$MEMKIND_HBW_NODES %s"
    fi
}

#Check automatic support for persistent memory NUMA node - simulate one if no one was found
function check_auto_dax_kmem_nodes()
{
    if [ ! -f /usr/bin/memkind-auto-dax-kmem-nodes ]; then
        if [ -x ./memkind-auto-dax-kmem-nodes ]; then
            export PATH=$PATH:$PWD
        else
            echo "Cannot find 'memkind-auto-dax-kmem-nodes' in $PWD. Did you run 'make'?"
            exit 1
        fi
    fi

    ret=$(memkind-auto-dax-kmem-nodes)
    if [[ $ret == "" ]]; then
        export MEMKIND_DAX_KMEM_NODES=1
    fi
}

function skip_hugepages_tests()
{
    echo "Skipping tests that require huge pages due to unsatisfactory system conditions"
    SKIP_PATTERNS=("2MBPages" "PAGE_SIZE_2MB" "HUGETLB")
    for pattern in "${SKIP_PATTERNS[@]}"; do
        if [ -z $SKIPPED_GTESTS ]; then
            SKIPPED_GTESTS="-:*"$pattern"*"
        else
            SKIPPED_GTESTS=$SKIPPED_GTESTS":*"$pattern"*"
        fi
        SKIPPED_PYTESTS=$SKIPPED_PYTESTS" and not "$pattern
    done
    show_skipped_tests "${SKIP_PATTERNS[@]}"
}

# Check support for hugepages - skip hugepages tests if no hugepages detected in the OS
function check_hugepages()
{
    if [ "$(cat /proc/sys/vm/nr_hugepages)" == "0" ]; then
        skip_hugepages_tests
    fi
}

#begin of main script

check_numa

check_hbw_nodes
check_auto_dax_kmem_nodes
check_hugepages

OPTIND=1

while getopts "T:c:f:l:hdesx:p:" opt; do
    case "$opt" in
        T)
            TEST_PATH=$OPTARG;
            ;;
        c)
            CSV=$OPTARG;
            ;;
        f)
            TEST_FILTER=$OPTARG;
            ;;
        l)
            LOG_FILE=$OPTARG;
            ;;
        d)
            echo "Skipping tests that detect high bandwidth memory nodes due to unsatisfactory system conditions"
            if [[ $SKIPPED_PYTESTS == "" ]]; then
                SKIPPED_PYTESTS=" and not hbw_detection"
            else
                SKIPPED_PYTESTS=$SKIPPED_PYTESTS" and not hbw_detection"
            fi
            show_skipped_tests "test_TC_MEMKIND_hbw_detection"
            ;;
        e)
            GTEST_BINARIES=(performance_test)
            SKIPPED_PYTESTS=$SKIPPED_PYTESTS"${PYTEST_FILES[*]}"
            break;
            ;;
        p)
            SKIPPED_PYTESTS=$SKIPPED_PYTESTS$OPTARG
            show_skipped_tests "$OPTARG"
            break;
            ;;
        x)
            echo "Skipping some tests on demand '$OPTARG'"
            if [[ $SKIPPED_GTESTS == "" ]]; then
                SKIPPED_GTESTS=":-"$OPTARG
            else
                SKIPPED_GTESTS=$SKIPPED_GTESTS":"$OPTARG
            fi
            show_skipped_tests "$OPTARG"
            ;;
        s)
            echo "Run also disabled tests"
            RUN_DISABLED_GTEST=" --gtest_also_run_disabled_tests"
            ;;
        h)
            usage;
            exit 0;
            ;;
        *)
            usage;
            exit 0;
            ;;
    esac
done

TEST_PATH=$(normalize_path "$TEST_PATH")

# Clear any remnants of previous execution(s)
rm -rf "$CSV"
rm -rf "$LOG_FILE"

# Run tests written in gtest
for i in ${!GTEST_BINARIES[*]}; do
    GTEST_BINARY_PATH=$TEST_PATH${GTEST_BINARIES[$i]}
    emit
    emit "### Processing gtest binary '$GTEST_BINARY_PATH' ###"
    for LINE in $($GTEST_BINARY_PATH --gtest_list_tests); do
        if [[ $LINE == *. ]]; then
            TEST_SUITE=$LINE;
        else
            TEST_CMD="$GTEST_BINARY_PATH --gtest_filter=%s 2>&1"
            execute_gtest "$TEST_CMD" "$TEST_SUITE$LINE"
            ret=$?
            if [ $err -eq 0 ]; then err=$ret; fi
        fi
    done
done

# Run tests written in pytest
for i in ${!PYTEST_FILES[*]}; do
    PTEST_BINARY_PATH=$TEST_PATH${PYTEST_FILES[$i]}
    emit
    emit "### Processing pytest file '$PTEST_BINARY_PATH' ###"
    IFS=$'\n'
    for LINE in $($PYTEST "$PTEST_BINARY_PATH" --collect-only); do
        if [[ $LINE == *"<Class "* ]]; then
            TEST_SUITE="${LINE//^.*'\(.*\)'.*$/\1}"
        elif [[ $LINE == *"<Function "* ]]; then
            LINE="${LINE//^.*'\(.*\)'.*$/\1}"
            TEST_CMD="$PYTEST $PTEST_BINARY_PATH -k='%s' 2>&1"
            execute_pytest "$TEST_CMD" "$TEST_SUITE" "$LINE"
            ret=$?
            if [ $err -eq 0 ]; then err=$ret; fi
        fi
    done
done

exit $err
