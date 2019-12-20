#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2018 - 2019 Intel Corporation.

set -ex
curl -SL https://sourceforge.net/projects/astyle/files/astyle/astyle%203.1/astyle_3.1_linux.tar.gz -o /tmp/astyle.tar.gz
tar -xzvf /tmp/astyle.tar.gz -C /tmp/
echo "Install astyle"
cd /tmp/astyle/build/gcc
make
sudo make install
