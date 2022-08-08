---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind-auto-dax-kmem-nodes.1.html"]
title: MEMKIND-AUTO-DAX-NODES
section: 1
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2019-2022, Intel Corporation)

[comment]: <> (memkind-auto-dax-kmem-nodes.1 -- man page for memkind-auto-dax-kmem-nodes)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**OPTIONS**](#options)\
[**EXIT STATUS**](#exit-status)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)\
[**AUTHOR**](#author)


# NAME #

memkind-auto-dax-kmem-nodes - prints comma-separated list of automatically
detected persistent memory NUMA nodes.

**Note:** This is EXPERIMENTAL API. The functionality and the header file itself
can be changed (including non-backward compatible changes), or removed.

# SYNOPSIS #

```c
memkind-auto-dax-kmem-nodes [-h|--help]
```

# DESCRIPTION #

memkind-auto-dax-kmem-nodes
:   prints a comma-separated list of automatically detected persistent memory
NUMA nodes that can be used with the *numactl --membind* option.

# OPTIONS #

-h, --help
:   Displays help text and exit.

# EXIT STATUS #

+ 0 on success
+ 1 if automatic detection is disabled (memkind was not build with
  suitable libdaxctl-devel version)
+ 2 if argument is invalid
+ 3 on other failure

# COPYRIGHT #

Copyright (C) 2019 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**daxctl**(3), **memkind**(3), **numactl**(8)

# AUTHOR #

Michal Biesek <michal.biesek@intel.com>
