#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

CLANG_FORMAT_OPT="-i --style=file"
if ! CLANG_FORMAT=$(which $CLANG_FORMAT_BIN)
then
    echo "Package $CLANG_FORMAT_BIN was not found. Unable to check source files format policy." >&2
    exit 1
fi

if  ! [ -z "$(git diff --name-only)" ]
then
    echo "Please stage or stash all modified files before running this script."
    git diff --name-only
    exit 1
fi

rm -f clang-format.out
find . -regextype sed -regex '.*\.[h|hpp|c|cpp]' \
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
