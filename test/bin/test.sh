#!/bin/bash
knlmcdram=192.168.1.2
basedir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

ping -c 1 -W 1 $knlmcdram
if [ $? -ne 0 ]
then
    echo "read-configuration /hd2/simics/workspace/booted-knl-mcdram.ckpt" > simics.in
    echo "continue" >> simics.in
    nohup simics < simics.in >& simics.out &
    sleep 15
fi

ssh-copy-id mic@$knlmcdram
ssh-copy-id root@$knlmcdram
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

