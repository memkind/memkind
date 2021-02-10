// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2020 Intel Corporation. */

#include <dlfcn.h>
#include <stdio.h>

extern void *(*scalable_malloc)(size_t);
extern void *(*scalable_realloc)(void *, size_t);
extern void *(*scalable_calloc)(size_t, size_t);
extern void (*scalable_free)(void *);

int load_tbbmalloc_symbols();
