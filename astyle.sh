#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2018 - 2020 Intel Corporation.

ASTYLE_MIN_VER="3.1"
ASTYLE_OPT="--style=linux --indent=spaces=4 -S -r --max-continuation-indent=80 "
ASTYLE_OPT+="--max-code-length=80 --break-after-logical --indent-namespaces -z2 "
ASTYLE_OPT+="--align-pointer=name"
if ! ASTYLE=$(which astyle)
then
    echo "Package astyle was not found. Unable to check source files format policy." >&2
    exit 1
fi
ASTYLE_VER=$(astyle --version 2>&1 | awk '{print $NF}')
if (( $(echo "$ASTYLE_VER < $ASTYLE_MIN_VER" | bc -l) ));
then
    echo "Minimal required version of astyle is $ASTYLE_MIN_VER. Detected version is $ASTYLE_VER" >&2
    echo "Unable to check source files format policy." >&2
    exit 1
fi
$ASTYLE $ASTYLE_OPT ./*.c,*.cpp,*.h,*.hpp --exclude=jemalloc > astyle.out
if TEST=$(cat astyle.out | grep -c Formatted)
then
    cat astyle.out
    git --no-pager diff
    echo "Please fix style issues as shown above"
    exit 1
else
    echo "Source code is compliant with format policy. No style issues were found."
    exit 0
fi
