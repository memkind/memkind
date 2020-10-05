#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2020 Intel Corporation.

#Test MEMKIND_DEFAULT kind
MEMKIND_DAX_KMEM_ON=0 LD_LIBRARY_PATH="$(pwd)/.libs" LD_PRELOAD=libmemkind_malloc_wrapper.so ls
#Test PMEM kind
PMEM_KIND_PATH=/tmp LD_LIBRARY_PATH="$(pwd)/.libs" LD_PRELOAD=libmemkind_malloc_wrapper.so ls
#Test MEMKIND_DAX_KMEM kind
MEMKIND_DAX_KMEM_ON=1 MEMKIND_DAX_KMEM_NODES=0 LD_LIBRARY_PATH="$(pwd)/.libs" LD_PRELOAD=libmemkind_malloc_wrapper.so ls
