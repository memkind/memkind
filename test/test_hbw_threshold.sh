#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROGNAME=`basename $0`

# Default path (to installation dir of memkind-tests RPM)
TEST_PATH="$basedir/"

# Pytest files
PYTEST_FILES=(hbw_env_var_test.py)

red=`tput setaf 1`
green=`tput setaf 2`
yellow=`tput setaf 3`
default=`tput sgr0`

err=0

function usage () {
   cat <<EOF

Usage: $PROGNAME [-h]

OPTIONS
    -h,
        display script usage
EOF
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

function show_skipped_tests()
{
    SKIP_PATTERN=$1
    DEFAULT_IFS=$IFS

    # Search for gtests that match given pattern
    for i in ${!GTEST_BINARIES[*]}; do
        GTEST_BINARY_PATH=$TEST_PATH${GTEST_BINARIES[$i]}
        for LINE in $($GTEST_BINARY_PATH --gtest_list_tests); do
            if [[ $LINE == *. ]]; then
                TEST_SUITE=$LINE;
            else
                if [[ "$TEST_SUITE$LINE" == *"$SKIP_PATTERN"* ]]; then
                    emit "$TEST_SUITE$LINE,${yellow}SKIPPED${default}"
                fi
            fi
        done
    done

    # Search for pytests that match given pattern
    for i in ${!PYTEST_FILES[*]}; do
        PTEST_BINARY_PATH=$TEST_PATH${PYTEST_FILES[$i]}
        IFS=$'\n'
        for LINE in $(py.test $PTEST_BINARY_PATH --collect-only); do
            if [[ $LINE == *"<Class "* ]]; then
                TEST_SUITE=$(sed "s/^.*'\(.*\)'.*$/\1/" <<< $LINE)
            elif [[ $LINE == *"<Function "* ]]; then
                LINE=$(sed "s/^.*'\(.*\)'.*$/\1/" <<< $LINE)
                if [[ "$TEST_SUITE.$LINE" == *"$SKIP_PATTERN"* ]]; then
                    emit "$TEST_SUITE.$LINE,${yellow}SKIPPED${default}"
                fi
            fi
        done
    done

    IFS=$DEFAULT_IFS
    emit ""
}

function execute_pytest()
{
    ret=1
    TESTCMD=$1
    TEST_SUITE=$2
    TEST=$3
    # Apply filter (if provided)
    if [ "$TEST_FILTER" != "" ]; then
        if [[ $TEST_SUITE.$TEST != $TEST_FILTER ]]; then
            return
        fi
    fi
    # Concatenate test command
    TESTCMD=$(printf "$TESTCMD" "$TEST$SKIPPED_PYTESTS")
    # And test prefix if applicable
    if [ "$TEST_PREFIX" != "" ]; then
        TESTCMD=$(printf "$TEST_PREFIX" "$TESTCMD")
    fi
    OUTPUT=`eval $TESTCMD`
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
        echo $RESULT >> $CSV
    else
        echo $RESULT
    fi
    return $ret
}

#begin of main script

OPTIND=1

while getopts ":h" opt; do
    case "$opt" in
        h)
            usage;
            exit 0;
            ;;
    esac
done

TEST_PATH=`normalize_path "$TEST_PATH"`

# Clear any remnants of previous execution(s)
rm -rf $CSV
rm -rf $LOG_FILE

# Run tests written in pytest
for i in ${!PYTEST_FILES[*]}; do
    PTEST_BINARY_PATH=$TEST_PATH${PYTEST_FILES[$i]}
    emit
    emit "### Processing pytest file '$PTEST_BINARY_PATH' ###"
    IFS=$'\n'
    for LINE in $(py.test $PTEST_BINARY_PATH --collect-only); do
        if [[ $LINE == *"<Class "* ]]; then
            TEST_SUITE=$(sed "s/^.*'\(.*\)'.*$/\1/" <<< $LINE)
        elif [[ $LINE == *"<Function "* ]]; then
            LINE=$(sed "s/^.*'\(.*\)'.*$/\1/" <<< $LINE)
            TEST_CMD="py.test $PTEST_BINARY_PATH -k='%s' 2>&1"
            execute_pytest "$TEST_CMD" "$TEST_SUITE" "$LINE"
            ret=$?
            if [ $err -eq 0 ]; then err=$ret; fi
        fi
    done
done

exit $err
