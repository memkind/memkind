#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

set -e

cd $(dirname $0)
EXTRA_CONF=$@

./autogen.sh
./configure $EXTRA_CONF
make format
