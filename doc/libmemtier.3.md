---
draft: false
layout: "library"
slider_enable: true
description: ""
disclaimer: "The contents of this web site and the associated <a href=\"https://github.com/memkind\">GitHub repositories</a> are BSD-licensed open source."
aliases: ["libmemtier.3.html"]
title: LIBMEMTIER
section: 3
---

[comment]: <> (SPDX-License-Identifier: BSD-2-Clause)
[comment]: <> (Copyright 2021-2022, Intel Corporation)

[comment]: <> (libmemtier.3 -- man page for libmemtier)

# TABLE OF CONTENTS #

[**NAME**](#name)\
[**SYNOPSIS**](#synopsis)\
[**DESCRIPTION**](#description)\
[**ENVIRONMENT**](#environment)\
[**COPYRIGHT**](#copyright)\
[**SEE ALSO**](#see-also)


# NAME #

**libmemtier** - memory tiering interface.

**Note:** *memkind_memtier.h* functionality is considered
as a stable API (STANDARD API).

# SYNOPSIS #

```c
#include <memkind_memtier.h>

Link with -lmemkind
```
The API can be used either directly with the usage of C-functions or via
environment variables. See also [**ENVIRONMENT**](#environment) section.

TIER MANAGEMENT:
```c
struct memtier_builder *memtier_builder_new(memtier_policy_t policy);
void memtier_builder_delete(struct memtier_builder *builder);
int memtier_builder_add_tier(struct memtier_builder *builder, memkind_t kind, unsigned kind_ratio);
struct memtier_memory *memtier_builder_construct_memtier_memory(struct memtier_builder *builder);
void memtier_delete_memtier_memory(struct memtier_memory *memory);
```
HEAP MANAGEMENT:
```c
void *memtier_malloc(struct memtier_memory *memory, size_t size);
void *memtier_kind_malloc(memkind_t kind, size_t size);
void *memtier_calloc(struct memtier_memory *memory, size_t num, size_t size);
void *memtier_kind_calloc(memkind_t kind, size_t num, size_t size);
void *memtier_realloc(struct memtier_memory *memory, void *ptr, size_t size);
void *memtier_kind_realloc(memkind_t kind, void *ptr, size_t size);
int memtier_posix_memalign(struct memtier_memory *memory, void **memptr, size_t alignment, size_t size);
int memtier_kind_posix_memalign(memkind_t kind, void **memptr, size_t alignment, size_t size);
size_t memtier_usable_size(void *ptr);
void memtier_free(void *ptr);
void memtier_kind_free(memkind_t kind, void *ptr);
size_t memtier_kind_allocated_size(memkind_t kind);
```
DECORATORS:
```c
void memtier_kind_malloc_post(memkind_t kind, size_t size, void **result);
void memtier_kind_calloc_post(memkind_t kind, size_t nmemb, size_t size, void **result);
void memtier_kind_posix_memalign_post(memkind_t kind, void **memptr, size_t alignment, size_t size, int *err);
void memtier_kind_realloc_post(memkind_t *kind, void *ptr, size_t size, void **result);
void memtier_kind_free_pre(void **ptr);
void memtier_kind_usable_size_post(void **ptr, size_t size);
```
MEMTIER PROPERTY MANAGEMENT:
```c
int memtier_ctl_set(struct memtier_builder *builder, const char *name, const void *val);
```

# DESCRIPTION #

This library enables explicit allocation-time memory tiering. It allows to make
allocations with the usage of multiple kinds keeping a specified ratio between
them. This ratio determines how much of total allocated memory should be
allocated with the usage of each kind.

#### TIER MANAGEMENT: ####

The functions in this section are used to set up, create and destroy the
*memtier_memory* object. This object is passed as an argument to the
`memtier_malloc()` group of functions. It defines the way the allocations
are distributed between different memory tiers.

`memtier_builder_new()`
:   returns a pointer to a new *memtier_builder* object which is used for
    creating the *memtier_memory* object, *p*olicy* determines the policy of
    allocations distribution between tiers by the *memtier_memory* object. See
    the **POLICIES** section in [**libmemtier**(7)](/memkind/manpages/libmemtier.7.html)
    for available options.

`memtier_builder_delete()`
:   deletes the *builder* object releasing the memory it used. Use after the
    *memtier_memory* object is created with the function
    `memtier_builder_construct_memtier_memory()`.

`memtier_builder_add_tier()`
:   adds memory *kind* to the *builder*. This *kind* defines a memory tier used
    in the *memtier_memory* object. This function can be called more than once to
    create several different memory tiers. The "weight" of the tier is determined
    by the *kind_ratio* parameter. The higher it is relative to other tiers’
    *kind_ratio*, the higher the share of allocated memory on that tier, e.g. given
    that the ratio between DRAM and KMEM_DAX is 1:4, the sample allocation sizes are:
    20 GB total, 4 GB DRAM, 16 GB KMEM_DAX.
    **Note:** For more information on kinds see the **KINDS** section
    in the [**memkind**](/memkind/manpages/memkind.3.html)(3).

`memtier_builder_construct_memtier_memory()`
:   returns a pointer to a newly allocated *memtier_memory* object. The *builder*
    can be safely removed after this operation using the
    `memtier_builder_delete()` function.

`memtier_delete_memtier_memory()`
:   deletes the *memory* tiering object releasing the memory it used.

#### HEAP MANAGEMENT ####

The functions described in this section define a heap manager with an interface
modeled on the ISO C standard API’s, except that the user must specify either
the *kind* of memory with the first argument to each function or the tiered
*memory* object which defines memory tiers used for allocations.
See the **KINDS** section in the [**memkind**(3)](/memkind/manpages/memkind.3.html)
manual for a full description of the implemented kinds.

`memtier_malloc()`
:   allocates *size* bytes of memory on one of the memory tiers defined by the
    *memory* parameter. See [**libmemtier**(7)](/memkind/manpages/libmemtier.7.html)
    for further details on memory tiers. `memkind_malloc()` is used for allocations.
    For further details on it’s behavior see
    [**memkind**(3)](/memkind/manpages/memkind.3.html).

`memtier_kind_malloc()`
:   is a wrapper to the `memkind_malloc()` function. See
    [**memkind**(3)](/memkind/manpages/memkind.3.html) for further details.

`memtier_calloc()`
:   allocates *num* times *size* bytes of memory on one of the memory tiers
    defined by the *memory* argument. `memkind_calloc()` is used for allocations.
    For further details on it’s behavior see [**memkind**(3)](/memkind/manpages/memkind.3.html).

`memtier_kind_calloc()`
:   is a wrapper to the `memkind_calloc()` function. See
    [**memkind**(3)](/memkind/manpages/memkind.3.html) for further details.

`memtier_realloc()`
:   changes the size of the previously allocated memory referenced by *ptr*
    to *size* bytes using memory from the tier on which *ptr* is allocated. If
    *ptr* is NULL, new memory is allocated on a memory tier defined by the *memory*
    argument. `memkind_realloc()` is used for reallocation. See
    [**memkind**(3)](/memkind/manpages/memkind.3.html) for further details.

`memtier_kind_realloc()`
:   changes the size of the previously allocated memory referenced by *ptr* to
    *size* bytes using specific kind. If *size* is equal to zero and *ptr* is not NULL,
    then the call is equivalent to `memkind_free(kind, ptr)` and NULL is returned.
    If *ptr* is NULL, `memtier_kind_malloc()` is called to allocate new memory.
    Otherwise, the `memkind_realloc()` function is used. See
    [**memkind**(3)](/memkind/manpages/memkind.3.html) for further details.

`memtier_posix_memalign()`
:   is a wrapper of `memkind_posix_memalign()` with the main difference that the
    *memory* is used to determine the kind to be used for the allocation. See
    [**memkind**(3)](/memkind/manpages/memkind.3.html) for further details.

`memtier_kind_posix_memalign()`
:   is a wrapper of `memkind_posix_memalign()`. See
    [**memkind**(3)](/memkind/manpages/memkind.3.html) for further details.

`memtier_usable_size()`
:   returns the *size* of the block of memory allocated with the memtier
    API at the address pointed by *ptr*.

`memtier_free()`
:   is a wrapper for the `memtier_kind_free()` function with the *kind*
    parameter passed as NULL.

`memtier_kind_free()`
:   frees up the memory pointed to by *ptr*. The behavior is the same as for
    the `memkind_free()`. If *kind* is NULL, the *kind* used to allocate *ptr*
    is detected automatically. See [**memkind**(3)](/memkind/manpages/memkind.3.html)
    for further details.

`memtier_kind_allocated_size()`
:   returns the total size of memory allocated with the usage of *kind* and
    the memtier API.

#### DECORATORS: ####

This is the set of functions used to print information on each call to the
respective `memtier_kind_*` function described in the
[**HEAP MANAGEMENT**](#heap-management) section. Printed information include
the name of the *kind* used, parameters passed to the underlying function from
the malloc family of functions and the address of the memory returned.

#### MEMTIER PROPERTY MANAGEMENT: ####

`memtier_ctl_set()`
:   is useful for changing the default values of parameters that define the
    *DYNAMIC_THRESHOLD* policy. This function can be used in the process of
    creating a **memtier_memory** object with the usage of *builder*.\
    The parameter *name* can be one of the following:

+ **policy.dynamic_threshold.thresholds[ID].val**\
 initial threshold level, all allocations of the size below this value will come
 from the IDth tier, greater than or equal to this value will come from the (ID+1)th tier.
 Provided string is converted to the *size_t* type. This value is modified automatically
 during the application run to keep the desired ratio between tiers. The default value
 between first two tiers is 1024 bytes.
+ **policy.dynamic_threshold.thresholds[ID].min**\
 minimum value of the threshold level. Provided string is converted to the *size_t* type.
 The default value between first two tiers is 513 bytes.
+ **policy.dynamic_threshold.thresholds[ID].max**\
 maximum value of the threshold level. Provided string is converted to the *size_t* type.
 The default value between first two tiers is 1536 bytes.
+ **policy.dynamic_threshold.check_cnt**\
 number of allocation operations (i.e. malloc, realloc) after which the ratio check
 between tiers is performed. Provided string is converted to the *unsigned int* type.
 The default value is 20.
+ **policy.dynamic_threshold.trigger**\
 the dynamic threshold value is adjusted when the absolute difference between current
 ratio and expected ratio is greater than or equal to this value. Provided string is
 converted to the *float* type. The default value is 0.02.
+ **policy.dynamic_threshold.degree**\
 the threshold value is updated by increasing or decreasing it’s value by degree percentage
 (i.e. degree=0.02 changes threshold value by 2%). Provided string is converted to the *float*
 type. The default value is 0.15.

In the above examples, ID should be replaced with the ID of thresholds configuration.
The configuration between first two tiers added to builder has an ID equal to 0.
The configuration ID of the next two tiers, that is, the second and third ones,
is equal to 1, and so on. The last configuration’s ID is equal to the number of
tiers minus one.

# ENVIRONMENT #

See **libmemtier**(7) for details on the usage of memkind tiering via environment variables.

# COPYRIGHT #

Copyright (C) 2021 - 2022 Intel Corporation. All rights reserved.

# SEE ALSO #

**libmemtier**(7), **memkind**(3), **memkind_malloc**(3), **memkind_calloc**(3), **memkind_realloc**(3), **memkind_free**(3), **memkind_posix_memalign**(3)
