#!/bin/bash

basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
numactl --hardware | grep "^node 1" > /dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: $0 requires a NUMA enabled system with more than one node."
    exit -1
fi

export NUMAKIND_HBW_NODES=1
numactl --membind=0 $basedir/all_tests $@
