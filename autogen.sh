#!/bin/bash
#
#  Copyright (C) 2014 - 2019 Intel Corporation.
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

set -e

# If the VERSION file does not exist, then create it based on git
# describe or if not in a git repo just set VERSION to 0.0.0.
# Determine VERSION scheme, based on git:
# -if HEAD is tag annotated - it is official release e.g. setting VERSION to "1.9.0"
# -if HEAD is not tag annotated - it is snapshot e.g. setting VERSION to "1.8.0+dev19+g1c93b2d"

if [ ! -f VERSION ]; then
    if [ -f .git/config ]; then
        sha=$(git describe --long | awk -F- '{print $(NF)}')
        release=$(git describe --long | awk -F- '{print $(NF-1)}')
        version=$(git describe --long | sed -e "s|\(.*\)-$release-$sha|\1|" -e "s|-|+|g" -e "s|^v||")
        if [ ${release} != "0" ]; then
            echo "WARNING: No annotated tag referring to this commit was found, setting version to development build " 2>&1
            version=${version}+dev${release}+${sha}
        else
            echo "Annotated tag referring to this commit was found, setting version as an official release" 2>&1
            version=${version}
        fi
    else
        echo "WARNING: VERSION file does not exist and working directory is not a git repository, setting version to 0.0.0" 2>&1
        version=0.0.0
    fi
    echo $version > VERSION
fi

autoreconf --install

