#!/bin/bash
#
#  Copyright (C) 2019 Intel Corporation.
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
# set_host_configuration.sh - set Huge Pages parameters required for memkind tests

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
