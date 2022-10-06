---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["libmemtier.7.html"]
title: LIBMEMTIER
section: 7
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2021-2022, Intel Corporation)

[comment]: <> (libmemtier.7 -- man page for libmemtier)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**ENVIRONMENT**](#environment)\
[**TIER PARAMETERS**](#tier-management)\
[**POLICIES**](#policies)\
[**THRESHOLD PARAMETERS**](#threshold-parameters)\
[**DRAM FALLBACK POLICY**](#dram-fallback-policy)\
[**EXAMPLES**](#examples)\
[**NOTES**](#notes)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**libmemtier.so** - interposer library which enables the memkind memory tiering.

# SYNOPSIS #

In order to use the memkind memory tiering with pre-built
binaries, pass the following environment variables along
with the command:

```c
LD_PRELOAD=libmemtier.so MEMKIND_MEM_TIERS="..." command {arguments ...}
```

# DESCRIPTION #

This library enables the memkind memory tiering mechanism.
With this funcionality, allocations will be split between
different types of memory automatically. The library allows
making allocations with the usage of multiple kinds keeping a
specified ratio between them. This ratio determines how much
of the total allocated memory should be allocated with each
kind. Both **LD_PRELOAD** and **MEMKIND_MEM_TIERS**
environment variables are mandatory for enabling the memkind
memory tiering.

# ENVIRONMENT #

MEMKIND_MEM_TIERS
:   A semicolon-separated list of tier configurations ended
    with a policy parameter. Each of the tier configurations is a
    comma-separated list of tier parameters in the format:
    **param_name:value,(...)** For the list of available tier
    parameters, please see the [**TIER PARAMETERS**](#tier-parameters)
    section below.

MEMKIND_MEM_THRESHOLDS
:   Semicolon-separated list of threshold configurations.
    Each configuration consists of a comma-separated list of
    threshold parameters in the same format as
    **MEMKIND_MEM_TIERS**, where the **param_name** is one of the
    *INIT_VAL*, *MIN_VAL and MAX_VAL*. Each parameter can be set
    only once. When any of the parameters are missing, they will
    be set to a default value. If threshold configuration is
    missing, threshold with default configuration will be used.
    If any of the parameters is set more than once, the
    application will be aborted. Order of provided configurations
    is important - configuration of Nth threshold defines threshold between
    Nth and (N+1)th tier. Note that for N+1
    tiers there should be at most N thresholds defined. Set this
    variable only when **DYNAMIC_THRESHOLD** policy is used. For
    more details about **param_name** see the
    [**THRESHOLD PARAMETERS**](#threshold-parameters)
    section below. See also [**EXAMPLES**](#examples)
    section for the usage of **MEMKIND_MEM_THRESHOLDS**
    environment variable.

# TIER PARAMETERS #

KIND
:   (required) - kind of memory used in memory tier.
    Allowed kind names are *’DRAM’*, *’KMEM_DAX’* and *’FS_DAX’*.
    There can be multiple different kind names provided in the
    **MEMKIND_MEM_TIERS** environment variable - one in each tier
    configuration. Additionally, in this variable more than one
    *’FS_DAX’* kind name is allowed. More information about
    available kinds can be found in the **memkind**(3) manual
    where the *’DRAM’* kind name allocates memory with the usage
    of the **MEMKIND_DEFAULT** kind, the *’KMEM_DAX’* with the
    **MEMKIND_DAX_KMEM** kind and the *’FS_DAX’* with a
    file-backed kind of memory created with the
    `memkind_create_pmem()` function.

PATH
:   (required only for the *FS_DAX* kind) - the path to the
    location where pmem file will be created. The path has to
    exist. For other kinds, if set, will cause an error.

PMEM_SIZE_LIMIT
:   (optional, only for the *FS_DAX* kind) - if set, it
    limits the size of pmem file and the maximum size of total
    allocations from persistent memory. By default, no limit is
    introduced. Pass this option only with FS_DAX kind. For other
    kinds, if set, will cause an error. The accepted formats are:
    1, 1K, 1M, 1G. See the **memkind**(3) manual for information
    about limitations to this value which are the same as for the
    **max_size** value of the `memkind_create_pmem()` function.

POLICY
:   (required, only one in the whole environment variable) -
    determines the algorithm used to distribute allocations
    between provided memory kinds. This parameter has to be the
    last parameter in **MEMKIND_MEM_TIERS** configuration string.
    Currently only *STATIC_RATIO* and *DYNAMIC_THRESHOLD*
    policies are valid. See the [**POLICIES**](#policies) section.

RATIO
:   (required) - the part of the ratio tied to the given
    kind. It’s an *unsigned* type in a range from 1 to
    *UINT_MAX*. See [**EXAMPLES**](#examples) section.

**NOTE:** The application will fail when provided environment
variable string is not in the correct format.

# POLICIES #

STATIC_RATIO
:   All allocations are made in such a way that the ratio
    between memory tiers is constant. No threshold is used.

DYNAMIC_THRESHOLD
:   The ratio between memory tiers is kept with a help of a
    threshold between kinds which value changes in time. Minimum
    two tiers are required for this policy, otherwise the
    application will be aborted. The threshold value can change
    in time to keep the desired ratio between tiers, but it will
    not be lesser than MIN_VAL and it will not be greater than
    MAX_VAL. For every allocation, if its size is greater than or
    equal to INIT_VAL, it will come from the (N+1)th tier.

Default values for a threshold between first two tiers in the
MEMKIND_MEM_TIERS environment variable are:

+ `INIT_VAL = 1024, MIN_VAL = 513, MAX_VAL = 1536.`

If there are more tiers defined, each next undefined
threshold will have all parameters increased by 1024, so the
next undefined threshold between the next two tiers will have:

+ `INIT_VAL = 2048, MIN_VAL = 1537, MAX_VAL = 2560.`

# THRESHOLD PARAMETERS #

INIT_VAL
:   (optional) - the initial value of the threshold between
    two adjacent Nth and (N+1)th tiers. It must be greater than
    or equal to *MIN_VAL* and less than or equal to *MAX_VAL*.

MIN_VAL
:   (optional) - the minimum value of the threshold.

MAX_VAL
:   (optional) - the maximum value of the threshold.

**NOTE:** Because setting the above parameters is optional,
they will be set to default values in case they are not
defined.

# DRAM FALLBACK POLICY #

With the usage of both *DRAM* and *KMEM_DAX* tiers, if there
is not enough memory to satisfy the DRAM tier memory
allocation request, the allocation will fall back to PMEM
memory.

With the usage of both DRAM and *FS_DAX* tiers, if there is
not enough memory to satisfy the *DRAM* tier memory
allocation request, the allocation will fail.

If there is not enough memory to satisfy the *FS_DAX* or
*KMEM_DAX* tier memory allocation request, the allocation
will fail.

# EXAMPLES #

The following example will run ls with the memkind memory
tiering library. Make sure that paths to both
**libmemtier.so** and **libmemkind.so** are included in
**LD_LIBRARY_PATH**. During the application run, 20% of the
allocated memory will come from PMEM memory and 80% will come
from DRAM (*FS_DAX:DRAM* ratio is 1:4):

+ `LD_PRELOAD=libmemtier.so MEMKIND_MEM_TIERS="KIND:FS_DAX,PATH:/mnt/pmem0,PMEM_SIZE_LIMIT:10G,RATIO:1;KIND:DRAM,RATIO:4;POLICY:STATIC_RATIO" /bin/ls -l`

The example value of **MEMKIND_MEM_TIERS** environment
variable where all allocations will come from PMEM memory
with filesystem created with the path */mnt/pmem0*
(PMEM file size is limited only by the specified filesystem):

+ `LD_PRELOAD=libmemtier.so MEMKIND_MEM_TIERS="KIND:FS_DAX,PATH:/mnt/pmem0,RATIO:1;POLICY:STATIC_RATIO"`

The example value of **MEMKIND_MEM_THRESHOLDS** environment
variable. With *INIT_VAL=64*, on the application start all
allocations lower than 64 bytes threshold will come from DRAM
and equal to or greater than this value will come from PMEM
memory NUMA nodes. The threshold value changes during the
runtime in order to maintain the ratio. *MIN_VAL=1* and
*MAX_VAL=10000* set the lower and upper limits of the
threshold value. Note that the *DYNAMIC_THRESHOLD* policy has
to be set in **MEMKIND_MEM_TIERS** environment variable:

+ `LD_PRELOAD=libmemtier.so MEMKIND_MEM_TIERS="KIND:DRAM,RATIO:1;KIND:KMEM_DAX,RATIO:4;POLICY:DYNAMIC_THRESHOLD" MEMKIND_MEM_THRESHOLDS="INIT_VAL:64,MIN_VAL:1,MAX_VAL:10000"`

# NOTES #

**libmemtier** works for applications that do not statically
link a **malloc** implementation.
When **libmemtier** is loaded with **LD_PRELOAD**, allocations with
size zero, like **malloc**(0), have the same result as the system's
standard library call.
Most notably, a valid pointer may be returned in such calls,
contrary to the default memkind behavior of returning NULL when
size zero is passed to malloc-like functions.

# COPYRIGHT #

Copyright (C) 2021 - 2022 Intel Corporation. All rights
reserved.

# SEE ALSO #

**memkind**(3), **malloc**(3)
