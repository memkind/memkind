#!/bin/bash

basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
simics_ckpt=/hd2/simics/workspace/booted-knl-mcdram.ckpt
simics_ip=192.168.1.2
simics_login=mic

if [ $# -ne 0 ]
then
    if [ $1 == --help ] || [ $1 == -h ]
    then
        echo "Usage: $0 [checkpoint]"
        exit 1
    fi
    simics_ckpt=$1
fi

ping -c 1 -W 1 $simics_ip
if [ $? -ne 0 ]
then
    export DISPLAY=:0
    echo "read-configuration $simics_ckpt" > simics.in
    echo "continue" >> simics.in
    simics < simics.in >& simics.out &
    echo $! > simics.pid
    sleep 15
    rm simics.in
    echo '#!/bin/sh' > askpass.sh
    echo 'echo $PASSWORD' >> askpass.sh
    chmod u+x askpass.sh
    export SSH_ASKPASS=./askpass.sh
    PASSWORD=$simics_login setsid ssh-copy-id $simics_login@$simics_ip
    PASSWORD=root setsid ssh-copy-id root@$simics_ip
    rm askpass.sh
    ssh root@$simics_ip passwd -d mic
    ssh root@$simics_ip passwd -d root
fi
