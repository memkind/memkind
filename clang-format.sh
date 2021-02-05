#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

CLANG_FORMAT_BIN="clang-format-10"
CLANG_FORMAT_MIN_VER="10.0.0"
CLANG_FORMAT_OPT="-i --style=file"
if ! CLANG_FORMAT=$(which $CLANG_FORMAT_BIN)
then
    echo "Package $CLANG_FORMAT_BIN was not found. Unable to check source files format policy." >&2
    exit 1
fi

CLANG_FORMAT_VER=$($CLANG_FORMAT_BIN --version 2>&1 | sed -E "s/.* ([0-9.]+).*/\1/")
dpkg --compare-versions $CLANG_FORMAT_VER "ge" $CLANG_FORMAT_MIN_VER
if (($?));
then
    echo "Minimal required version of clang-format is $CLANG_FORMAT_MIN_VER. Detected version is $CLANG_FORMAT_VER" >&2
    echo "Unable to check source files format policy." >&2
    exit 1
fi

rm -f clang-format.out
find . -type f \( -iname "*.c" -or -iname "*.cpp" -or -iname "*.h" -or -iname "*.hpp" \) \
     -not -path "./jemalloc/*" -not -path "./test/gtest_fused/*" | \
while read FILE_NAME; do
    $CLANG_FORMAT $CLANG_FORMAT_OPT $FILE_NAME
done 

if  ! [ -z "$(git diff --name-only)" ]
then
    git --no-pager diff
    echo "Please fix style issues as shown above"
    exit 1
else
    echo "Source code is compliant with format policy. No style issues were found."
    exit 0
fi
