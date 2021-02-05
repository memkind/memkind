#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

if ! CLANG_FORMAT=$(which $CLANG_FORMAT_BIN)
then
    echo "Package clang-format was not found. Unable to check source files format policy." >&2
    exit 1
fi

CLANG_FORMAT_OPT="--style=file "
if test "$CLANG_FORMAT_ACTION" == "check" ; then
    CLANG_FORMAT_OPT=$CLANG_FORMAT_OPT"--dry-run -Werror"
elif test "$CLANG_FORMAT_ACTION" == "apply" ; then
    CLANG_FORMAT_OPT=$CLANG_FORMAT_OPT"-i"
fi

STYLE_NOT_MATCH=0
while read FILE_NAME; do
    $CLANG_FORMAT $CLANG_FORMAT_OPT $FILE_NAME
    STYLE_NOT_MATCH=$(($STYLE_NOT_MATCH || $?))
done <<< "$(find . -regextype sed -regex '.*\.[h|hpp|c|cpp]' \
     -not -path "./jemalloc/*" -not -path "./test/gtest_fused/*")"

if test "$CLANG_FORMAT_ACTION" == "check" && (($STYLE_NOT_MATCH)) ; then
    echo "Please fix style issues as shown above" >&2
    exit 1
elif test "$CLANG_FORMAT_ACTION" == "check" && ! (($STYLE_NOT_MATCH)) ; then
    echo "Source code is compliant with format policy. No style issues were found."
    exit 0
elif test "$CLANG_FORMAT_ACTION" == "apply" ; then
    echo "Applying code style done."
    exit 0
fi
