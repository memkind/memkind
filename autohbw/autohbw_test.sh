#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2016 - 2020 Intel Corporation.

# The following is needed if your system does not really have HBW nodes.
# Please see 'man memkind' for a description of this variable
#
# export MEMKIND_HBW_NODES=0

# Set any autohbw env variables
#
export AUTO_HBW_SIZE=4K

# Make sure you have set LD_LIBRARY_PATH to lib directory with libautohbw.so
# and libmemkind.so
#
LD_PRELOAD=libautohbw.so:libmemkind.so /bin/ls
