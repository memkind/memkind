#!/bin/bash
#
#  Copyright (C) 2018 Intel Corporation.
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

ASTYLE_OPT="--style=linux --indent=spaces=4 -S -r "
ASTYLE_OPT+="--max-code-length=80 --break-after-logical --indent-namespaces"
if ! ASTYLE=$(which astyle)
then
    echo "Package astyle was not found. Unable to check source files format policy." >&2
    exit 1
fi
$ASTYLE $ASTYLE_OPT ./*.c --exclude=jemalloc >> astyle.out
$ASTYLE $ASTYLE_OPT ./*.cpp --exclude=jemalloc >> astyle.out
$ASTYLE $ASTYLE_OPT ./*.h --exclude=jemalloc >> astyle.out
$ASTYLE $ASTYLE_OPT ./*.hpp --exclude=jemalloc >> astyle.out
TEST=$(cat astyle.out | grep -c Formatted)
if [[ $TEST -ne 0 ]]; then
    cat astyle.out
    git --no-pager diff
    echo "Please fix style issues as shown above"
    exit 1
else
    echo "Source code is compliant with format policy. No style issues were found."
    exit 0
fi
