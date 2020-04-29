#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2014 - 2019 Intel Corporation.

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

