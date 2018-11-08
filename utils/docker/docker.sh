#!/usr/bin/env bash
#
# Copyright 2018, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# docker.sh - preparation and run of memkind unit tests
# Parameters:
# -number of pull request on memkind repository on github
# -directory inside /mnt which will be mounted to /dev/pmem
#  e.g /mnt/pmem0 mounted to /dev/pmem0
# -Codecov token for memkind repository to upload the coverage results
#

MEMKIND_URL=https://github.com/memkind/memkind.git

# set parameters for enabling the kernel's huge page pool
sudo sysctl vm.nr_hugepages=3000
sudo sysctl vm.nr_overcommit_hugepages=128

# prepare build directory
mkdir /mnt/$2
sudo mount -t ext4 -o dax /dev/$2 /mnt/$2

# cloning the repo and merging with current master
cd $HOME
git clone $MEMKIND_URL
cd $HOME/memkind
git fetch origin pull/$1/head:$1branch
git checkout $1branch
git merge master --no-commit --no-ff

# building jemalloc and memkind
./build_jemalloc.sh
./autogen.sh
./configure --enable-gcov
make -j8
make checkprogs -j8
sudo make install

# coping libraries and binaries as some tests expect them in non standard locations
sudo cp /usr/local/lib/lib* /usr/lib

# running tests
set +e
make check || true
cat test-suite.log

# run pmem examples
cd $HOME/memkind/examples/.libs
find . -name "pmem*" -executable -type f -exec sh -c "sudo "{}" /mnt/$2" \;

# run coverage
cd $HOME/memkind
make test-clean
cat << EOF > script.sh
#!/bin/bash
# -p dir       Project root directory
# -s DIR       Directory to search for coverage reports.
# -Z           Exit with 1 if not successful. Default will Exit with 0

bash <(curl -s https://codecov.io/bash) -t $3 -p $HOME/memkind -s $HOME/memkind -Z
EOF
chmod 700 script.sh
./script.sh
