---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["memkind_interleave.3.html"]
title: MEMKIND_INTERLEAVE
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2016-2022, Intel Corporation)

[comment]: <> (memkind_interleave.3 -- man page for memkind_interleave)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**memkind_interleave** - interleave memory memkind operations.

**Note:** This is EXPERIMENTAL API. The functionality and the header file
itself can be changed (including non-backward compatible changes) or removed.

# SYNOPSIS #

```c
void memkind_interleave_init_once(void);
```

# DESCRIPTION #

`memkind_interleave_init_once()` initializes **MEMKIND_INTERLEAVE** kind and
it should not be called more than once. **Note:** `memkind_interleave_init_once()`
may reserve some extra memory.

# COPYRIGHT #

Copyright (C) 2016 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**memkind**(3), **memkind_arena**(3), **memkind_default**(3), **memkind_hbw**(3), **memkind_pmem**(3), **jemalloc**(3), **mbind**(2), **mmap**(2)
