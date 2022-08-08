---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind-hbw-nodes.1.html"]
title: MEMKIND-HBW_NODES
section: 1
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2016-2022, Intel Corporation)

[comment]: <> (memkind-hbw-nodes.1 -- man page for memkind-auto-dax-kmem-nodes)

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

**memkind-hbw-nodes** - print comma-separated list of high bandwidth nodes.

**Note:** This is EXPERIMENTAL API. The functionality and the header file itself
can be changed (including non-backward compatible changes), or removed.

# SYNOPSIS #

```c
memkind-hbw-nodes [-h|--help]
```

# DESCRIPTION #

memkind-hbw-nodes
:   prints a comma-separated list of high bandwidth NUMA nodes that can be
used with the *numactl --membind* option.

# OPTIONS #

-h, --help
:   Displays help text and exit.

# EXIT STATUS #

+ 0 on success
+ 1 on failure
+ 2 on invalid argument

# COPYRIGHT #

Copyright (C) 2016 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**hbwmalloc**(3), **memkind**(3), **numactl**(8)

# AUTHOR #

Krzysztof Kulakowski <krzysztof.kulakowski@intel.com>
