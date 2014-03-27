#!/bin/bash
ckpt=/hd2/simics/workspace/booted-knl-mcdram.ckpt
knlmcdram=192.168.1.2
basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ $# -ne 0 ]
then
    if [ $1 == --help ] || [ $1 == -h ]
    then
        echo "Usage: $0 [checkpoint]"
        exit 1
    fi
    ckpt=$1
fi

ping -c 1 -W 1 $knlmcdram
if [ $? -ne 0 ]
then
    export DISPLAY=:0
    echo "read-configuration $ckpt" > simics.in
    echo "continue" >> simics.in
    simics < simics.in >& simics.out &
    echo $! > simics-pid
    sleep 15
    rm simics.in
    echo '#!/bin/sh' > askpass.sh
    echo 'echo $PASSWORD' >> askpass.sh
    chmod u+x askpass.sh
    export SSH_ASKPASS=./askpass.sh
    PASSWORD=mic setsid ssh-copy-id mic@$knlmcdram
    PASSWORD=root setsid ssh-copy-id root@$knlmcdram
    rm askpass.sh
 fi

scp ~/rpmbuild/RPMS/x86_64/numakind-0.0*.rpm mic@$knlmcdram:
scp ~/rpmbuild/RPMS/x86_64/jemalloc-3.5*.rpm mic@$knlmcdram:
scp /usr/lib64/libgtest*.so.0.0.0 mic@$knlmcdram:
scp $basedir/all_tests mic@$knlmcdram:
ssh root@$knlmcdram "rpm -e numakind >& /dev/null"
ssh root@$knlmcdram "rpm -e jemalloc >& /dev/null"
ssh root@$knlmcdram "rpm -i ~mic/jemalloc-3.5*.rpm ~mic/numakind-0.0*.rpm"
ssh root@$knlmcdram "install ~mic/libgtest*.so.0.0.0 /usr/lib64"
ssh root@$knlmcdram "/sbin/ldconfig"
ssh mic@$knlmcdram "./all_tests --gtest_output=xml:all_tests.xml"
scp mic@$knlmcdram:all_tests.xml .
if [ -f simics-pid ]
then
    kill -9 `cat simics-pid`
    rm simics-pid
fi
