#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019 Intel Corporation.

#
# set_host_configuration.sh - set Huge Pages parameters required for memkind tests
set -e

MEMKIND_HUGE_PAGES_NO=3000
MEMKIND_OVERCOMMIT_HUGEPAGES_NO=128

# find out current configuration
MEMKIND_HUGE_PAGES_FOUND=$(cat /proc/sys/vm/nr_hugepages)
MEMKIND_OVERCOMMIT_HUGEPAGES_FOUND=$(cat /proc/sys/vm/nr_overcommit_hugepages)

# set expected configuration
if [ "$MEMKIND_HUGE_PAGES_FOUND" != "$MEMKIND_HUGE_PAGES_NO" ]; then
    echo "Setting number of hugepages to ${MEMKIND_HUGE_PAGES_NO}."
    sudo sysctl vm.nr_hugepages="$MEMKIND_HUGE_PAGES_NO"
fi
if [ "$MEMKIND_OVERCOMMIT_HUGEPAGES_FOUND" != "$MEMKIND_OVERCOMMIT_HUGEPAGES_NO" ]; then
    echo "Setting number of overcommit hugepages to ${MEMKIND_OVERCOMMIT_HUGEPAGES_NO}."
    sudo sysctl vm.nr_overcommit_hugepages="$MEMKIND_OVERCOMMIT_HUGEPAGES_NO"
fi
